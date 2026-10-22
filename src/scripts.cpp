#include "scripts.hpp"

#include "amx_api.hpp"
#include "amxhooks.hpp"
#include "server.hpp"

#include <algorithm>
#include <unordered_map>

namespace scripts
{
	namespace
	{
		struct Entry
		{
			AMX *amx;
			std::unordered_map<std::string, int> publics;
		};

		std::vector<Entry> g_entries;
		AMX *g_gamemode = nullptr;

		Entry *Find(AMX *amx)
		{
			for (auto &e : g_entries)
			{
				if (e.amx == amx)
				{
					return &e;
				}
			}
			return nullptr;
		}

		int PublicIndex(Entry &e, const char *name)
		{
			auto it = e.publics.find(name);

			if (it != e.publics.end())
			{
				return it->second;
			}

			int index = -1;

			if (amxhooks::FindPublic(e.amx, name, &index) != AMX_ERR_NONE)
			{
				index = -1;
			}

			e.publics.emplace(name, index);
			return index;
		}

		void UpdateContext()
		{
			server::SetContext(AnyContext());
		}
	}

	void OnLoad(AMX *amx)
	{
		if (!Find(amx))
		{
			g_entries.push_back({amx, {}});
		}

		UpdateContext();
	}

	void OnUnload(AMX *amx)
	{
		g_entries.erase(std::remove_if(g_entries.begin(), g_entries.end(), [amx](const Entry &e)
			{ return e.amx == amx; }),
			g_entries.end());

		if (g_gamemode == amx)
		{
			g_gamemode = nullptr;
		}

		UpdateContext();
	}

	void SetGameMode(AMX *amx)
	{
		if (Find(amx))
		{
			g_gamemode = amx;
			UpdateContext();
		}
	}

	AMX *GameMode()
	{
		return g_gamemode;
	}

	AMX *AnyContext()
	{
		if (g_gamemode)
		{
			return g_gamemode;
		}

		return g_entries.empty() ? nullptr : g_entries.front().amx;
	}

	Call &Call::Int(cell value)
	{
		Arg a{};
		a.type = Arg::Type::Value;
		a.value = value;
		args_.push_back(std::move(a));
		return *this;
	}

	Call &Call::Float(float value)
	{
		return Int(amx_ftoc(value));
	}

	Call &Call::Ref(cell &value)
	{
		Arg a{};
		a.type = Arg::Type::Ref;
		a.ref = &value;
		args_.push_back(std::move(a));
		return *this;
	}

	Call &Call::RefFloat(float &value)
	{
		Arg a{};
		a.type = Arg::Type::Ref;
		a.ref_float = &value;
		args_.push_back(std::move(a));
		return *this;
	}

	Call &Call::Array(cell *data, int size, bool copy_back)
	{
		Arg a{};
		a.type = Arg::Type::Array;
		a.array = data;
		a.size = size;
		a.copy_back = copy_back;
		args_.push_back(std::move(a));
		return *this;
	}

	Call &Call::String(const char *str, int size, std::string *out)
	{
		Arg a{};
		a.type = Arg::Type::String;
		a.str = str ? str : "";
		a.size = std::max<int>(size, static_cast<int>(a.str.size()) + 1);
		a.str_out = out;
		args_.push_back(std::move(a));
		return *this;
	}

	bool Call::ExecIn(AMX *amx, cell &retval)
	{
		Entry *e = Find(amx);

		if (!e)
		{
			return false;
		}

		int index = PublicIndex(*e, name_);

		if (index < 0)
		{
			return false;
		}

		std::vector<cell *> slots(args_.size(), nullptr);
		amx::Heap heap(amx);

		// Allocate everything first: nothing is pushed if the heap is full
		for (size_t i = 0; i < args_.size(); ++i)
		{
			Arg &a = args_[i];

			if (a.type == Arg::Type::Value)
			{
				continue;
			}

			int cells = a.type == Arg::Type::Ref ? 1 : a.size;
			cell *phys = nullptr;
			cell addr = heap.Alloc(cells, &phys);

			if (!phys)
			{
				return false;
			}

			switch (a.type)
			{
			case Arg::Type::Ref:
				*phys = a.ref_float ? amx_ftoc(*a.ref_float) : *a.ref;
				break;

			case Arg::Type::Array:
				std::copy(a.array, a.array + a.size, phys);
				break;

			case Arg::Type::String:
				std::fill(phys, phys + a.size, 0);
				amx::SetString(phys, a.str.c_str(), 0, 0, static_cast<size_t>(a.size));
				break;

			default:
				break;
			}

			slots[i] = phys;
			a.value = addr;
		}

		for (size_t i = args_.size(); i-- > 0;)
		{
			amx::Push(amx, args_[i].value);
		}

		retval = 0;
		amxhooks::Exec(amx, &retval, index);

		for (size_t i = 0; i < args_.size(); ++i)
		{
			Arg &a = args_[i];
			cell *phys = slots[i];

			if (!phys)
			{
				continue;
			}

			switch (a.type)
			{
			case Arg::Type::Ref:
				if (a.ref_float)
				{
					*a.ref_float = amx_ctof(*phys);
				}
				else
				{
					*a.ref = *phys;
				}
				break;

			case Arg::Type::Array:
				if (a.copy_back)
				{
					std::copy(phys, phys + a.size, a.array);
				}
				break;

			case Arg::Type::String:
			{
				phys[a.size - 1] = 0;
				std::string tmp(static_cast<size_t>(a.size), '\0');
				amx::GetString(&tmp[0], phys, 0, tmp.size());
				tmp.resize(std::strlen(tmp.c_str()));
				a.str = tmp;

				if (a.str_out)
				{
					*a.str_out = tmp;
				}
				break;
			}

			default:
				break;
			}
		}

		return true;
	}

	cell Call::Exec(cell fallback, bool stop_on_zero)
	{
		// Copy the order: a callback may load or unload scripts
		std::vector<AMX *> order;

		for (auto &e : g_entries)
		{
			if (e.amx != g_gamemode)
			{
				order.push_back(e.amx);
			}
		}

		if (g_gamemode)
		{
			order.push_back(g_gamemode);
		}

		cell result = fallback;
		bool any = false;

		for (AMX *amx : order)
		{
			cell ret = 0;

			if (!ExecIn(amx, ret))
			{
				continue;
			}

			if (!any)
			{
				any = true;
				result = ret;
			}
			else if (ret == 0)
			{
				result = 0;
			}

			if (stop_on_zero && ret == 0)
			{
				break;
			}
		}

		return result;
	}
}
