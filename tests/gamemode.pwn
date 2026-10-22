// Compile test: gamemode using the weapon-config compatible API and the
// weapon-forge extras. Built in CI against SA-MP and open.mp includes.

#if defined TEST_OPEN_MP
	#include <open.mp>
#else
	#include <a_samp>
#endif

#include <weapon-forge>

main()
{
	print("weapon-forge test gamemode");
}

public OnGameModeInit()
{
	new version[16];
	WF_GetVersion(version);
	printf("[test] weapon-forge %s | open.mp: %d | raknet: %d", version, WF_IsOpenMP(), WF_IsRakNetActive());

	// weapon-config API
	SetWeaponDamage(WEAPON_DEAGLE, DAMAGE_TYPE_STATIC, 50.0);
	SetWeaponDamage(WEAPON_SNIPER, DAMAGE_TYPE_RANGE, 41.0, 50.0, 30.0, 100.0, 20.0);
	SetWeaponDamage(WEAPON_M4, DAMAGE_TYPE_RANGE_MULTIPLIER, 1.0, 40.0, 0.8);
	SetCustomArmourRules(true, true);
	SetWeaponArmourRule(WEAPON_COLT45, true, false);
	SetDamageSounds(0, 17802);
	SetCbugAllowed(false);
	SetCustomFallDamage(true, 25.0, -0.6);
	SetVehiclePassengerDamage(true);
	SetVehicleUnoccupiedDamage(false);
	SetDamageFeed(true);
	SetRespawnTime(3500);
	SetWeaponShootRate(WEAPON_DEAGLE, 140);
	SetWeaponMaxRange(WEAPON_DEAGLE, 40.0);
	SetWeaponName(WEAPON_UNKNOWN, "Bleeding");
	SetHealthBarColor(0x000000FF, 0x5A0C0EFF, 0xB4191DFF);
	SetHealthBarPosition(546.0, 66.7);

	new Float:padding[4] = {2.1, 1.9, 1.6, 2.0};
	SetHealthBarPadding(padding);

	// weapon-forge extras
	WF_SetSpawnProtection(3000);
	WF_SetFriendlyFire(false);
	WF_SetCombatTimeout(10000);
	WF_SetHealthRegen(1.5, 8000);
	WF_SetAssistParams(15.0, 10000);
	WF_SetWeaponBodypartMultiplier(-1, 9, 1.5);
	WF_SetDamageFeedHideDelay(4000);
	WF_SetDamageFeedColors(0x30FF50FF, 0x33CCFFFF);
	WF_SetDebug(0);

	printf("[test] deagle damage %.2f, range %.1f, rate %d", GetWeaponDamage(WEAPON_DEAGLE), GetWeaponMaxRange(WEAPON_DEAGLE), GetWeaponShootRate(WEAPON_DEAGLE));

	AddPlayerClass(0, 0.0, 0.0, 3.0, 0.0, WEAPON_DEAGLE, 100, WEAPON_M4, 300, WEAPON_SNIPER, 50);
	return 1;
}

public OnGameModeExit()
{
	return 1;
}

public OnPlayerConnect(playerid)
{
	WF_SetPlayerDamageMultiplier(playerid, 1.0, 1.0);
	return 1;
}

public OnPlayerSpawn(playerid)
{
	SetPlayerHealth(playerid, 100.0);
	SetPlayerArmour(playerid, 50.0);

	new Float:health, Float:armour;
	GetPlayerHealth(playerid, health);
	GetPlayerArmour(playerid, armour);
	printf("[test] %d spawned with %.1f hp / %.1f armour", playerid, health, armour);
	return 1;
}

public OnPlayerDeath(playerid, killerid, WEAPON:reason)
{
	SendDeathMessage(killerid, playerid, reason);
	return 1;
}

public OnPlayerUpdate(playerid)
{
	return 1;
}

public OnPlayerKeyStateChange(playerid, KEY:newkeys, KEY:oldkeys)
{
	if ((newkeys & KEY_YES) && !IsPlayerDying(playerid)) {
		WF_SetPlayerBleeding(playerid, 2.0, 5000);
	}
	return 1;
}

public OnPlayerWeaponShot(playerid, WEAPON:weaponid, BULLET_HIT_TYPE:hittype, hitid, Float:fX, Float:fY, Float:fZ)
{
	return 1;
}

public OnPlayerStateChange(playerid, PLAYER_STATE:newstate, PLAYER_STATE:oldstate)
{
	if (GetPlayerState(playerid) == PLAYER_STATE_WASTED) {
		return 1;
	}
	return 1;
}

public OnPlayerDamage(&playerid, &Float:amount, &issuerid, &WEAPON:weapon, &bodypart)
{
	// Headshots with a sniper kill instantly
	if (weapon == WEAPON_SNIPER && bodypart == 9) {
		amount = 1000.0;
	}
	return 1;
}

public OnPlayerDamageDone(playerid, Float:amount, issuerid, WEAPON:weapon, bodypart)
{
	return 1;
}

public OnPlayerPrepareDeath(playerid, animlib[32], animname[32], &anim_lock, &respawn_time)
{
	if (IsBulletWeapon(WEAPON_DEAGLE)) {
		respawn_time = 4000;
	}
	return 1;
}

public OnPlayerDeathFinished(playerid, bool:cancelable)
{
	return 1;
}

public OnRejectedHit(playerid, hit[E_REJECTED_HIT])
{
	new output[144];
	GetRejectedHit(playerid, 0, output);
	printf("[test] rejected hit (%s): %s", hit[e_Name], output);
	return 1;
}

public OnInvalidWeaponDamage(playerid, damagedid, Float:amount, WEAPON:weaponid, bodypart, error, bool:given)
{
	return 1;
}

public OnPlayerUseVendingMachine(playerid, &Float:health_given)
{
	health_given = 20.0;
	return 1;
}

public OnPlayerHeadshot(playerid, issuerid, WEAPON:weaponid, Float:amount)
{
	new name[32];
	GetWeaponName(weaponid, name, sizeof(name));
	printf("[test] headshot %d -> %d (%s)", issuerid, playerid, name);
	return 1;
}

public OnPlayerKillAssist(playerid, victimid, killerid, Float:damage)
{
	return 1;
}

public OnPlayerEnterCombat(playerid)
{
	return 1;
}

public OnPlayerLeaveCombat(playerid)
{
	printf("[test] %d left combat, accuracy %.1f%%, kills %d", playerid, WF_GetPlayerAccuracy(playerid), WF_GetPlayerStat(playerid, WF_STAT_KILLS));
	return 1;
}

public OnPlayerCbug(playerid, WEAPON:weaponid, interval)
{
	return 1;
}
