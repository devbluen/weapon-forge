// Runtime test: connects an NPC and runs the damage/death/respawn pipeline.
// Needs "maxnpc 1" and tests/npc_idle.pwn compiled into npcmodes/.
// Prints "[rt] PASS" / "[rt] FAIL" lines and shuts the server down.

#if defined TEST_OPEN_MP
	#include <open.mp>
#else
	#include <a_samp>
#endif

#include <weapon-forge>

new g_Npc = INVALID_PLAYER_ID;
new g_Step;
new g_Fails;
new g_DamageCalls, g_DamageDoneCalls, g_PrepareCalls, g_DeathCalls, g_FinishedCalls, g_Spawns;
new g_LastDeathReason = -1, g_LastDeathKiller = -2;

forward RT_Step();
forward RT_Timeout();

main() {}

Check(bool:ok, const what[])
{
	if (ok) {
		printf("[rt] PASS %s", what);
	} else {
		printf("[rt] FAIL %s", what);
		g_Fails++;
	}
}

Finish()
{
	Check(WF_IsRakNetActive(), "sync packet processing active");

	printf("[rt] DONE fails=%d raknet=%d", g_Fails, WF_IsRakNetActive());
	SendRconCommand("exit");
}

public OnGameModeInit()
{
	new version[16];
	WF_GetVersion(version);
	printf("[rt] weapon-forge %s open.mp=%d", version, WF_IsOpenMP());

	AddPlayerClass(0, 1958.33, 1343.12, 15.36, 269.15, WEAPON_DEAGLE, 50, WEAPON_UNARMED, 0, WEAPON_UNARMED, 0);
	SetRespawnTime(1500);
	SetDamageSounds(0, 0);
	WF_SetSpawnProtection(0);

	Check(GetWeaponDamage(WEAPON_DEAGLE) == 46.2, "default deagle damage");
	SetWeaponDamage(WEAPON_DEAGLE, DAMAGE_TYPE_STATIC, 50.0);
	Check(GetWeaponDamage(WEAPON_DEAGLE) == 50.0, "SetWeaponDamage");
	Check(GetWeaponShootRate(WEAPON_M4) == 90, "default M4 shoot rate");
	Check(IsBulletWeapon(WEAPON_M4) && !IsBulletWeapon(WEAPON_KNIFE) && IsMeleeWeapon(WEAPON_KNIFE), "weapon classes");

	new name[32];
	GetWeaponName(WEAPON_PISTOLWHIP, name);
	Check(!strcmp(name, "Pistol whip"), "custom weapon names");

	#if defined _INC_open_mp
		NPC_Spawn(NPC_Create("wf_bot"));
	#else
		ConnectNPC("wf_bot", "npc_idle");
	#endif
	SetTimer("RT_Timeout", 30000, false);
	return 1;
}

public RT_Timeout()
{
	print("[rt] FAIL timeout (step did not complete)");
	g_Fails++;
	Finish();
	return 1;
}

public OnPlayerConnect(playerid)
{
	if (IsPlayerNPC(playerid)) {
		g_Npc = playerid;
		printf("[rt] npc connected: %d", playerid);
	}
	return 1;
}

public OnPlayerRequestClass(playerid, classid)
{
	SetSpawnInfo(playerid, NO_TEAM, 0, 1958.33, 1343.12, 15.36, 269.15, WEAPON_DEAGLE, 50, WEAPON_UNARMED, 0, WEAPON_UNARMED, 0);
	return 1;
}

public OnPlayerSpawn(playerid)
{
	if (playerid != g_Npc) {
		return 1;
	}

	g_Spawns++;
	printf("[rt] npc spawned (%d)", g_Spawns);

	if (g_Spawns == 1) {
		g_Step = 0;
		SetTimer("RT_Step", 1000, false);
	} else if (g_Spawns == 2) {
		new Float:health;
		GetPlayerHealth(playerid, health);
		Check(health == 100.0, "health restored after respawn");
		Check(!IsPlayerDying(playerid), "not dying after respawn");
		Check(g_FinishedCalls == 1, "OnPlayerDeathFinished called once");
		Check(WF_GetPlayerStat(playerid, WF_STAT_DEATHS) == 1, "death statistic");
		Finish();
	}
	return 1;
}

public RT_Step()
{
	new Float:health, Float:armour;

	switch (g_Step++) {
		case 0: {
			GetPlayerHealth(g_Npc, health);
			Check(health == 100.0, "initial server-side health");
			Check(WF_IsPlayerSpawned(g_Npc), "npc is spawned");

			DamagePlayer(g_Npc, 30.0, INVALID_PLAYER_ID, WEAPON_COLT45, 3);
			GetPlayerHealth(g_Npc, health);
			Check(health == 70.0, "DamagePlayer removes health");
			Check(g_DamageCalls == 1 && g_DamageDoneCalls == 1, "OnPlayerDamage/OnPlayerDamageDone called");
			Check(GetLastDamageHealth(g_Npc) == 30.0, "GetLastDamageHealth");
		}
		case 1: {
			SetPlayerArmour(g_Npc, 50.0);
			DamagePlayer(g_Npc, 60.0, INVALID_PLAYER_ID, WEAPON_M4, 3);
			GetPlayerHealth(g_Npc, health);
			GetPlayerArmour(g_Npc, armour);
			Check(armour == 0.0 && health == 60.0, "armour absorbs damage first");

			WF_SetPlayerGodMode(g_Npc, true);
			DamagePlayer(g_Npc, 10.0, INVALID_PLAYER_ID, WEAPON_M4, 3);
			GetPlayerHealth(g_Npc, health);
			Check(health == 60.0, "god mode blocks damage");
			WF_SetPlayerGodMode(g_Npc, false);

			WF_SetPlayerDamageMultiplier(g_Npc, 1.0, 0.5);
			DamagePlayer(g_Npc, 20.0, INVALID_PLAYER_ID, WEAPON_M4, 3);
			GetPlayerHealth(g_Npc, health);
			Check(health == 50.0, "taken damage multiplier");
			WF_SetPlayerDamageMultiplier(g_Npc, 1.0, 1.0);

			SetPlayerHealth(g_Npc, 80.0);
			GetPlayerHealth(g_Npc, health);
			Check(health == 80.0, "SetPlayerHealth (server-side)");

			// Give the NPC time to send sync packets
			SetTimer("RT_Step", 3000, false);
			return 1;
		}
		case 2: {
			// The client's health is 8000080; its sync packets carry the fake value
			WF_GetPlayerRawHealth(g_Npc, health);
			printf("[rt] raw (synced) health: %.1f", health);
			Check(health == 80.0, "fake health written into sync packets");
		}
		case 3: {
			Check(WF_KillPlayer(g_Npc, INVALID_PLAYER_ID, WEAPON_DEAGLE) == 1, "WF_KillPlayer");
			Check(IsPlayerDying(g_Npc) == 1, "player is dying");
			Check(GetPlayerState(g_Npc) == PLAYER_STATE_WASTED, "GetPlayerState returns WASTED while dying");
			Check(g_PrepareCalls == 1, "OnPlayerPrepareDeath called");
			Check(g_DeathCalls == 1 && g_LastDeathReason == _:WEAPON_DEAGLE && g_LastDeathKiller == INVALID_PLAYER_ID, "OnPlayerDeath emitted with reason");

			DamagePlayer(g_Npc, 10.0);
			Check(g_DamageCalls == 4, "dying players take no damage");
			return 1;
		}
	}

	SetTimer("RT_Step", 300, false);
	return 1;
}

public OnPlayerDeath(playerid, killerid, WEAPON:reason)
{
	if (playerid == g_Npc) {
		g_DeathCalls++;
		g_LastDeathReason = _:reason;
		g_LastDeathKiller = killerid;
		printf("[rt] OnPlayerDeath(%d, %d, %d)", playerid, killerid, _:reason);
	}
	return 1;
}

public OnPlayerDamage(&playerid, &Float:amount, &issuerid, &WEAPON:weapon, &bodypart)
{
	g_DamageCalls++;
	return 1;
}

public OnPlayerDamageDone(playerid, Float:amount, issuerid, WEAPON:weapon, bodypart)
{
	g_DamageDoneCalls++;
	return 1;
}

public OnPlayerPrepareDeath(playerid, animlib[32], animname[32], &anim_lock, &respawn_time)
{
	g_PrepareCalls++;
	printf("[rt] OnPlayerPrepareDeath anim %s:%s respawn %d", animlib, animname, respawn_time);
	return 1;
}

public OnPlayerDeathFinished(playerid, bool:cancelable)
{
	g_FinishedCalls++;
	printf("[rt] OnPlayerDeathFinished(%d, %d)", playerid, cancelable);

	#if defined _INC_open_mp
		// open.mp NPCs are respawned through the NPC API
		if (cancelable && IsPlayerNPC(playerid)) {
			NPC_Respawn(playerid);
		}
	#endif
	return 1;
}
