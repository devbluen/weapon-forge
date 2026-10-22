/*
 * weapon-forge - server-side health, damage, death and weapon configuration
 * for SA-MP 0.3.7/0.3.DL and open.mp.
 *
 * Everything runs inside the plugin: it hooks the server callbacks and
 * natives by itself. This file only declares the natives, constants and
 * callbacks so scripts can use them; it adds no code to the script.
 *
 *     #include <open.mp>          // or <a_samp>
 *     #include <weapon-forge>
 *
 * The weapon-config names (SetWeaponDamage, DamagePlayer, ...) are declared
 * as aliases of the WF_* natives. Define WF_NO_COMPAT before including this
 * file to declare only the WF_* names.
 */

#if defined _INC_weapon_forge
	#endinput
#endif
#define _INC_weapon_forge

#tryinclude <open.mp>

#if !defined _INC_open_mp
	#include <a_samp>
#endif

// Tag used by open.mp, mapped to untagged in SA-MP
#if !defined _INC_open_mp && !defined WEAPON
	#define WEAPON: _:
#endif

/*
 * Constants (same values as weapon-config)
 */

// Errors given in OnInvalidWeaponDamage
enum {
	WC_NO_ERROR,
	WC_NO_ISSUER,
	WC_NO_DAMAGED,
	WC_INVALID_DAMAGE,
	WC_INVALID_DISTANCE
}

// Used in SetWeaponDamage
enum {
	DAMAGE_TYPE_MULTIPLIER,
	DAMAGE_TYPE_STATIC,
	DAMAGE_TYPE_RANGE_MULTIPLIER,
	DAMAGE_TYPE_RANGE
}

#define WF_MAX_PLAYER_NAME (24)

// Given in OnRejectedHit
enum E_REJECTED_HIT {
	       e_Time,
	       e_Hour,
	       e_Minute,
	       e_Second,
	WEAPON:e_Weapon,
	       e_Reason,
	       e_Info1,
	       e_Info2,
	       e_Info3,
	       e_Name[WF_MAX_PLAYER_NAME]
}

// e_Reason in E_REJECTED_HIT
enum {
	HIT_NO_DAMAGEDID,
	HIT_INVALID_WEAPON,
	HIT_LAST_SHOT_INVALID,
	HIT_MULTIPLE_PLAYERS,
	HIT_MULTIPLE_PLAYERS_SHOTGUN,
	HIT_DYING_PLAYER,
	HIT_SAME_TEAM,
	HIT_UNSTREAMED,
	HIT_INVALID_HITTYPE,
	HIT_BEING_RESYNCED,
	HIT_NOT_SPAWNED,
	HIT_OUT_OF_RANGE,
	HIT_TOO_FAR_FROM_SHOT,
	SHOOTING_RATE_TOO_FAST,
	SHOOTING_RATE_TOO_FAST_MULTIPLE,
	HIT_RATE_TOO_FAST,
	HIT_RATE_TOO_FAST_MULTIPLE,
	HIT_TOO_FAR_FROM_ORIGIN,
	HIT_INVALID_DAMAGE,
	HIT_SAME_VEHICLE,
	HIT_OWN_VEHICLE,
	HIT_INVALID_VEHICLE,
	HIT_DISCONNECTED,
	HIT_PROTECTED // weapon-forge: target in god mode or spawn protection
}

// WF_GetPlayerStat
enum {
	WF_STAT_SHOTS_FIRED,
	WF_STAT_SHOTS_HIT,
	WF_STAT_HITS_TAKEN,
	WF_STAT_HEADSHOTS_GIVEN,
	WF_STAT_HEADSHOTS_TAKEN,
	WF_STAT_KILLS,
	WF_STAT_DEATHS,
	WF_STAT_ASSISTS,
	WF_STAT_REJECTED_HITS
}

// WF_GetPlayerStatFloat
enum {
	WF_STATF_DAMAGE_GIVEN,
	WF_STATF_DAMAGE_TAKEN,
	WF_STATF_HEALTH_LOST,
	WF_STATF_ARMOUR_LOST
}

// Weapon ids used by weapon-config
#define BODY_PART_UNKNOWN 0
#define WEAPON_UNARMED (WEAPON:0)
#define WEAPON_VEHICLE_M4 (WEAPON:19)
#define WEAPON_VEHICLE_MINIGUN (WEAPON:20)
#define WEAPON_VEHICLE_ROCKETLAUNCHER (WEAPON:21)
#define WEAPON_PISTOLWHIP (WEAPON:48)
#define WEAPON_HELIBLADES (WEAPON:50)
#define WEAPON_EXPLOSION (WEAPON:51)
#define WEAPON_CARPARK (WEAPON:52)

// Possibly defined in omp-stdlib with a different value
#if defined WEAPON_UNKNOWN
	#undef WEAPON_UNKNOWN
#endif
#define WEAPON_UNKNOWN (WEAPON:55)

/*
 * Callbacks raised by the plugin
 */

// When a player takes or gives invalid damage (WC_* errors above)
forward OnInvalidWeaponDamage(playerid, damagedid, Float:amount, WEAPON:weaponid, bodypart, error, bool:given);
// Before damage is inflicted. Return 0 to cancel it; any argument can be modified.
forward OnPlayerDamage(&playerid, &Float:amount, &issuerid, &WEAPON:weapon, &bodypart);
// After damage was inflicted
forward OnPlayerDamageDone(playerid, Float:amount, issuerid, WEAPON:weapon, bodypart);
// Before the death animation is applied
forward OnPlayerPrepareDeath(playerid, animlib[32], animname[32], &anim_lock, &respawn_time);
// When the death animation is finished. Return 0 (with cancelable = true) to keep the player alive.
forward OnPlayerDeathFinished(playerid, bool:cancelable);
// When a shot or damage given is rejected
forward OnRejectedHit(playerid, hit[E_REJECTED_HIT]);
// When a player is about to use a vending machine. Return 0 to prevent it.
forward OnPlayerUseVendingMachine(playerid, &Float:health_given);
// After a bullet hit the head (bodypart 9)
forward OnPlayerHeadshot(playerid, issuerid, WEAPON:weaponid, Float:amount);
// For each player that dealt enough damage to the victim before someone else killed it
forward OnPlayerKillAssist(playerid, victimid, killerid, Float:damage);
// When a player gives or takes damage after being out of combat
forward OnPlayerEnterCombat(playerid);
// When the combat timeout (WF_SetCombatTimeout) expires
forward OnPlayerLeaveCombat(playerid);
// When a c-bug is detected (c-bug disabled). Return 0 to skip the punishment.
forward OnPlayerCbug(playerid, WEAPON:weaponid, interval);

/*
 * Natives
 *
 * The server natives SetPlayerHealth, GetPlayerHealth, SetPlayerArmour,
 * GetPlayerArmour, GetPlayerState, SpawnPlayer, SetPlayerTeam, GetPlayerTeam,
 * SendDeathMessage, GetWeaponName, ApplyAnimation, ClearAnimations,
 * TogglePlayerSpectating, TogglePlayerControllable, SetPlayerPos,
 * SetPlayerPosFindZ, SetPlayerVelocity, SetPlayerVirtualWorld,
 * GetPlayerVirtualWorld, PlayerSpectatePlayer, DestroyVehicle,
 * IsPlayerInCheckpoint, IsPlayerInRaceCheckpoint and SetPlayerSpecialAction
 * are replaced by the plugin in every script, so they keep their names.
 */

// Weapons and damage
native WF_SetWeaponDamage(WEAPON:weaponid, damage_type, Float:amount, Float:...);
native Float:WF_GetWeaponDamage(WEAPON:weaponid);
native WF_GetWeaponDamageType(WEAPON:weaponid);
native WF_SetWeaponMaxRange(WEAPON:weaponid, Float:range);
native Float:WF_GetWeaponMaxRange(WEAPON:weaponid);
native WF_SetWeaponShootRate(WEAPON:weaponid, max_rate);
native WF_GetWeaponShootRate(WEAPON:weaponid);
native WF_SetWeaponArmourRule(WEAPON:weaponid, bool:affects_armour, bool:torso_only = false);
native WF_SetCustomArmourRules(bool:armour_rules, bool:torso_rules = false);
// weaponid = -1 applies to every weapon; bodypart 3 (torso) .. 9 (head)
native WF_SetWeaponBodypartMultiplier(weaponid, bodypart, Float:multiplier);
native Float:WF_GetWeaponBodypartMultiplier(WEAPON:weaponid, bodypart);
native WF_SetWeaponName(WEAPON:weaponid, const name[]);
native WF_GetWeaponName(WEAPON:weaponid, name[], len = sizeof(name));
native bool:WF_IsBulletWeapon(WEAPON:weaponid);
native bool:WF_IsMeleeWeapon(WEAPON:weaponid);
native bool:WF_IsHighRateWeapon(WEAPON:weaponid);
native WF_DamagePlayer(playerid, Float:amount, issuerid = INVALID_PLAYER_ID, WEAPON:weaponid = WEAPON_UNKNOWN, bodypart = BODY_PART_UNKNOWN, bool:ignore_armour = false);

// Health, armour and death
native WF_SetPlayerHealth(playerid, Float:health, Float:armour = -1.0);
native Float:WF_GetPlayerHealth(playerid, &Float:health = 0.0);
native WF_SetPlayerArmour(playerid, Float:armour);
native Float:WF_GetPlayerArmour(playerid, &Float:armour = 0.0);
native WF_SetPlayerMaxHealth(playerid, Float:value);
native WF_SetPlayerMaxArmour(playerid, Float:value);
native Float:WF_GetPlayerMaxHealth(playerid);
native Float:WF_GetPlayerMaxArmour(playerid);
native Float:WF_GetLastDamageHealth(playerid);
native Float:WF_GetLastDamageArmour(playerid);
native WF_IsPlayerDying(playerid);
native bool:WF_IsPlayerSpawned(playerid);
native bool:WF_IsPlayerPaused(playerid);
native WF_SetRespawnTime(ms);
native WF_GetRespawnTime();
native WF_KillPlayer(playerid, killerid = INVALID_PLAYER_ID, WEAPON:weaponid = WEAPON_UNKNOWN);
native WF_RevivePlayer(playerid, Float:health = -1.0);
native WF_ResyncPlayer(playerid);

// Protection and modifiers
native WF_SetPlayerGodMode(playerid, bool:toggle);
native bool:WF_IsPlayerGodMode(playerid);
native WF_SetSpawnProtection(ms);
native WF_GetSpawnProtection();
native WF_SetPlayerProtection(playerid, ms);
native WF_GetPlayerProtectionTime(playerid);
native WF_SetPlayerDamageMultiplier(playerid, Float:given = 1.0, Float:taken = 1.0);
native WF_GetPlayerDamageMultiplier(playerid, &Float:given, &Float:taken);
native WF_SetFriendlyFire(bool:toggle);
native bool:WF_GetFriendlyFire();

// Combat, history and statistics
native WF_SetCombatTimeout(ms);
native bool:WF_IsPlayerInCombat(playerid);
native WF_GetLastAttacker(playerid);
native WF_GetTimeSinceLastDamage(playerid);
// idx 0 = most recent hit taken (up to 10)
native WF_GetPlayerHitHistory(playerid, idx, &issuerid, &WEAPON:weaponid, &Float:amount, &bodypart, &ms_ago);
native WF_GetLastShotInfo(playerid, &WEAPON:weaponid, &hittype, &hitid, &Float:hit_x, &Float:hit_y, &Float:hit_z, &Float:length);
native WF_SetAssistParams(Float:min_damage = 10.0, window = 15000);
native WF_GetPlayerStat(playerid, stat);
native Float:WF_GetPlayerStatFloat(playerid, stat);
native WF_ResetPlayerStats(playerid);
native Float:WF_GetPlayerAccuracy(playerid);

// Health over time
native WF_SetHealthRegen(Float:health_per_second, delay_after_damage = 5000);
native WF_SetPlayerHealthRegen(playerid, Float:health_per_second = -1.0, delay_after_damage = -1);
native WF_SetPlayerBleeding(playerid, Float:damage_per_second, duration, issuerid = INVALID_PLAYER_ID, WEAPON:weaponid = WEAPON_UNKNOWN);
native WF_StopPlayerBleeding(playerid);
native bool:WF_IsPlayerBleeding(playerid);

// Anti-cheat and sync
native WF_SetCbugAllowed(bool:enabled, playerid = INVALID_PLAYER_ID);
native bool:WF_GetCbugAllowed(playerid = INVALID_PLAYER_ID);
native WF_SetCbugDeathDelay(bool:toggle);
native WF_GetRejectedHit(playerid, idx, output[], maxlength = sizeof(output));
native WF_AverageShootRate(playerid, shots, &multiple_weapons = 0);
native WF_AverageHitRate(playerid, hits, &multiple_weapons = 0);
native WF_SetKnifeSync(toggle);
native WF_SetDisableSyncBugs(toggle);
native WF_SetLagCompMode(mode);
native WF_GetLagCompMode();
// Still call OnPlayerGiveDamage/OnPlayerTakeDamage in the scripts (off by default, like weapon-config)
native WF_SetForwardDamageCallbacks(bool:toggle);

// Environment
native WF_SetCustomFallDamage(bool:toggle, Float:damage_multiplier = 25.0, Float:death_velocity = -0.6);
native WF_SetCustomVendingMachines(bool:toggle);
native WF_SetVehiclePassengerDamage(bool:toggle);
native WF_SetVehicleUnoccupiedDamage(bool:toggle);
native WF_SetDamageSounds(taken, given);

// Damage feed
native WF_SetDamageFeed(bool:toggle);
native WF_SetDamageFeedForPlayer(playerid, toggle = -1);
native WF_IsDamageFeedActive(playerid = INVALID_PLAYER_ID);
native WF_SetDamageFeedHideDelay(ms);
native WF_SetDamageFeedColors(given_color, taken_color);
native WF_SetDamageFeedPosition(Float:given_x, Float:given_y, Float:taken_x, Float:taken_y);

// Health bar (textdraw showing the server-side health)
native WF_EnableHealthBarForPlayer(playerid, bool:enable);
native WF_GetHealthBarPosition(&Float:x, &Float:y, playerid = INVALID_PLAYER_ID);
native WF_GetHealthBarSize(&Float:x, &Float:y, playerid = INVALID_PLAYER_ID);
native WF_GetHealthBarPadding(Float:padding[4], playerid = INVALID_PLAYER_ID);
native WF_SetHealthBarPosition(Float:x, Float:y);
native WF_SetHealthBarSize(Float:x, Float:y);
native WF_SetHealthBarPadding(const Float:padding[4]);
native WF_SetHealthBarPosForPlayer(playerid, Float:x = Float:0x7FFFFFFF, Float:y = Float:0x7FFFFFFF);
native WF_SetHealthBarSizeForPlayer(playerid, Float:x = Float:0x7FFFFFFF, Float:y = Float:0x7FFFFFFF);
native WF_SetHealthBarPaddingForPlayer(playerid, const Float:padding[4] = {Float:0x7FFFFFFF, Float:0x7FFFFFFF, Float:0x7FFFFFFF, Float:0x7FFFFFFF});
native WF_GetHealthBarColor(&borderColor = 0, &bgColor = 0, &fgColor = 0, playerid = INVALID_PLAYER_ID);
native WF_SetHealthBarColor(borderColor = 0, bgColor = 0, fgColor = 0);
native WF_SetHealthBarColorForPlayer(playerid, borderColor = 0, bgColor = 0, fgColor = 0);

// Misc
native WF_SetDebug(level);
native bool:WF_IsOpenMP();
// Whether incoming sync packets are processed (nametag health, sync fixes)
native bool:WF_IsRakNetActive();
// Health stored by the server itself (8000000 + x, or the synced value when packets are processed)
native Float:WF_GetPlayerRawHealth(playerid, &Float:health = 0.0);
native WF_GetVersion(version[] = "", len = sizeof(version));

/*
 * weapon-config compatible names (aliases of the natives above)
 */

#if !defined WF_NO_COMPAT
	native SetWeaponDamage(WEAPON:weaponid, damage_type, Float:amount, Float:...) = WF_SetWeaponDamage;
	native Float:GetWeaponDamage(WEAPON:weaponid) = WF_GetWeaponDamage;
	native SetWeaponMaxRange(WEAPON:weaponid, Float:range) = WF_SetWeaponMaxRange;
	native Float:GetWeaponMaxRange(WEAPON:weaponid) = WF_GetWeaponMaxRange;
	native SetWeaponShootRate(WEAPON:weaponid, max_rate) = WF_SetWeaponShootRate;
	native GetWeaponShootRate(WEAPON:weaponid) = WF_GetWeaponShootRate;
	native SetWeaponArmourRule(WEAPON:weaponid, bool:affects_armour, bool:torso_only = false) = WF_SetWeaponArmourRule;
	native SetCustomArmourRules(bool:armour_rules, bool:torso_rules = false) = WF_SetCustomArmourRules;
	native SetWeaponName(WEAPON:weaponid, const name[]) = WF_SetWeaponName;
	native bool:IsBulletWeapon(WEAPON:weaponid) = WF_IsBulletWeapon;
	native bool:IsMeleeWeapon(WEAPON:weaponid) = WF_IsMeleeWeapon;
	native bool:IsHighRateWeapon(WEAPON:weaponid) = WF_IsHighRateWeapon;
	native DamagePlayer(playerid, Float:amount, issuerid = INVALID_PLAYER_ID, WEAPON:weaponid = WEAPON_UNKNOWN, bodypart = BODY_PART_UNKNOWN, bool:ignore_armour = false) = WF_DamagePlayer;
	native SetPlayerMaxHealth(playerid, Float:value) = WF_SetPlayerMaxHealth;
	native SetPlayerMaxArmour(playerid, Float:value) = WF_SetPlayerMaxArmour;
	native Float:GetPlayerMaxHealth(playerid) = WF_GetPlayerMaxHealth;
	native Float:GetPlayerMaxArmour(playerid) = WF_GetPlayerMaxArmour;
	native Float:GetLastDamageHealth(playerid) = WF_GetLastDamageHealth;
	native Float:GetLastDamageArmour(playerid) = WF_GetLastDamageArmour;
	native IsPlayerDying(playerid) = WF_IsPlayerDying;
	native bool:WC_IsPlayerSpawned(playerid) = WF_IsPlayerSpawned;
	native bool:WC_IsPlayerPaused(playerid) = WF_IsPlayerPaused;
	native WC_GetWeaponName(WEAPON:weaponid, name[], len = sizeof(name)) = WF_GetWeaponName;
	native SetRespawnTime(ms) = WF_SetRespawnTime;
	native GetRespawnTime() = WF_GetRespawnTime;
	native ResyncPlayer(playerid) = WF_ResyncPlayer;
	native SetCbugAllowed(bool:enabled, playerid = INVALID_PLAYER_ID) = WF_SetCbugAllowed;
	native bool:GetCbugAllowed(playerid = INVALID_PLAYER_ID) = WF_GetCbugAllowed;
	native SetCbugDeathDelay(bool:toggle) = WF_SetCbugDeathDelay;
	native GetRejectedHit(playerid, idx, output[], maxlength = sizeof(output)) = WF_GetRejectedHit;
	native AverageShootRate(playerid, shots, &multiple_weapons = 0) = WF_AverageShootRate;
	native AverageHitRate(playerid, hits, &multiple_weapons = 0) = WF_AverageHitRate;
	native SetKnifeSync(toggle) = WF_SetKnifeSync;
	native SetDisableSyncBugs(toggle) = WF_SetDisableSyncBugs;
	native SetCustomFallDamage(bool:toggle, Float:damage_multiplier = 25.0, Float:death_velocity = -0.6) = WF_SetCustomFallDamage;
	native SetCustomVendingMachines(bool:toggle) = WF_SetCustomVendingMachines;
	native SetVehiclePassengerDamage(bool:toggle) = WF_SetVehiclePassengerDamage;
	native SetVehicleUnoccupiedDamage(bool:toggle) = WF_SetVehicleUnoccupiedDamage;
	native SetDamageSounds(taken, given) = WF_SetDamageSounds;
	native SetDamageFeed(bool:toggle) = WF_SetDamageFeed;
	native SetDamageFeedForPlayer(playerid, toggle = -1) = WF_SetDamageFeedForPlayer;
	native IsDamageFeedActive(playerid = INVALID_PLAYER_ID) = WF_IsDamageFeedActive;
	native EnableHealthBarForPlayer(playerid, bool:enable) = WF_EnableHealthBarForPlayer;
	native GetHealthBarPosition(&Float:x, &Float:y, playerid = INVALID_PLAYER_ID) = WF_GetHealthBarPosition;
	native GetHealthBarSize(&Float:x, &Float:y, playerid = INVALID_PLAYER_ID) = WF_GetHealthBarSize;
	native GetHealthBarPadding(Float:padding[4], playerid = INVALID_PLAYER_ID) = WF_GetHealthBarPadding;
	native SetHealthBarPosition(Float:x, Float:y) = WF_SetHealthBarPosition;
	native SetHealthBarSize(Float:x, Float:y) = WF_SetHealthBarSize;
	native SetHealthBarPadding(const Float:padding[4]) = WF_SetHealthBarPadding;
	native SetHealthBarPositionForPlayer(playerid, Float:x = Float:0x7FFFFFFF, Float:y = Float:0x7FFFFFFF) = WF_SetHealthBarPosForPlayer;
	native SetHealthBarSizeForPlayer(playerid, Float:x = Float:0x7FFFFFFF, Float:y = Float:0x7FFFFFFF) = WF_SetHealthBarSizeForPlayer;
	native SetHealthBarPaddingForPlayer(playerid, const Float:padding[4] = {Float:0x7FFFFFFF, Float:0x7FFFFFFF, Float:0x7FFFFFFF, Float:0x7FFFFFFF}) = WF_SetHealthBarPaddingForPlayer;
	native GetHealthBarColor(&borderColor = 0, &bgColor = 0, &fgColor = 0, playerid = INVALID_PLAYER_ID) = WF_GetHealthBarColor;
	native SetHealthBarColor(borderColor = 0, bgColor = 0, fgColor = 0) = WF_SetHealthBarColor;
	native SetHealthBarColorForPlayer(playerid, borderColor = 0, bgColor = 0, fgColor = 0) = WF_SetHealthBarColorForPlayer;
#endif
