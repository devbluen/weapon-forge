#pragma once

#include "sdk/amx.h"

// Startup shared by the SA-MP plugin and the open.mp component entry points
namespace wf
{
	using Logger = void (*)(const char *format, ...);

	bool Startup(void **exports, Logger logger, bool component);
	void Shutdown();
	int AmxLoad(AMX *amx);
	int AmxUnload(AMX *amx);
	void ServerTick();
}
