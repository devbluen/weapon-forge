#pragma once

#include "state.hpp"
#include "timers.hpp"

#include <string>

namespace wf
{
	extern int g_max_players;
	// True while the plugin itself calls a standard callback (OnPlayerDeath,
	// OnVehicleDeath...) in the scripts, so the include hooks let it through.
	extern bool g_emitting;

	inline Player &P(int playerid)
	{
		return g_players[playerid];
	}

	// lifecycle (events.cpp)
	void ScriptInit();
	void ScriptExit();
	void ResetPlayer(int playerid);
	void ProcessTick();

	// health bar & damage feed (hud.cpp)
	void UpdateHealthBar(int playerid, bool force = false);
	void SetHealthBarVisible(int playerid, bool toggle);
	void UpdateHealthBarSize(int playerid);
	void DestroyHud(int playerid);
	void GetHealthBarPosition(int playerid, float &x, float &y);
	void GetHealthBarSize(int playerid, float &x, float &y);
	void GetHealthBarPadding(int playerid, float padding[4]);
	void GetHealthBarColor(int playerid, int &border, int &bg, int &fg);
	bool IsDamageFeedActive(int playerid);
	void DamageFeedUpdate(int playerid, bool modified = false);
	void DamageFeedAddHitGiven(int playerid, int damagedid, float amount, int weapon);
	void DamageFeedAddHitTaken(int playerid, int issuerid, float amount, int weapon);

	// combat (combat.cpp)
	bool IsPlayerSpawned(int playerid);
	bool IsPlayerPaused(int playerid);
	// God mode or spawn protection
	bool IsProtected(int playerid);
	bool HasSameTeam(int playerid, int otherid);
	bool IsVehicleBike(int vehicleid);
	bool IsVehicleArmedWithWeapon(int vehicleid, int weaponid);
	int AverageShootRate(int playerid, int shots, bool *multiple_weapons);
	int AverageHitRate(int playerid, int shots, bool *multiple_weapons);
	void AddRejectedHit(int playerid, int damagedid, int reason, int weapon, cell i1 = 0, cell i2 = 0, cell i3 = 0);
	std::string FormatRejectedHit(int playerid, int idx, bool *ok);
	void InflictDamage(int playerid, float amount, int issuerid = kInvalidPlayer, int weaponid = 55, int bodypart = 0, bool ignore_armour = false, bool bypass_protection = false);
	// Server-side SetPlayerHealth (health <= 0 kills the player)
	void SetHealth(int playerid, float health, float armour = -1.0f);
	void SetArmour(int playerid, float armour);
	void OnDamageDone(int playerid, float amount, int issuerid, int weapon, int bodypart);
	bool CallOnPlayerDamage(int &playerid, float &amount, int &issuerid, int &weapon, int &bodypart);
	void CallOnInvalidWeaponDamage(int playerid, int damagedid, float amount, int weaponid, int bodypart, int error, bool given);
	void EnterCombat(int playerid);

	int OnGiveDamage(int playerid, int damagedid, float amount, int weaponid, int bodypart);
	int OnTakeDamage(int playerid, int issuerid, float amount, int weaponid, int bodypart);
	int OnWeaponShot(int playerid, int weaponid, int hittype, int hitid, float x, float y, float z);
	void OnWeaponShotResult(int playerid, int hittype, int hitid, bool valid);

	// death (death.cpp)
	void PlayerDeath(int playerid, const char *animlib, const char *animname, bool anim_lock = false, int respawn_time = -1, bool freeze_sync = true, bool anim_freeze = true);
	void KillFromDamage(int playerid, int issuerid, int weaponid, int bodypart);
	void EmitDeath(int playerid, int killerid, int reason);
	bool CallDeathFinished(int playerid, bool cancelable);
	void DeathRespawn(int playerid);
	int OnRealDeath(int playerid, int &killerid, int &reason);
	void MakePlayerFacePlayer(int playerid, int targetid, bool opposite = false);
	bool IsPlayerBehindPlayer(int playerid, int targetid, float diff = 90.0f);
	void ClearDying(int playerid);
	void RevivePlayer(int playerid, float health);
	void UpdatePlayerVirtualWorld(int playerid);

	// sync (sync.cpp)
	void FreezeSyncPacket(int playerid, bool toggle);
	void SetFakeHealth(int playerid, int health);
	void SetFakeArmour(int playerid, int armour);
	void SetFakeFacingAngle(int playerid, float angle = kNaN);
	// Patches an incoming packet (data[0] = packet id) in place
	void ProcessIncomingPacket(int playerid, unsigned char *data, size_t bytes);
	void CommitSyncData(int playerid);

	// vending machines (vending.cpp)
	void CreateVendingMachines();
	void DestroyVendingMachines();
	void RemoveDefaultVendingMachines(int playerid);
	bool TryUseVendingMachine(int playerid);
	void FinishVendingMachine(int playerid, float health_given);

	// events (events.cpp), return values follow the include's contract
	void OnPlayerConnect(int playerid);
	void OnPlayerDisconnect(int playerid);
	void CleanupPlayer(int playerid);
	int OnPlayerSpawn(int playerid);
	int OnPlayerRequestClass(int playerid);
	int OnPlayerKeyStateChange(int playerid, int newkeys, int oldkeys);
	void OnPlayerEnterVehicle(int playerid);
	void OnPlayerExitVehicle(int playerid);
	void OnPlayerStateChange(int playerid, int newstate, int oldstate);
	int OnPlayerUpdate(int playerid);
	void ProcessPeriodic(uint32_t now);
	void OnVehicleSpawn(int vehicleid);
	int OnVehicleDeath(int vehicleid);
	void OnTimer(timers::Type type, int id, const timers::Args &args);
}
