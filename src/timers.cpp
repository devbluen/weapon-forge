#include "timers.hpp"

#include "state.hpp"

#include <queue>
#include <vector>

namespace wf
{
	void OnTimer(timers::Type type, int id, const timers::Args &args);
}

namespace timers
{
	namespace
	{
		constexpr int kMaxIds = 2001;

		struct Entry
		{
			uint32_t at;
			uint64_t seq;
			Type type;
			int id;
			uint32_t serial;
			Args args;
		};

		struct Later
		{
			bool operator()(const Entry &a, const Entry &b) const
			{
				int32_t d = static_cast<int32_t>(a.at - b.at);
				if (d != 0)
				{
					return d > 0;
				}
				return a.seq > b.seq;
			}
		};

		std::priority_queue<Entry, std::vector<Entry>, Later> g_queue;
		uint32_t g_serial[TYPE_COUNT][kMaxIds] = {};
		bool g_active[TYPE_COUNT][kMaxIds] = {};
		uint64_t g_seq = 0;

		bool ValidId(int id)
		{
			return id >= 0 && id < kMaxIds;
		}
	}

	void Start(Type type, int id, int delay_ms, Args args)
	{
		if (!ValidId(id))
		{
			return;
		}

		if (delay_ms < 0)
		{
			delay_ms = 0;
		}

		uint32_t serial = ++g_serial[type][id];
		g_active[type][id] = true;
		g_queue.push({wf::Tick() + static_cast<uint32_t>(delay_ms), g_seq++, type, id, serial, args});
	}

	void Stop(Type type, int id)
	{
		if (!ValidId(id))
		{
			return;
		}

		++g_serial[type][id];
		g_active[type][id] = false;
	}

	bool Active(Type type, int id)
	{
		return ValidId(id) && g_active[type][id];
	}

	void StopAll(int playerid)
	{
		for (int t = 0; t < TYPE_COUNT; ++t)
		{
			if (t == VEHICLE_KILL || t == VEHICLE_RESPAWN || t == DISCONNECT_CLEANUP)
			{
				continue;
			}
			Stop(static_cast<Type>(t), playerid);
		}
	}

	void Clear()
	{
		g_queue = {};

		for (int t = 0; t < TYPE_COUNT; ++t)
		{
			for (int i = 0; i < kMaxIds; ++i)
			{
				++g_serial[t][i];
				g_active[t][i] = false;
			}
		}
	}

	void Process(uint32_t now)
	{
		while (!g_queue.empty())
		{
			const Entry &top = g_queue.top();

			if (static_cast<int32_t>(now - top.at) < 0)
			{
				break;
			}

			Entry e = top;
			g_queue.pop();

			if (g_serial[e.type][e.id] != e.serial)
			{
				continue;
			}

			g_active[e.type][e.id] = false;
			wf::OnTimer(e.type, e.id, e.args);
		}
	}
}
