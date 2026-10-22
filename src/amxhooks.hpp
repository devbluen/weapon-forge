#pragma once

#include "sdk/amx.h"

// Hooks on the server's AMX functions (shared by SA-MP and open.mp):
// - amx_Register: captures the server natives and replaces the ones the
//   plugin manages (SetPlayerHealth, GetPlayerHealth, ...) in every script
// - amx_FindPublic: reports the callbacks the plugin needs as present, so
//   the server always raises them even if no script implements them
// - amx_Exec: processes server callbacks before the scripts see them
namespace amxhooks
{
	bool Install(void **exports);
	void Remove();

	// Marks the next amx_FindPublic/amx_Exec made by the plugin itself, so
	// they bypass the hooks.
	int FindPublic(AMX *amx, const char *name, int *index);
	int Exec(AMX *amx, cell *retval, int index);
	int Register(AMX *amx, const AMX_NATIVE_INFO *list, int number);

	void OnAmxUnload(AMX *amx);
	// Called once per server tick: separates events of different frames.
	void NextFrame();
}
