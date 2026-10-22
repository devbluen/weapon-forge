#include "hook.hpp"

#include <cstring>

#if defined(_WIN32)
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#include <windows.h>
#else
	#include <sys/mman.h>
	#include <unistd.h>
#endif

namespace
{
	bool MakeWritable(void *addr, size_t size)
	{
#if defined(_WIN32)
		DWORD old = 0;
		return VirtualProtect(addr, size, PAGE_EXECUTE_READWRITE, &old) != 0;
#else
		long page = sysconf(_SC_PAGESIZE);
		uintptr_t start = reinterpret_cast<uintptr_t>(addr) & ~static_cast<uintptr_t>(page - 1);
		uintptr_t end = reinterpret_cast<uintptr_t>(addr) + size;
		return mprotect(reinterpret_cast<void *>(start), end - start, PROT_READ | PROT_WRITE | PROT_EXEC) == 0;
#endif
	}

	unsigned char *AllocExec(size_t size)
	{
#if defined(_WIN32)
		return static_cast<unsigned char *>(VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
#else
		void *p = mmap(nullptr, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		return p == MAP_FAILED ? nullptr : static_cast<unsigned char *>(p);
#endif
	}

	void FreeExec(unsigned char *p, size_t size)
	{
#if defined(_WIN32)
		(void)size;
		VirtualFree(p, 0, MEM_RELEASE);
#else
		munmap(p, size);
#endif
	}

	void WriteJump(unsigned char *at, const void *to)
	{
		at[0] = 0xE9;
		int32_t rel = static_cast<int32_t>(reinterpret_cast<intptr_t>(to) - reinterpret_cast<intptr_t>(at + 5));
		std::memcpy(at + 1, &rel, sizeof(rel));
	}

	constexpr size_t kTrampolineSize = 64;

	// ModRM (+SIB +displacement) length
	size_t ModRMLength(const unsigned char *p)
	{
		unsigned char modrm = p[0];
		unsigned char mod = modrm >> 6;
		unsigned char rm = modrm & 7;
		size_t len = 1;

		if (mod != 3 && rm == 4)
		{
			unsigned char sib = p[1];
			len += 1;

			if (mod == 0 && (sib & 7) == 5)
			{
				len += 4;
			}
		}

		if (mod == 0 && rm == 5)
		{
			len += 4;
		}
		else if (mod == 1)
		{
			len += 1;
		}
		else if (mod == 2)
		{
			len += 4;
		}

		return len;
	}
}

size_t X86InstructionLength(const unsigned char *code)
{
	const unsigned char *p = code;
	size_t prefix = 0;

	// endbr32
	if (p[0] == 0xF3 && p[1] == 0x0F && p[2] == 0x1E && p[3] == 0xFB)
	{
		return 4;
	}

	// Segment / lock prefixes
	while (p[0] == 0x64 || p[0] == 0x65 || p[0] == 0x2E || p[0] == 0x3E || p[0] == 0xF0)
	{
		++p;
		++prefix;
	}

	unsigned char op = p[0];

	if ((op >= 0x50 && op <= 0x5F) || op == 0x90 || op == 0xCC || op == 0xC3 || op == 0x9C || op == 0x9D || op == 0x60 || op == 0x61)
	{
		return prefix + 1;
	}

	if (op >= 0xB8 && op <= 0xBF)
	{
		return prefix + 5;
	}

	switch (op)
	{
	case 0x6A: // push imm8
		return prefix + 2;
	case 0x68: // push imm32
	case 0xA1: // mov eax, moffs32
	case 0xA3: // mov moffs32, eax
	case 0xE8: // call rel32
	case 0xE9: // jmp rel32
	case 0x3D: // cmp eax, imm32
	case 0x2D: // sub eax, imm32
	case 0x05: // add eax, imm32
	case 0x25: // and eax, imm32
		return prefix + 5;
	case 0x01: // add r/m, r
	case 0x03:
	case 0x09:
	case 0x0B:
	case 0x21:
	case 0x23:
	case 0x29:
	case 0x2B:
	case 0x31:
	case 0x33:
	case 0x39:
	case 0x3B:
	case 0x84:
	case 0x85:
	case 0x88:
	case 0x89:
	case 0x8A:
	case 0x8B:
	case 0x8D:
	case 0xFF:
		return prefix + 1 + ModRMLength(p + 1);
	case 0x80: // op r/m8, imm8
	case 0x83: // op r/m32, imm8
	case 0xC6: // mov r/m8, imm8
		return prefix + 1 + ModRMLength(p + 1) + 1;
	case 0x81: // op r/m32, imm32
	case 0xC7: // mov r/m32, imm32
		return prefix + 1 + ModRMLength(p + 1) + 4;
	case 0x0F:
		// movzx / movsx
		if (p[1] == 0xB6 || p[1] == 0xB7 || p[1] == 0xBE || p[1] == 0xBF)
		{
			return prefix + 2 + ModRMLength(p + 2);
		}
		return 0;
	default:
		return 0;
	}
}

Hook::~Hook()
{
	Remove();
}

bool Hook::Install(void *target, void *detour)
{
	if (target_ || !target || !detour)
	{
		return false;
	}

	auto *code = static_cast<unsigned char *>(target);

	// Decode whole instructions covering the 5-byte jump
	size_t size = 0;
	bool decodable = true;

	while (size < 5)
	{
		size_t len = X86InstructionLength(code + size);

		if (!len)
		{
			decodable = false;
			break;
		}

		size += len;
	}

	if (!MakeWritable(code, 16))
	{
		return false;
	}

	target_ = code;
	std::memcpy(original_, code, sizeof(original_));

	if (decodable && size <= 15)
	{
		trampoline_ = AllocExec(kTrampolineSize);
	}

	if (trampoline_)
	{
		std::memcpy(trampoline_, code, size);

		// Relocate rel32 calls/jumps copied into the trampoline
		for (size_t off = 0; off < size;)
		{
			size_t len = X86InstructionLength(code + off);

			if ((code[off] == 0xE8 || code[off] == 0xE9) && len == 5)
			{
				int32_t rel;
				std::memcpy(&rel, code + off + 1, sizeof(rel));
				intptr_t dest = reinterpret_cast<intptr_t>(code + off + 5) + rel;
				int32_t new_rel = static_cast<int32_t>(dest - reinterpret_cast<intptr_t>(trampoline_ + off + 5));
				std::memcpy(trampoline_ + off + 1, &new_rel, sizeof(new_rel));
			}

			off += len;
		}

		WriteJump(trampoline_ + size, code + size);
		patch_size_ = size;
	}
	else
	{
		patch_size_ = 5;
	}

	WriteJump(jump_, nullptr);
	int32_t rel = static_cast<int32_t>(reinterpret_cast<intptr_t>(detour) - reinterpret_cast<intptr_t>(code + 5));
	std::memcpy(jump_ + 1, &rel, sizeof(rel));

	Patch();
	return true;
}

void Hook::Patch()
{
	std::memcpy(target_, jump_, 5);

	for (size_t i = 5; i < patch_size_; ++i)
	{
		target_[i] = 0x90;
	}

	patched_ = true;
}

void Hook::Unpatch()
{
	std::memcpy(target_, original_, patch_size_);
	patched_ = false;
}

void Hook::Remove()
{
	if (!target_)
	{
		return;
	}

	Unpatch();

	if (trampoline_)
	{
		FreeExec(trampoline_, kTrampolineSize);
		trampoline_ = nullptr;
	}

	target_ = nullptr;
}

Hook::Unpatched::Unpatched(Hook &hook) : hook_(hook), active_(!hook.trampoline_ && hook.patched_)
{
	if (active_)
	{
		hook_.Unpatch();
	}
}

Hook::Unpatched::~Unpatched()
{
	if (active_ && hook_.target_)
	{
		hook_.Patch();
	}
}
