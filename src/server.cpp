#include "server.hpp"

#include "amx_api.hpp"

#include <initializer_list>

namespace server
{
	namespace
	{
		struct Def
		{
			const char *name;
			bool optional;
		};

		const Def kDefs[] = {
			{"SetPlayerHealth", false},
			{"SetPlayerArmour", false},
			{"SetPlayerTeam", false},
			{"GetPlayerTeam", false},
			{"ApplyAnimation", false},
			{"ClearAnimations", false},
			{"GetPlayerState", false},
			{"GetPlayerWeapon", false},
			{"GetPlayerPos", false},
			{"SetPlayerPos", false},
			{"GetPlayerVelocity", false},
			{"SetPlayerVelocity", false},
			{"GetPlayerVehicleID", false},
			{"IsPlayerInAnyVehicle", false},
			{"IsPlayerConnected", false},
			{"IsPlayerNPC", false},
			{"SendDeathMessage", false},
			{"PlayerPlaySound", false},
			{"GetPlayerAnimationIndex", false},
			{"TogglePlayerControllable", false},
			{"TogglePlayerSpectating", false},
			{"SetPlayerArmedWeapon", false},
			{"GetPlayerWeaponData", false},
			{"ResetPlayerWeapons", false},
			{"GetPlayerPing", false},
			{"GetPlayerVirtualWorld", false},
			{"SetPlayerVirtualWorld", false},
			{"GetPlayerFacingAngle", false},
			{"SetPlayerFacingAngle", false},
			{"GetPlayerSpecialAction", false},
			{"SetPlayerSpecialAction", false},
			{"GetPlayerKeys", false},
			{"GetPlayerLastShotVectors", false},
			{"IsPlayerStreamedIn", false},
			{"IsVehicleStreamedIn", false},
			{"GetPlayerSurfingVehicleID", false},
			{"GetPlayerSurfingObjectID", false},
			{"GetPlayerDistanceFromPoint", false},
			{"GetPlayerVehicleSeat", false},
			{"GetVehicleModel", false},
			{"GetVehicleVelocity", false},
			{"GetVehicleZAngle", false},
			{"GetVehicleHealth", false},
			{"SetVehicleHealth", false},
			{"SetVehicleToRespawn", false},
			{"IsValidVehicle", false},
			{"GetPlayerFightingStyle", false},
			{"GetPlayerMoney", false},
			{"IsPlayerInRangeOfPoint", false},
			{"CreateObject", false},
			{"DestroyObject", false},
			{"RemoveBuildingForPlayer", false},
			{"GetPlayerName", false},
			{"GetConsoleVarAsInt", false},
			{"IsPlayerInCheckpoint", false},
			{"IsPlayerInRaceCheckpoint", false},
			{"GetMaxPlayers", false},
			{"CreatePlayerTextDraw", false},
			{"PlayerTextDrawDestroy", false},
			{"PlayerTextDrawLetterSize", false},
			{"PlayerTextDrawTextSize", false},
			{"PlayerTextDrawAlignment", false},
			{"PlayerTextDrawColor", false},
			{"PlayerTextDrawSetOutline", false},
			{"PlayerTextDrawBackgroundColor", false},
			{"PlayerTextDrawFont", false},
			{"PlayerTextDrawShow", false},
			{"PlayerTextDrawHide", false},
			{"PlayerTextDrawSetString", false},
			{"GetPlayerHealth", false},
			{"GetPlayerArmour", false},
			{"SpawnPlayer", false},
			{"SetPlayerPosFindZ", false},
			{"PlayerSpectatePlayer", false},
			{"DestroyVehicle", false},
			{"AllowPlayerTeleport", true},
			{"IsPlayerTeleportAllowed", true},
		};

		static_assert(sizeof(kDefs) / sizeof(kDefs[0]) == static_cast<size_t>(N::COUNT), "native table out of sync");

		AMX_NATIVE g_natives[static_cast<int>(N::COUNT)] = {};
		AMX *g_context = nullptr;

		cell F(float f)
		{
			return amx_ftoc(f);
		}

		cell Call(N n, std::initializer_list<cell> args)
		{
			cell params[16];
			int count = 0;

			for (cell arg : args)
			{
				params[++count] = arg;
			}

			params[0] = static_cast<cell>(count * sizeof(cell));
			return Invoke(n, params);
		}
	}

	void Capture(const char *name, AMX_NATIVE func)
	{
		for (int i = 0; i < static_cast<int>(N::COUNT); ++i)
		{
			if (!g_natives[i] && std::strcmp(kDefs[i].name, name) == 0)
			{
				g_natives[i] = func;
				return;
			}
		}
	}

	bool Has(N n)
	{
		return g_natives[static_cast<int>(n)] != nullptr;
	}

	std::string Missing()
	{
		std::string out;

		for (int i = 0; i < static_cast<int>(N::COUNT); ++i)
		{
			if (!g_natives[i] && !kDefs[i].optional)
			{
				if (!out.empty())
				{
					out += ", ";
				}
				out += kDefs[i].name;
			}
		}

		return out;
	}

	void SetContext(AMX *amx)
	{
		g_context = amx;
	}

	AMX *Context()
	{
		return g_context;
	}

	cell Invoke(N n, cell *params)
	{
		AMX_NATIVE fn = g_natives[static_cast<int>(n)];

		if (!fn || !g_context)
		{
			return 0;
		}

		return fn(g_context, params);
	}

	bool SetPlayerHealth(int playerid, float health)
	{
		return Call(N::SetPlayerHealth, {playerid, F(health)}) != 0;
	}

	bool SetPlayerArmour(int playerid, float armour)
	{
		return Call(N::SetPlayerArmour, {playerid, F(armour)}) != 0;
	}

	bool SetPlayerTeam(int playerid, int team)
	{
		return Call(N::SetPlayerTeam, {playerid, team}) != 0;
	}

	int GetPlayerTeam(int playerid)
	{
		return Call(N::GetPlayerTeam, {playerid});
	}

	bool ApplyAnimation(int playerid, const char *lib, const char *name, float delta, bool loop, bool lockx, bool locky, bool freeze, int time, int forcesync)
	{
		if (!g_context)
		{
			return false;
		}

		amx::Heap heap(g_context);
		cell l = heap.String(lib);
		cell a = heap.String(name);

		return Call(N::ApplyAnimation, {playerid, l, a, F(delta), loop, lockx, locky, freeze, time, forcesync}) != 0;
	}

	bool ClearAnimations(int playerid, int forcesync)
	{
		return Call(N::ClearAnimations, {playerid, forcesync}) != 0;
	}

	int GetPlayerState(int playerid)
	{
		return Call(N::GetPlayerState, {playerid});
	}

	int GetPlayerWeapon(int playerid)
	{
		return Call(N::GetPlayerWeapon, {playerid});
	}

	namespace
	{
		bool Get3(N n, int id, float &x, float &y, float &z)
		{
			if (!g_context)
			{
				x = y = z = 0.0f;
				return false;
			}

			amx::Heap heap(g_context);
			cell *p = nullptr;
			cell addr = heap.Alloc(3, &p);

			if (!p)
			{
				x = y = z = 0.0f;
				return false;
			}

			p[0] = p[1] = p[2] = 0;
			cell ret = Call(n, {id, addr, addr + static_cast<cell>(sizeof(cell)), addr + static_cast<cell>(2 * sizeof(cell))});

			x = amx_ctof(p[0]);
			y = amx_ctof(p[1]);
			z = amx_ctof(p[2]);

			return ret != 0;
		}

		float Get1f(N n, int id)
		{
			if (!g_context)
			{
				return 0.0f;
			}

			amx::Heap heap(g_context);
			cell *p = nullptr;
			cell addr = heap.Alloc(1, &p);

			if (!p)
			{
				return 0.0f;
			}

			p[0] = 0;
			Call(n, {id, addr});
			return amx_ctof(p[0]);
		}
	}

	bool GetPlayerPos(int playerid, float &x, float &y, float &z)
	{
		return Get3(N::GetPlayerPos, playerid, x, y, z);
	}

	bool SetPlayerPos(int playerid, float x, float y, float z)
	{
		return Call(N::SetPlayerPos, {playerid, F(x), F(y), F(z)}) != 0;
	}

	bool GetPlayerVelocity(int playerid, float &x, float &y, float &z)
	{
		return Get3(N::GetPlayerVelocity, playerid, x, y, z);
	}

	bool SetPlayerVelocity(int playerid, float x, float y, float z)
	{
		return Call(N::SetPlayerVelocity, {playerid, F(x), F(y), F(z)}) != 0;
	}

	int GetPlayerVehicleID(int playerid)
	{
		return Call(N::GetPlayerVehicleID, {playerid});
	}

	bool IsPlayerInAnyVehicle(int playerid)
	{
		return Call(N::IsPlayerInAnyVehicle, {playerid}) != 0;
	}

	bool IsPlayerConnected(int playerid)
	{
		return Call(N::IsPlayerConnected, {playerid}) != 0;
	}

	bool IsPlayerNPC(int playerid)
	{
		return Call(N::IsPlayerNPC, {playerid}) != 0;
	}

	bool SendDeathMessage(int killer, int killee, int weapon)
	{
		return Call(N::SendDeathMessage, {killer, killee, weapon}) != 0;
	}

	bool PlayerPlaySound(int playerid, int sound, float x, float y, float z)
	{
		return Call(N::PlayerPlaySound, {playerid, sound, F(x), F(y), F(z)}) != 0;
	}

	int GetPlayerAnimationIndex(int playerid)
	{
		return Call(N::GetPlayerAnimationIndex, {playerid});
	}

	bool TogglePlayerControllable(int playerid, bool toggle)
	{
		return Call(N::TogglePlayerControllable, {playerid, toggle}) != 0;
	}

	bool TogglePlayerSpectating(int playerid, bool toggle)
	{
		return Call(N::TogglePlayerSpectating, {playerid, toggle}) != 0;
	}

	bool SetPlayerArmedWeapon(int playerid, int weapon)
	{
		return Call(N::SetPlayerArmedWeapon, {playerid, weapon}) != 0;
	}

	bool GetPlayerWeaponData(int playerid, int slot, int &weapon, int &ammo)
	{
		weapon = ammo = 0;

		if (!g_context)
		{
			return false;
		}

		amx::Heap heap(g_context);
		cell *p = nullptr;
		cell addr = heap.Alloc(2, &p);

		if (!p)
		{
			return false;
		}

		p[0] = p[1] = 0;
		cell ret = Call(N::GetPlayerWeaponData, {playerid, slot, addr, addr + static_cast<cell>(sizeof(cell))});
		weapon = p[0];
		ammo = p[1];
		return ret != 0;
	}

	bool ResetPlayerWeapons(int playerid)
	{
		return Call(N::ResetPlayerWeapons, {playerid}) != 0;
	}

	int GetPlayerPing(int playerid)
	{
		return Call(N::GetPlayerPing, {playerid});
	}

	int GetPlayerVirtualWorld(int playerid)
	{
		return Call(N::GetPlayerVirtualWorld, {playerid});
	}

	bool SetPlayerVirtualWorld(int playerid, int world)
	{
		return Call(N::SetPlayerVirtualWorld, {playerid, world}) != 0;
	}

	float GetPlayerFacingAngle(int playerid)
	{
		return Get1f(N::GetPlayerFacingAngle, playerid);
	}

	bool SetPlayerFacingAngle(int playerid, float angle)
	{
		return Call(N::SetPlayerFacingAngle, {playerid, F(angle)}) != 0;
	}

	int GetPlayerSpecialAction(int playerid)
	{
		return Call(N::GetPlayerSpecialAction, {playerid});
	}

	bool SetPlayerSpecialAction(int playerid, int action)
	{
		return Call(N::SetPlayerSpecialAction, {playerid, action}) != 0;
	}

	bool GetPlayerKeys(int playerid, int &keys, int &ud, int &lr)
	{
		keys = ud = lr = 0;

		if (!g_context)
		{
			return false;
		}

		amx::Heap heap(g_context);
		cell *p = nullptr;
		cell addr = heap.Alloc(3, &p);

		if (!p)
		{
			return false;
		}

		p[0] = p[1] = p[2] = 0;
		cell ret = Call(N::GetPlayerKeys, {playerid, addr, addr + static_cast<cell>(sizeof(cell)), addr + static_cast<cell>(2 * sizeof(cell))});
		keys = p[0];
		ud = p[1];
		lr = p[2];
		return ret != 0;
	}

	bool GetPlayerLastShotVectors(int playerid, float &ox, float &oy, float &oz, float &hx, float &hy, float &hz)
	{
		ox = oy = oz = hx = hy = hz = 0.0f;

		if (!g_context)
		{
			return false;
		}

		amx::Heap heap(g_context);
		cell *p = nullptr;
		cell addr = heap.Alloc(6, &p);

		if (!p)
		{
			return false;
		}

		for (int i = 0; i < 6; ++i)
		{
			p[i] = 0;
		}

		const cell s = static_cast<cell>(sizeof(cell));
		cell ret = Call(N::GetPlayerLastShotVectors, {playerid, addr, addr + s, addr + 2 * s, addr + 3 * s, addr + 4 * s, addr + 5 * s});

		ox = amx_ctof(p[0]);
		oy = amx_ctof(p[1]);
		oz = amx_ctof(p[2]);
		hx = amx_ctof(p[3]);
		hy = amx_ctof(p[4]);
		hz = amx_ctof(p[5]);

		return ret != 0;
	}

	bool IsPlayerStreamedIn(int playerid, int forplayerid)
	{
		return Call(N::IsPlayerStreamedIn, {playerid, forplayerid}) != 0;
	}

	bool IsVehicleStreamedIn(int vehicleid, int forplayerid)
	{
		return Call(N::IsVehicleStreamedIn, {vehicleid, forplayerid}) != 0;
	}

	int GetPlayerSurfingVehicleID(int playerid)
	{
		return Call(N::GetPlayerSurfingVehicleID, {playerid});
	}

	int GetPlayerSurfingObjectID(int playerid)
	{
		return Call(N::GetPlayerSurfingObjectID, {playerid});
	}

	float GetPlayerDistanceFromPoint(int playerid, float x, float y, float z)
	{
		return amx_ctof(Call(N::GetPlayerDistanceFromPoint, {playerid, F(x), F(y), F(z)}));
	}

	int GetPlayerVehicleSeat(int playerid)
	{
		return Call(N::GetPlayerVehicleSeat, {playerid});
	}

	int GetVehicleModel(int vehicleid)
	{
		return Call(N::GetVehicleModel, {vehicleid});
	}

	bool GetVehicleVelocity(int vehicleid, float &x, float &y, float &z)
	{
		return Get3(N::GetVehicleVelocity, vehicleid, x, y, z);
	}

	float GetVehicleZAngle(int vehicleid)
	{
		return Get1f(N::GetVehicleZAngle, vehicleid);
	}

	float GetVehicleHealth(int vehicleid)
	{
		return Get1f(N::GetVehicleHealth, vehicleid);
	}

	bool SetVehicleHealth(int vehicleid, float health)
	{
		return Call(N::SetVehicleHealth, {vehicleid, F(health)}) != 0;
	}

	bool SetVehicleToRespawn(int vehicleid)
	{
		return Call(N::SetVehicleToRespawn, {vehicleid}) != 0;
	}

	bool IsValidVehicle(int vehicleid)
	{
		return Call(N::IsValidVehicle, {vehicleid}) != 0;
	}

	int GetPlayerFightingStyle(int playerid)
	{
		return Call(N::GetPlayerFightingStyle, {playerid});
	}

	int GetPlayerMoney(int playerid)
	{
		return Call(N::GetPlayerMoney, {playerid});
	}

	bool IsPlayerInRangeOfPoint(int playerid, float range, float x, float y, float z)
	{
		return Call(N::IsPlayerInRangeOfPoint, {playerid, F(range), F(x), F(y), F(z)}) != 0;
	}

	int CreateObject(int model, float x, float y, float z, float rx, float ry, float rz, float drawdistance)
	{
		return Call(N::CreateObject, {model, F(x), F(y), F(z), F(rx), F(ry), F(rz), F(drawdistance)});
	}

	bool DestroyObject(int objectid)
	{
		return Call(N::DestroyObject, {objectid}) != 0;
	}

	bool RemoveBuildingForPlayer(int playerid, int model, float x, float y, float z, float radius)
	{
		return Call(N::RemoveBuildingForPlayer, {playerid, model, F(x), F(y), F(z), F(radius)}) != 0;
	}

	std::string GetPlayerName(int playerid)
	{
		if (!g_context)
		{
			return {};
		}

		amx::Heap heap(g_context);
		cell *p = nullptr;
		const int size = 25;
		cell addr = heap.Alloc(size, &p);

		if (!p)
		{
			return {};
		}

		p[0] = 0;
		Call(N::GetPlayerName, {playerid, addr, size});

		char buf[size + 1] = {};
		amx::GetString(buf, p, 0, sizeof(buf));
		return buf;
	}

	int GetConsoleVarAsInt(const char *name)
	{
		if (!g_context || !Has(N::GetConsoleVarAsInt))
		{
			return 0;
		}

		amx::Heap heap(g_context);
		cell str = heap.String(name);
		return Call(N::GetConsoleVarAsInt, {str});
	}

	bool IsPlayerInCheckpoint(int playerid)
	{
		return Call(N::IsPlayerInCheckpoint, {playerid}) != 0;
	}

	bool IsPlayerInRaceCheckpoint(int playerid)
	{
		return Call(N::IsPlayerInRaceCheckpoint, {playerid}) != 0;
	}

	int GetMaxPlayers()
	{
		return Call(N::GetMaxPlayers, {});
	}

	int CreatePlayerTextDraw(int playerid, float x, float y, const char *text)
	{
		if (!g_context)
		{
			return 0xFFFF;
		}

		amx::Heap heap(g_context);
		cell str = heap.String(text);
		return Call(N::CreatePlayerTextDraw, {playerid, F(x), F(y), str});
	}

	bool PlayerTextDrawDestroy(int playerid, int td)
	{
		return Call(N::PlayerTextDrawDestroy, {playerid, td}) != 0;
	}

	bool PlayerTextDrawLetterSize(int playerid, int td, float x, float y)
	{
		return Call(N::PlayerTextDrawLetterSize, {playerid, td, F(x), F(y)}) != 0;
	}

	bool PlayerTextDrawTextSize(int playerid, int td, float x, float y)
	{
		return Call(N::PlayerTextDrawTextSize, {playerid, td, F(x), F(y)}) != 0;
	}

	bool PlayerTextDrawAlignment(int playerid, int td, int alignment)
	{
		return Call(N::PlayerTextDrawAlignment, {playerid, td, alignment}) != 0;
	}

	bool PlayerTextDrawColor(int playerid, int td, int color)
	{
		return Call(N::PlayerTextDrawColor, {playerid, td, color}) != 0;
	}

	bool PlayerTextDrawSetOutline(int playerid, int td, int size)
	{
		return Call(N::PlayerTextDrawSetOutline, {playerid, td, size}) != 0;
	}

	bool PlayerTextDrawBackgroundColor(int playerid, int td, int color)
	{
		return Call(N::PlayerTextDrawBackgroundColor, {playerid, td, color}) != 0;
	}

	bool PlayerTextDrawFont(int playerid, int td, int font)
	{
		return Call(N::PlayerTextDrawFont, {playerid, td, font}) != 0;
	}

	bool PlayerTextDrawShow(int playerid, int td)
	{
		return Call(N::PlayerTextDrawShow, {playerid, td}) != 0;
	}

	bool PlayerTextDrawHide(int playerid, int td)
	{
		return Call(N::PlayerTextDrawHide, {playerid, td}) != 0;
	}

	bool PlayerTextDrawSetString(int playerid, int td, const char *text)
	{
		if (!g_context)
		{
			return false;
		}

		amx::Heap heap(g_context);
		cell str = heap.String(text);
		return Call(N::PlayerTextDrawSetString, {playerid, td, str}) != 0;
	}

	float GetPlayerHealth(int playerid)
	{
		return Get1f(N::GetPlayerHealth, playerid);
	}

	float GetPlayerArmour(int playerid)
	{
		return Get1f(N::GetPlayerArmour, playerid);
	}

	bool SpawnPlayer(int playerid)
	{
		return Call(N::SpawnPlayer, {playerid}) != 0;
	}

	bool SetPlayerPosFindZ(int playerid, float x, float y, float z)
	{
		return Call(N::SetPlayerPosFindZ, {playerid, F(x), F(y), F(z)}) != 0;
	}

	bool PlayerSpectatePlayer(int playerid, int targetid, int mode)
	{
		return Call(N::PlayerSpectatePlayer, {playerid, targetid, mode}) != 0;
	}

	bool DestroyVehicle(int vehicleid)
	{
		return Call(N::DestroyVehicle, {vehicleid}) != 0;
	}

	bool AllowPlayerTeleport(int playerid, bool allow)
	{
		return Call(N::AllowPlayerTeleport, {playerid, allow}) != 0;
	}

	bool IsPlayerTeleportAllowed(int playerid)
	{
		return Call(N::IsPlayerTeleportAllowed, {playerid}) != 0;
	}
}
