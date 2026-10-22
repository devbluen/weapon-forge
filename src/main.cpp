// Entry points. The same binary is:
// - a SA-MP plugin (Supports/Load/AmxLoad/ProcessTick...), also loadable by
//   open.mp as a legacy plugin;
// - an open.mp component (ComponentEntryPoint, see omp_component.cpp), which
//   is the recommended way on open.mp because it gives network access.
#include "sdk/plugincommon.h"

#include "amx_api.hpp"
#include "amxhooks.hpp"
#include "core.hpp"
#include "main.hpp"
#include "natives.hpp"
#include "net_samp.hpp"
#include "scripts.hpp"
#include "server.hpp"
#include "version.hpp"
#include "weapons.hpp"

#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <string>

#if defined(_WIN32)
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#include <windows.h>
#else
	#include <unistd.h>
#endif

namespace wf
{
	Config g_config;
	Player g_players[kMaxPlayers];
	Vehicle g_vehicles[kMaxVehicles + 1];
	bool g_open_mp = false;
	bool g_raknet = false;
	int g_max_players = kMaxPlayers;
	bool g_emitting = false;

	namespace
	{
		Logger g_logger = nullptr;
		bool g_started = false;
		const auto g_start = std::chrono::steady_clock::now();

		void VLog(const char *prefix, const char *fmt, va_list args)
		{
			char buffer[1024];
			std::vsnprintf(buffer, sizeof(buffer), fmt, args);

			if (g_logger)
			{
				g_logger("[weapon-forge] %s%s", prefix, buffer);
			}
		}

		bool DetectOpenMP()
		{
			std::string path;

#if defined(_WIN32)
			char buffer[MAX_PATH] = {};
			GetModuleFileNameA(nullptr, buffer, MAX_PATH);
			path = buffer;
#else
			char buffer[4096] = {};
			ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
			if (len > 0)
			{
				path.assign(buffer, static_cast<size_t>(len));
			}
#endif

			for (char &c : path)
			{
				if (c >= 'A' && c <= 'Z')
				{
					c = static_cast<char>(c - 'A' + 'a');
				}
			}

			return path.find("omp-server") != std::string::npos || path.find("open.mp") != std::string::npos;
		}
	}

	uint32_t Tick()
	{
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - g_start).count();
		// Never 0: 0 means "never" in several timestamps
		return static_cast<uint32_t>(ms) + 1000u;
	}

	void Log(const char *fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		VLog("", fmt, args);
		va_end(args);
	}

	void Debug(const char *fmt, ...)
	{
		if (!g_config.debug)
		{
			return;
		}

		va_list args;
		va_start(args, fmt);
		VLog("(debug) ", fmt, args);
		va_end(args);
	}

	bool Startup(void **exports, Logger logger, bool component)
	{
		if (g_started)
		{
			return true;
		}

		g_logger = logger;
		g_open_mp = component || DetectOpenMP();

		amx::Init(exports);
		weapons::Reset();
		timers::Clear();

		if (!amxhooks::Install(exports))
		{
			Log("ERROR: could not hook the AMX functions, the plugin is disabled");
			amxhooks::Remove();
			return false;
		}

		g_started = true;
		Log("v%s loaded (%s%s)", WF_VERSION, g_open_mp ? "open.mp" : "SA-MP", component ? " component" : "");

		if (g_open_mp && !component)
		{
			Log("WARN: on open.mp, install weapon-forge in components/ (not as a legacy plugin) to enable the sync features (nametag health, sync fixes)");
		}

		return true;
	}

	void Shutdown()
	{
		if (!g_started)
		{
			return;
		}

		net_samp::Remove();
		amxhooks::Remove();
		timers::Clear();
		g_started = false;
		Log("Unloaded.");
	}

	int AmxLoad(AMX *amx)
	{
		scripts::OnLoad(amx);
		return RegisterNatives(amx);
	}

	int AmxUnload(AMX *amx)
	{
		amxhooks::OnAmxUnload(amx);
		scripts::OnUnload(amx);
		return AMX_ERR_NONE;
	}

	void ServerTick()
	{
		if (!g_open_mp)
		{
			net_samp::TryInstall();
		}

		if (server::Context())
		{
			ProcessTick();
		}
	}
}

// ---------------------------------------------------------------------------
// SA-MP plugin interface
// ---------------------------------------------------------------------------

PLUGIN_EXPORT unsigned int PLUGIN_CALL Supports()
{
	return SUPPORTS_VERSION | SUPPORTS_AMX_NATIVES | SUPPORTS_PROCESS_TICK;
}

PLUGIN_EXPORT bool PLUGIN_CALL Load(void **ppData)
{
	auto logger = reinterpret_cast<wf::Logger>(ppData[PLUGIN_DATA_LOGPRINTF]);

	if (!wf::Startup(static_cast<void **>(ppData[PLUGIN_DATA_AMX_EXPORTS]), logger, false))
	{
		return false;
	}

	if (!wf::g_open_mp)
	{
		net_samp::Init(ppData);
	}

	return true;
}

PLUGIN_EXPORT void PLUGIN_CALL Unload()
{
	wf::Shutdown();
}

PLUGIN_EXPORT int PLUGIN_CALL AmxLoad(AMX *amx)
{
	return wf::AmxLoad(amx);
}

PLUGIN_EXPORT int PLUGIN_CALL AmxUnload(AMX *amx)
{
	return wf::AmxUnload(amx);
}

PLUGIN_EXPORT void PLUGIN_CALL ProcessTick()
{
	wf::ServerTick();
}
