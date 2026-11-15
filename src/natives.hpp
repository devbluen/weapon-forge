#pragma once

#include "sdk/amx.h"

namespace wf
{
	int RegisterNatives(AMX *amx);
	// The plugin's replacement for a server native, or nullptr. `original` is
	// the server's implementation, kept for the replacements that forward to it.
	AMX_NATIVE NativeOverride(const char *name, AMX_NATIVE original);
}
