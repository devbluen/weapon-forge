#include "amx_api.hpp"

#include "sdk/plugincommon.h"

namespace
{
	void **g_exports = nullptr;

	template <typename Fn>
	Fn Get(int index)
	{
		return reinterpret_cast<Fn>(g_exports[index]);
	}
}

namespace amx
{
	void Init(void **exports)
	{
		g_exports = exports;
	}

	int Allot(AMX *amx, int cells, cell *amx_addr, cell **phys_addr)
	{
		using Fn = int (*)(AMX *, int, cell *, cell **);
		return Get<Fn>(PLUGIN_AMX_EXPORT_Allot)(amx, cells, amx_addr, phys_addr);
	}

	int Release(AMX *amx, cell amx_addr)
	{
		using Fn = int (*)(AMX *, cell);
		return Get<Fn>(PLUGIN_AMX_EXPORT_Release)(amx, amx_addr);
	}

	int Exec(AMX *amx, cell *retval, int index)
	{
		using Fn = int (*)(AMX *, cell *, int);
		return Get<Fn>(PLUGIN_AMX_EXPORT_Exec)(amx, retval, index);
	}

	int FindPublic(AMX *amx, const char *name, int *index)
	{
		using Fn = int (*)(AMX *, const char *, int *);
		return Get<Fn>(PLUGIN_AMX_EXPORT_FindPublic)(amx, name, index);
	}

	int Push(AMX *amx, cell value)
	{
		using Fn = int (*)(AMX *, cell);
		return Get<Fn>(PLUGIN_AMX_EXPORT_Push)(amx, value);
	}

	int GetAddr(AMX *amx, cell amx_addr, cell **phys_addr)
	{
		using Fn = int (*)(AMX *, cell, cell **);
		return Get<Fn>(PLUGIN_AMX_EXPORT_GetAddr)(amx, amx_addr, phys_addr);
	}

	int StrLen(const cell *cstring, int *length)
	{
		using Fn = int (*)(const cell *, int *);
		return Get<Fn>(PLUGIN_AMX_EXPORT_StrLen)(cstring, length);
	}

	int GetString(char *dest, const cell *source, int use_wchar, size_t size)
	{
		using Fn = int (*)(char *, const cell *, int, size_t);
		return Get<Fn>(PLUGIN_AMX_EXPORT_GetString)(dest, source, use_wchar, size);
	}

	int SetString(cell *dest, const char *source, int pack, int use_wchar, size_t size)
	{
		using Fn = int (*)(cell *, const char *, int, int, size_t);
		return Get<Fn>(PLUGIN_AMX_EXPORT_SetString)(dest, source, pack, use_wchar, size);
	}

	int Register(AMX *amx, const AMX_NATIVE_INFO *list, int number)
	{
		using Fn = int (*)(AMX *, const AMX_NATIVE_INFO *, int);
		return Get<Fn>(PLUGIN_AMX_EXPORT_Register)(amx, list, number);
	}

	int RaiseError(AMX *amx, int error)
	{
		using Fn = int (*)(AMX *, int);
		return Get<Fn>(PLUGIN_AMX_EXPORT_RaiseError)(amx, error);
	}

	cell *Addr(AMX *amx, cell amx_addr)
	{
		cell *phys = nullptr;
		if (GetAddr(amx, amx_addr, &phys) != AMX_ERR_NONE)
		{
			return nullptr;
		}
		return phys;
	}

	std::string ReadString(AMX *amx, cell amx_addr)
	{
		cell *phys = Addr(amx, amx_addr);
		if (!phys)
		{
			return {};
		}

		int len = 0;
		StrLen(phys, &len);
		if (len <= 0)
		{
			return {};
		}

		std::string out(static_cast<size_t>(len) + 1, '\0');
		GetString(&out[0], phys, 0, out.size());
		out.resize(static_cast<size_t>(len));
		return out;
	}

	void WriteString(AMX *amx, cell amx_addr, const char *str, size_t maxlen)
	{
		if (maxlen == 0)
		{
			return;
		}

		cell *phys = Addr(amx, amx_addr);
		if (phys)
		{
			SetString(phys, str, 0, 0, maxlen);
		}
	}

	Heap::~Heap()
	{
		if (used_)
		{
			Release(amx_, first_);
		}
	}

	cell Heap::Alloc(int cells, cell **phys)
	{
		cell addr = 0;
		cell *ptr = nullptr;

		if (Allot(amx_, cells, &addr, &ptr) != AMX_ERR_NONE)
		{
			return 0;
		}

		if (!used_)
		{
			first_ = addr;
			used_ = true;
		}

		if (phys)
		{
			*phys = ptr;
		}

		return addr;
	}

	cell Heap::String(const char *str)
	{
		size_t len = std::strlen(str) + 1;
		cell *phys = nullptr;
		cell addr = Alloc(static_cast<int>(len), &phys);

		if (phys)
		{
			SetString(phys, str, 0, 0, len);
		}

		return addr;
	}
}
