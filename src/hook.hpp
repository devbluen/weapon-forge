#pragma once

#include <cstddef>
#include <cstdint>

// Minimal x86 (32-bit) inline function hook.
//
// The first instructions of the target are copied into an executable
// trampoline (relocating rel32 calls/jumps) and replaced by a jump to the
// hook. When the prologue can't be decoded, the hook falls back to "toggle"
// mode: Call() temporarily restores the original bytes.
class Hook
{
public:
	Hook() = default;
	~Hook();
	Hook(const Hook &) = delete;
	Hook &operator=(const Hook &) = delete;

	bool Install(void *target, void *detour);
	void Remove();

	bool Installed() const { return target_ != nullptr; }
	bool HasTrampoline() const { return trampoline_ != nullptr; }

	// Address to call the original function through.
	// In toggle mode this is the target itself (see Unpatched).
	void *Original() const { return trampoline_ ? trampoline_ : target_; }

	// RAII guard used by callers in toggle mode.
	class Unpatched
	{
	public:
		explicit Unpatched(Hook &hook);
		~Unpatched();

	private:
		Hook &hook_;
		bool active_;
	};

private:
	void Patch();
	void Unpatch();

	unsigned char *target_ = nullptr;
	unsigned char *trampoline_ = nullptr;
	unsigned char original_[16] = {};
	unsigned char jump_[5] = {};
	size_t patch_size_ = 0;
	bool patched_ = false;
};

// Length of the x86 instruction at `code`, or 0 if unsupported.
size_t X86InstructionLength(const unsigned char *code);
