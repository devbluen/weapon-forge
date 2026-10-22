// Compile test: filterscript (not the master script).

#define FILTERSCRIPT

#if defined TEST_OPEN_MP
	#include <open.mp>
#else
	#include <a_samp>
#endif

#include <weapon-forge>

public OnFilterScriptInit()
{
	print("[test] weapon-forge filterscript loaded");
	return 1;
}

public OnPlayerDeath(playerid, killerid, WEAPON:reason)
{
	new Float:health;
	GetPlayerHealth(playerid, health);
	printf("[test fs] %d died (killer %d, reason %d, health %.1f)", playerid, killerid, _:reason, health);
	return 1;
}
