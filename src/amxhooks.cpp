#include "amxhooks.hpp"

#include "amx_api.hpp"
#include "core.hpp"
#include "hook.hpp"
#include "natives.hpp"
#include "scripts.hpp"
#include "server.hpp"

#include "sdk/plugincommon.h"

#include <cstring>
#include <unordered_map>
#include <vector>

namespace amxhooks
{
	namespace
	{
		using ExecFn = int (*)(AMX *, cell *, int);
		using FindFn = int (*)(AMX *, const char *, int *);
		using RegisterFn = int (*)(AMX *, const AMX_NATIVE_INFO *, int);

		Hook g_exec_hook;
		Hook g_find_hook;
		Hook g_register_hook;

		// Public index reported for callbacks a script doesn't implement
		constexpr int kFakeIndex = 0x7F000000;

		enum Callback
		{
			CB_GameModeInit,
			CB_GameModeExit,
			CB_PlayerConnect,
			CB_PlayerDisconnect,
			CB_PlayerSpawn,
			CB_PlayerRequestClass,
			CB_PlayerDeath,
			CB_PlayerKeyStateChange,
			CB_PlayerEnterVehicle,
			CB_PlayerExitVehicle,
			CB_PlayerStateChange,
			CB_PlayerUpdate,
			CB_PlayerGiveDamage,
			CB_PlayerTakeDamage,
			CB_PlayerWeaponShot,
			CB_VehicleSpawn,
			CB_VehicleDeath,
			// Filters only: no need to raise them when no script has them
			CB_PlayerPickUpPickup,
			CB_PlayerPickUpDynamicPickup,
			CB_PlayerEnterCheckpoint,
			CB_PlayerLeaveCheckpoint,
			CB_PlayerEnterRaceCheckpoint,
			CB_PlayerLeaveRaceCheckpoint,
			CB_COUNT
		};

		constexpr int kFirstFilterOnly = CB_PlayerPickUpPickup;

		const char *const kCallbackNames[CB_COUNT] = {
			"OnGameModeInit",
			"OnGameModeExit",
			"OnPlayerConnect",
			"OnPlayerDisconnect",
			"OnPlayerSpawn",
			"OnPlayerRequestClass",
			"OnPlayerDeath",
			"OnPlayerKeyStateChange",
			"OnPlayerEnterVehicle",
			"OnPlayerExitVehicle",
			"OnPlayerStateChange",
			"OnPlayerUpdate",
			"OnPlayerGiveDamage",
			"OnPlayerTakeDamage",
			"OnPlayerWeaponShot",
			"OnVehicleSpawn",
			"OnVehicleDeath",
			"OnPlayerPickUpPickup",
			"OnPlayerPickUpDynamicPickup",
			"OnPlayerEnterCheckpoint",
			"OnPlayerLeaveCheckpoint",
			"OnPlayerEnterRaceCheckpoint",
			"OnPlayerLeaveRaceCheckpoint",
		};

		enum class Action
		{
			Run,  // let the script run
			Skip, // don't run the script, return `skip_ret`
		};

		// One server callback is raised in every script (filterscripts, then
		// the gamemode). The plugin processes it once, on the first script,
		// and applies the same decision to the others.
		struct Event
		{
			bool active = false;
			uint64_t frame = 0;
			AMX *seen[32] = {};
			int seen_count = 0;
			cell args[8] = {};
			int arg_count = 0;

			Action action = Action::Run;
			cell skip_ret = 1;
			cell killerid = 0;
			cell reason = 0;
			bool shot_valid = false;
		};

		Event g_events[CB_COUNT];
		uint64_t g_frame = 1;

		// Next plugin-originated call that must bypass the hooks
		struct
		{
			AMX *amx = nullptr;
			int index = 0;
		} g_internal_exec;

		bool g_internal_find = false;

		std::unordered_map<AMX *, std::vector<signed char>> g_public_map;

		int CallExec(AMX *amx, cell *retval, int index)
		{
			Hook::Unpatched guard(g_exec_hook);
			return reinterpret_cast<ExecFn>(g_exec_hook.Original())(amx, retval, index);
		}

		int CallFind(AMX *amx, const char *name, int *index)
		{
			Hook::Unpatched guard(g_find_hook);
			return reinterpret_cast<FindFn>(g_find_hook.Original())(amx, name, index);
		}

		int CallRegister(AMX *amx, const AMX_NATIVE_INFO *list, int number)
		{
			Hook::Unpatched guard(g_register_hook);
			return reinterpret_cast<RegisterFn>(g_register_hook.Original())(amx, list, number);
		}

		int CallbackByName(const char *name)
		{
			if (name[0] != 'O' || name[1] != 'n')
			{
				return -1;
			}

			for (int i = 0; i < CB_COUNT; ++i)
			{
				if (!std::strcmp(kCallbackNames[i], name))
				{
					return i;
				}
			}

			return -1;
		}

		const std::vector<signed char> &PublicMap(AMX *amx)
		{
			auto it = g_public_map.find(amx);

			if (it != g_public_map.end())
			{
				return it->second;
			}

			std::vector<signed char> map;
			auto *hdr = reinterpret_cast<AMX_HEADER *>(amx->base);

			if (hdr && hdr->defsize == sizeof(AMX_FUNCSTUBNT))
			{
				int count = (hdr->natives - hdr->publics) / hdr->defsize;
				auto *stubs = reinterpret_cast<AMX_FUNCSTUBNT *>(amx->base + hdr->publics);
				map.resize(static_cast<size_t>(count), -1);

				for (int i = 0; i < count; ++i)
				{
					map[static_cast<size_t>(i)] = static_cast<signed char>(CallbackByName(reinterpret_cast<const char *>(amx->base + stubs[i].nameofs)));
				}
			}

			return g_public_map.emplace(amx, std::move(map)).first->second;
		}

		int CallbackByIndex(AMX *amx, int index)
		{
			if (index >= kFakeIndex && index < kFakeIndex + CB_COUNT)
			{
				return index - kFakeIndex;
			}

			if (index < 0)
			{
				return -1;
			}

			const auto &map = PublicMap(amx);
			return static_cast<size_t>(index) < map.size() ? map[static_cast<size_t>(index)] : -1;
		}

		// Discards the arguments pushed for a call that won't run
		int SkipExec(AMX *amx, cell *retval, cell ret)
		{
			amx->stk += amx->paramcount * static_cast<cell>(sizeof(cell));
			amx->paramcount = 0;

			if (retval)
			{
				*retval = ret;
			}

			return AMX_ERR_NONE;
		}

		bool IsNewEvent(const Event &e, AMX *amx, const cell *args, int count)
		{
			if (!e.active || e.frame != g_frame || e.arg_count != count)
			{
				return true;
			}

			for (int i = 0; i < count && i < 8; ++i)
			{
				if (e.args[i] != args[i])
				{
					return true;
				}
			}

			// Each script receives an event once: seeing a script again means
			// a new event of the same kind.
			for (int i = 0; i < e.seen_count; ++i)
			{
				if (e.seen[i] == amx)
				{
					return true;
				}
			}

			return false;
		}

		bool Connected(cell playerid)
		{
			return wf::ValidPlayer(playerid) && wf::P(playerid).connected;
		}

		void Skip(Event &e, cell ret)
		{
			e.action = Action::Skip;
			e.skip_ret = ret;
		}

		// Processes a new event; decides what happens in the scripts.
		void Decide(int cb, Event &e, const cell *a, int n)
		{
			using namespace wf;

			e.action = Action::Run;
			e.skip_ret = 1;

			auto arg = [&](int i) -> cell { return i < n ? a[i] : 0; };
			auto argf = [&](int i) -> float { return amx_ctof(arg(i)); };

			switch (cb)
			{
			case CB_GameModeInit:
				ScriptInit();
				break;

			case CB_GameModeExit:
				ScriptExit();
				break;

			case CB_PlayerConnect:
				if (ValidPlayer(arg(0)))
				{
					OnPlayerConnect(arg(0));
				}
				break;

			case CB_PlayerDisconnect:
				if (Connected(arg(0)))
				{
					OnPlayerDisconnect(arg(0));
				}
				break;

			case CB_PlayerSpawn:
				if (Connected(arg(0)))
				{
					OnPlayerSpawn(arg(0));
				}
				break;

			case CB_PlayerRequestClass:
				if (Connected(arg(0)))
				{
					OnPlayerRequestClass(arg(0));
				}
				break;

			case CB_PlayerDeath:
				if (Connected(arg(0)))
				{
					int killerid = arg(1), reason = arg(2);

					if (!OnRealDeath(arg(0), killerid, reason))
					{
						Skip(e, 1);
					}

					e.killerid = killerid;
					e.reason = reason;
				}
				else
				{
					e.killerid = arg(1);
					e.reason = arg(2);
				}
				break;

			case CB_PlayerKeyStateChange:
				if (Connected(arg(0)) && !OnPlayerKeyStateChange(arg(0), arg(1), arg(2)))
				{
					Skip(e, 0);
				}
				break;

			case CB_PlayerEnterVehicle:
				if (Connected(arg(0)))
				{
					OnPlayerEnterVehicle(arg(0));
				}
				break;

			case CB_PlayerExitVehicle:
				if (Connected(arg(0)))
				{
					OnPlayerExitVehicle(arg(0));
				}
				break;

			case CB_PlayerStateChange:
				if (Connected(arg(0)))
				{
					OnPlayerStateChange(arg(0), arg(1), arg(2));
				}
				break;

			case CB_PlayerUpdate:
				if (Connected(arg(0)))
				{
					switch (OnPlayerUpdate(arg(0)))
					{
					case 0:
						Skip(e, 0);
						break;
					case 2:
						Skip(e, 1);
						break;
					}
				}
				break;

			case CB_PlayerGiveDamage:
				if (Connected(arg(0)))
				{
					OnGiveDamage(arg(0), arg(1), argf(2), arg(3), arg(4));
				}
				if (!g_config.forward_damage_callbacks)
				{
					Skip(e, 0);
				}
				break;

			case CB_PlayerTakeDamage:
				if (Connected(arg(0)))
				{
					OnTakeDamage(arg(0), arg(1), argf(2), arg(3), arg(4));
				}
				if (!g_config.forward_damage_callbacks)
				{
					Skip(e, 0);
				}
				break;

			case CB_PlayerWeaponShot:
				e.shot_valid = false;

				if (Connected(arg(0)))
				{
					if (!OnWeaponShot(arg(0), arg(1), arg(2), arg(3), argf(4), argf(5), argf(6)))
					{
						Skip(e, 0);
					}
					else
					{
						e.shot_valid = true;
						OnWeaponShotResult(arg(0), arg(2), arg(3), true);
					}
				}
				break;

			case CB_VehicleSpawn:
				OnVehicleSpawn(arg(0));
				break;

			case CB_VehicleDeath:
				if (!OnVehicleDeath(arg(0)))
				{
					Skip(e, 1);
				}
				break;

			case CB_PlayerPickUpPickup:
			case CB_PlayerPickUpDynamicPickup:
			case CB_PlayerEnterCheckpoint:
			case CB_PlayerEnterRaceCheckpoint:
				if (Connected(arg(0)) && !IsPlayerSpawned(arg(0)))
				{
					Skip(e, 0);
				}
				break;

			case CB_PlayerLeaveCheckpoint:
			case CB_PlayerLeaveRaceCheckpoint:
				// While dying, the plugin raises it when the death animation starts
				if (Connected(arg(0)) && P(arg(0)).dying)
				{
					Skip(e, 0);
				}
				break;
			}
		}

		int ExecDetour(AMX *amx, cell *retval, int index)
		{
			if (g_internal_exec.amx == amx && g_internal_exec.index == index)
			{
				g_internal_exec.amx = nullptr;
				return CallExec(amx, retval, index);
			}

			if (index == AMX_EXEC_MAIN)
			{
				// Only the gamemode's main() is run by the server
				scripts::SetGameMode(amx);
				return CallExec(amx, retval, index);
			}

			int cb = CallbackByIndex(amx, index);
			bool fake = index >= kFakeIndex;

			if (cb < 0)
			{
				return fake ? SkipExec(amx, retval, 1) : CallExec(amx, retval, index);
			}

			// Arguments pushed by the server (data is null when it's part of base)
			unsigned char *data = amx->data ? amx->data : amx->base + reinterpret_cast<AMX_HEADER *>(amx->base)->dat;
			cell *args = reinterpret_cast<cell *>(data + amx->stk);
			int count = amx->paramcount;
			Event &e = g_events[cb];

			if (IsNewEvent(e, amx, args, count))
			{
				e.active = true;
				e.frame = g_frame;
				e.seen_count = 0;
				e.arg_count = count;

				for (int i = 0; i < count && i < 8; ++i)
				{
					e.args[i] = args[i];
				}

				Decide(cb, e, args, count);
			}

			if (e.seen_count < 32)
			{
				e.seen[e.seen_count++] = amx;
			}

			if (e.action == Action::Skip)
			{
				return SkipExec(amx, retval, e.skip_ret);
			}

			if (cb == CB_PlayerDeath && count >= 3)
			{
				// Same corrected killer/reason for every script
				args[1] = e.killerid;
				args[2] = e.reason;
			}

			if (fake)
			{
				return SkipExec(amx, retval, 1);
			}

			cell ret = 1;
			int err = CallExec(amx, &ret, index);

			if (retval)
			{
				*retval = ret;
			}

			if (cb == CB_PlayerWeaponShot && e.shot_valid && count >= 4 && Connected(e.args[0]))
			{
				// Any script returning 0 invalidates the shot
				e.shot_valid = ret != 0;
				wf::OnWeaponShotResult(e.args[0], e.args[2], e.args[3], e.shot_valid);
			}

			return err;
		}

		int FindDetour(AMX *amx, const char *name, int *index)
		{
			if (g_internal_find)
			{
				return CallFind(amx, name, index);
			}

			int err = CallFind(amx, name, index);

			if (err != AMX_ERR_NONE && name && index)
			{
				int cb = CallbackByName(name);

				// Raise the callbacks the plugin needs in one script at least
				if (cb >= 0 && cb < kFirstFilterOnly && (!scripts::GameMode() || scripts::GameMode() == amx))
				{
					*index = kFakeIndex + cb;
					return AMX_ERR_NONE;
				}
			}

			return err;
		}

		int RegisterDetour(AMX *amx, const AMX_NATIVE_INFO *list, int number)
		{
			if (!list)
			{
				return CallRegister(amx, list, number);
			}

			std::vector<AMX_NATIVE_INFO> copy;
			int count = 0;

			while (number < 0 ? list[count].name != nullptr : count < number)
			{
				++count;
			}

			for (int i = 0; i < count; ++i)
			{
				const AMX_NATIVE_INFO &native = list[i];

				if (!native.name || !native.func)
				{
					continue;
				}

				AMX_NATIVE replacement = wf::NativeOverride(native.name);

				if (replacement == native.func)
				{
					continue;
				}

				server::Capture(native.name, native.func);

				if (replacement)
				{
					if (copy.empty())
					{
						copy.assign(list, list + count);
						copy.push_back({nullptr, nullptr});
					}

					copy[static_cast<size_t>(i)].func = replacement;
				}
			}

			if (copy.empty())
			{
				return CallRegister(amx, list, number);
			}

			return CallRegister(amx, copy.data(), number < 0 ? -1 : number);
		}
	}

	bool Install(void **exports)
	{
		bool ok = g_register_hook.Install(exports[PLUGIN_AMX_EXPORT_Register], reinterpret_cast<void *>(&RegisterDetour));
		ok = g_find_hook.Install(exports[PLUGIN_AMX_EXPORT_FindPublic], reinterpret_cast<void *>(&FindDetour)) && ok;
		ok = g_exec_hook.Install(exports[PLUGIN_AMX_EXPORT_Exec], reinterpret_cast<void *>(&ExecDetour)) && ok;

		if (!g_exec_hook.HasTrampoline() || !g_find_hook.HasTrampoline() || !g_register_hook.HasTrampoline())
		{
			wf::Log("WARN: using fallback hook mode (exec=%d find=%d register=%d)", g_exec_hook.HasTrampoline(), g_find_hook.HasTrampoline(), g_register_hook.HasTrampoline());
		}

		return ok;
	}

	void Remove()
	{
		g_exec_hook.Remove();
		g_find_hook.Remove();
		g_register_hook.Remove();
	}

	int FindPublic(AMX *amx, const char *name, int *index)
	{
		if (!g_find_hook.Installed())
		{
			return amx::FindPublic(amx, name, index);
		}

		g_internal_find = true;
		int err = CallFind(amx, name, index);
		g_internal_find = false;
		return err;
	}

	int Exec(AMX *amx, cell *retval, int index)
	{
		if (!g_exec_hook.Installed())
		{
			return amx::Exec(amx, retval, index);
		}

		// The original is reached through the detour, which recognises the call.
		g_internal_exec.amx = amx;
		g_internal_exec.index = index;
		return amx::Exec(amx, retval, index);
	}

	int Register(AMX *amx, const AMX_NATIVE_INFO *list, int number)
	{
		return amx::Register(amx, list, number);
	}

	void OnAmxUnload(AMX *amx)
	{
		g_public_map.erase(amx);

		for (Event &e : g_events)
		{
			for (int i = 0; i < e.seen_count; ++i)
			{
				if (e.seen[i] == amx)
				{
					e.seen[i] = nullptr;
				}
			}
		}
	}

	void NextFrame()
	{
		++g_frame;
	}
}
