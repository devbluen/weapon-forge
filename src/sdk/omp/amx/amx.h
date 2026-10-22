// Shim for the open.mp SDK, which includes <amx/amx.h> from open.mp's Pawn
// repository. The plugin's own ABI-compatible definitions are enough.
#pragma once

#include "../../amx.h"

#ifndef AMXAPI
	#define AMXAPI
#endif

#ifndef AMX_NATIVE_CALL
	#define AMX_NATIVE_CALL
#endif
