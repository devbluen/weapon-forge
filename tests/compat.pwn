// Compatibility test: streamer (its bundled sampgdk hooks amx_Exec and
// amx_FindPublic too) and YSI y_hooks (rewrites the public table at runtime).
// The gamemode has no OnPlayerRequestClass, OnPlayerStateChange or
// OnPlayerUpdate, so the plugin has to raise them itself (this used to crash
// streamer), and its callbacks are y_hooks hooks.
//
// Needs the streamer and YSI includes, streamer loaded as a plugin
// (legacy_plugins on open.mp) and, on SA-MP, "maxnpc 1" with
// tests/npc_idle.pwn compiled into npcmodes/.
// Prints "[compat] PASS" / "[compat] FAIL" lines and shuts the server down.

#define YSI_NO_HEAP_MALLOC
#define YSI_NO_VERSION_CHECK
#define YSI_NO_OPTIMISATION_MESSAGE

#if defined TEST_OPEN_MP
	#include <open.mp>
#else
	#include <a_samp>
#endif

#include <weapon-forge>
#include <streamer>
#include <YSI_Coding\y_hooks>

new g_Npc = INVALID_PLAYER_ID;
new g_Fails, g_Spawns, g_HookedConnect, g_HookedFinished, g_PublicFinished;

forward CT_Step();
forward CT_Timeout();

main() {}

Check(bool:ok, const what[])
{
	if (!ok) {
		g_Fails++;
	}
	printf("[compat] %s %s", ok ? ("PASS") : ("FAIL"), what);
}

Finish()
{
	Check(g_HookedConnect == 1, "y_hooks OnPlayerConnect called once");
	Check(g_HookedFinished == 1 && g_PublicFinished == 1, "OnPlayerDeathFinished reached the hook and the public");
	printf("[compat] DONE fails=%d", g_Fails);
	SendRconCommand("exit");
}

hook OnGameModeInit()
{
	AddPlayerClass(0, 1958.33, 1343.12, 15.36, 269.15, WEAPON_DEAGLE, 50, WEAPON_UNARMED, 0, WEAPON_UNARMED, 0);
	SetRespawnTime(1500);
	CreateDynamicPickup(1240, 2, 1958.33, 1345.12, 15.36);

	#if defined TEST_OPEN_MP
		NPC_Spawn(NPC_Create("wf_bot"));
	#else
		ConnectNPC("wf_bot", "npc_idle");
	#endif
	SetTimer("CT_Timeout", 30000, false);
	return 1;
}

hook OnPlayerConnect(playerid)
{
	if (IsPlayerNPC(playerid)) {
		g_Npc = playerid;
		g_HookedConnect++;
	}
	return 1;
}

hook OnPlayerSpawn(playerid)
{
	if (playerid != g_Npc) {
		return 1;
	}

	if (++g_Spawns == 1) {
		SetTimer("CT_Step", 1500, false);
	} else {
		Check(WF_GetPlayerHealth(g_Npc) == 100.0, "health restored after respawn");
		Finish();
	}
	return 1;
}

public CT_Step()
{
	// Only works if the plugin recognised OnPlayerConnect after y_hooks rewrote the publics
	WF_SetPlayerHealth(g_Npc, 77.0);
	WF_SetPlayerArmour(g_Npc, 40.0);
	Check(WF_GetPlayerHealth(g_Npc) == 77.0, "WF_SetPlayerHealth");
	Check(WF_GetPlayerArmour(g_Npc) == 40.0, "WF_SetPlayerArmour");
	Check(WF_EnableHealthBarForPlayer(g_Npc, true) == 1, "health bar accepted for the player");

	#if defined TEST_OPEN_MP
		// A script destroying stale ids must not remove the plugin's textdraws
		new before;
		for (new i; i < 256; i++) {
			if (IsValidPlayerTextDraw(g_Npc, PlayerText:i)) before++;
		}
		for (new i; i < 256; i++) {
			PlayerTextDrawHide(g_Npc, PlayerText:i);
			PlayerTextDrawDestroy(g_Npc, PlayerText:i);
		}
		new after;
		for (new i; i < 256; i++) {
			if (IsValidPlayerTextDraw(g_Npc, PlayerText:i)) after++;
		}
		Check(before >= 3 && after == before, "health bar survives scripts destroying stale textdraw ids");
	#endif

	Check(WF_KillPlayer(g_Npc, INVALID_PLAYER_ID, WEAPON_DEAGLE) == 1, "WF_KillPlayer");
	return 1;
}

hook OnPlayerDeathFinished(playerid, bool:cancelable)
{
	g_HookedFinished++;
	return 1;
}

public OnPlayerDeathFinished(playerid, bool:cancelable)
{
	g_PublicFinished++;

	#if defined TEST_OPEN_MP
		// open.mp NPCs are respawned through the NPC API
		if (cancelable && IsPlayerNPC(playerid)) {
			NPC_Respawn(playerid);
		}
	#endif
	return 1;
}

public CT_Timeout()
{
	Check(false, "timeout (a step did not complete)");
	Finish();
	return 1;
}
