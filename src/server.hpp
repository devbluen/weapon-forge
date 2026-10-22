#pragma once

#include "sdk/amx.h"

#include <string>

// Bridge to the server's own natives. They are captured by name when the
// server registers them (amx_Register hook), so every native is available
// even if no script uses it. open.mp registers the SA-MP names too.
namespace server
{
	enum class N : int
	{
		SetPlayerHealth,
		SetPlayerArmour,
		SetPlayerTeam,
		GetPlayerTeam,
		ApplyAnimation,
		ClearAnimations,
		GetPlayerState,
		GetPlayerWeapon,
		GetPlayerPos,
		SetPlayerPos,
		GetPlayerVelocity,
		SetPlayerVelocity,
		GetPlayerVehicleID,
		IsPlayerInAnyVehicle,
		IsPlayerConnected,
		IsPlayerNPC,
		SendDeathMessage,
		PlayerPlaySound,
		GetPlayerAnimationIndex,
		TogglePlayerControllable,
		TogglePlayerSpectating,
		SetPlayerArmedWeapon,
		GetPlayerWeaponData,
		ResetPlayerWeapons,
		GetPlayerPing,
		GetPlayerVirtualWorld,
		SetPlayerVirtualWorld,
		GetPlayerFacingAngle,
		SetPlayerFacingAngle,
		GetPlayerSpecialAction,
		SetPlayerSpecialAction,
		GetPlayerKeys,
		GetPlayerLastShotVectors,
		IsPlayerStreamedIn,
		IsVehicleStreamedIn,
		GetPlayerSurfingVehicleID,
		GetPlayerSurfingObjectID,
		GetPlayerDistanceFromPoint,
		GetPlayerVehicleSeat,
		GetVehicleModel,
		GetVehicleVelocity,
		GetVehicleZAngle,
		GetVehicleHealth,
		SetVehicleHealth,
		SetVehicleToRespawn,
		IsValidVehicle,
		GetPlayerFightingStyle,
		GetPlayerMoney,
		IsPlayerInRangeOfPoint,
		CreateObject,
		DestroyObject,
		RemoveBuildingForPlayer,
		GetPlayerName,
		GetConsoleVarAsInt,
		IsPlayerInCheckpoint,
		IsPlayerInRaceCheckpoint,
		GetMaxPlayers,
		CreatePlayerTextDraw,
		PlayerTextDrawDestroy,
		PlayerTextDrawLetterSize,
		PlayerTextDrawTextSize,
		PlayerTextDrawAlignment,
		PlayerTextDrawColor,
		PlayerTextDrawSetOutline,
		PlayerTextDrawBackgroundColor,
		PlayerTextDrawFont,
		PlayerTextDrawShow,
		PlayerTextDrawHide,
		PlayerTextDrawSetString,
		GetPlayerHealth,
		GetPlayerArmour,
		SpawnPlayer,
		SetPlayerPosFindZ,
		PlayerSpectatePlayer,
		DestroyVehicle,
		// Optional (not required for the plugin to work)
		AllowPlayerTeleport,
		IsPlayerTeleportAllowed,
		COUNT
	};

	// Stores a native registered by the server (first registration wins).
	void Capture(const char *name, AMX_NATIVE func);
	bool Has(N n);
	// Returns the names of required natives that are still unresolved.
	std::string Missing();

	// The script used as context (heap) when calling natives.
	void SetContext(AMX *amx);
	AMX *Context();

	cell Invoke(N n, cell *params);

	// Typed wrappers
	bool SetPlayerHealth(int playerid, float health);
	bool SetPlayerArmour(int playerid, float armour);
	bool SetPlayerTeam(int playerid, int team);
	int GetPlayerTeam(int playerid);
	bool ApplyAnimation(int playerid, const char *lib, const char *name, float delta, bool loop, bool lockx, bool locky, bool freeze, int time, int forcesync);
	bool ClearAnimations(int playerid, int forcesync);
	int GetPlayerState(int playerid);
	int GetPlayerWeapon(int playerid);
	bool GetPlayerPos(int playerid, float &x, float &y, float &z);
	bool SetPlayerPos(int playerid, float x, float y, float z);
	bool GetPlayerVelocity(int playerid, float &x, float &y, float &z);
	bool SetPlayerVelocity(int playerid, float x, float y, float z);
	int GetPlayerVehicleID(int playerid);
	bool IsPlayerInAnyVehicle(int playerid);
	bool IsPlayerConnected(int playerid);
	bool IsPlayerNPC(int playerid);
	bool SendDeathMessage(int killer, int killee, int weapon);
	bool PlayerPlaySound(int playerid, int sound, float x, float y, float z);
	int GetPlayerAnimationIndex(int playerid);
	bool TogglePlayerControllable(int playerid, bool toggle);
	bool TogglePlayerSpectating(int playerid, bool toggle);
	bool SetPlayerArmedWeapon(int playerid, int weapon);
	bool GetPlayerWeaponData(int playerid, int slot, int &weapon, int &ammo);
	bool ResetPlayerWeapons(int playerid);
	int GetPlayerPing(int playerid);
	int GetPlayerVirtualWorld(int playerid);
	bool SetPlayerVirtualWorld(int playerid, int world);
	float GetPlayerFacingAngle(int playerid);
	bool SetPlayerFacingAngle(int playerid, float angle);
	int GetPlayerSpecialAction(int playerid);
	bool SetPlayerSpecialAction(int playerid, int action);
	bool GetPlayerKeys(int playerid, int &keys, int &ud, int &lr);
	bool GetPlayerLastShotVectors(int playerid, float &ox, float &oy, float &oz, float &hx, float &hy, float &hz);
	bool IsPlayerStreamedIn(int playerid, int forplayerid);
	bool IsVehicleStreamedIn(int vehicleid, int forplayerid);
	int GetPlayerSurfingVehicleID(int playerid);
	int GetPlayerSurfingObjectID(int playerid);
	float GetPlayerDistanceFromPoint(int playerid, float x, float y, float z);
	int GetPlayerVehicleSeat(int playerid);
	int GetVehicleModel(int vehicleid);
	bool GetVehicleVelocity(int vehicleid, float &x, float &y, float &z);
	float GetVehicleZAngle(int vehicleid);
	float GetVehicleHealth(int vehicleid);
	bool SetVehicleHealth(int vehicleid, float health);
	bool SetVehicleToRespawn(int vehicleid);
	bool IsValidVehicle(int vehicleid);
	int GetPlayerFightingStyle(int playerid);
	int GetPlayerMoney(int playerid);
	bool IsPlayerInRangeOfPoint(int playerid, float range, float x, float y, float z);
	int CreateObject(int model, float x, float y, float z, float rx, float ry, float rz, float drawdistance);
	bool DestroyObject(int objectid);
	bool RemoveBuildingForPlayer(int playerid, int model, float x, float y, float z, float radius);
	std::string GetPlayerName(int playerid);
	int GetConsoleVarAsInt(const char *name);
	bool IsPlayerInCheckpoint(int playerid);
	bool IsPlayerInRaceCheckpoint(int playerid);
	int GetMaxPlayers();
	int CreatePlayerTextDraw(int playerid, float x, float y, const char *text);
	bool PlayerTextDrawDestroy(int playerid, int td);
	bool PlayerTextDrawLetterSize(int playerid, int td, float x, float y);
	bool PlayerTextDrawTextSize(int playerid, int td, float x, float y);
	bool PlayerTextDrawAlignment(int playerid, int td, int alignment);
	bool PlayerTextDrawColor(int playerid, int td, int color);
	bool PlayerTextDrawSetOutline(int playerid, int td, int size);
	bool PlayerTextDrawBackgroundColor(int playerid, int td, int color);
	bool PlayerTextDrawFont(int playerid, int td, int font);
	bool PlayerTextDrawShow(int playerid, int td);
	bool PlayerTextDrawHide(int playerid, int td);
	bool PlayerTextDrawSetString(int playerid, int td, const char *text);
	float GetPlayerHealth(int playerid);
	float GetPlayerArmour(int playerid);
	bool SpawnPlayer(int playerid);
	bool SetPlayerPosFindZ(int playerid, float x, float y, float z);
	bool PlayerSpectatePlayer(int playerid, int targetid, int mode);
	bool DestroyVehicle(int vehicleid);
	bool AllowPlayerTeleport(int playerid, bool allow);
	bool IsPlayerTeleportAllowed(int playerid);
}
