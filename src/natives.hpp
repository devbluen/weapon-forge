#pragma once

#include "sdk/amx.h"

namespace wf
{
	int RegisterNatives(AMX *amx);
	// The plugin's replacement for a server native, or nullptr
	AMX_NATIVE NativeOverride(const char *name);
}
