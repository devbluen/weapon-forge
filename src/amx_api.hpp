#pragma once

#include "sdk/amx.h"

#include <string>

// Thin wrappers over the AMX function table handed to the plugin by the
// server (PLUGIN_DATA_AMX_EXPORTS). Both SA-MP and open.mp provide it.
namespace amx
{
	void Init(void **exports);

	int Allot(AMX *amx, int cells, cell *amx_addr, cell **phys_addr);
	int Release(AMX *amx, cell amx_addr);
	int Exec(AMX *amx, cell *retval, int index);
	int FindPublic(AMX *amx, const char *name, int *index);
	int Push(AMX *amx, cell value);
	int GetAddr(AMX *amx, cell amx_addr, cell **phys_addr);
	int StrLen(const cell *cstring, int *length);
	int GetString(char *dest, const cell *source, int use_wchar, size_t size);
	int SetString(cell *dest, const char *source, int pack, int use_wchar, size_t size);
	int Register(AMX *amx, const AMX_NATIVE_INFO *list, int number);
	int RaiseError(AMX *amx, int error);

	// Helpers
	cell *Addr(AMX *amx, cell amx_addr);
	std::string ReadString(AMX *amx, cell amx_addr);
	void WriteString(AMX *amx, cell amx_addr, const char *str, size_t maxlen);

	// Allocates cells on an AMX heap and frees everything on destruction.
	class Heap
	{
	public:
		explicit Heap(AMX *amx) : amx_(amx) {}
		~Heap();
		Heap(const Heap &) = delete;
		Heap &operator=(const Heap &) = delete;

		cell Alloc(int cells, cell **phys = nullptr);
		cell String(const char *str);

	private:
		AMX *amx_;
		cell first_ = 0;
		bool used_ = false;
	};
}
