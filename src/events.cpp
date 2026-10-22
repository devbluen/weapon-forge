// Player/vehicle event handling forwarded by the include, script lifecycle
// and the periodic work done in ProcessTick.
#include "amxhooks.hpp"
#include "core.hpp"
#include "scripts.hpp"
#include "server.hpp"
#include "weapons.hpp"

#include <cmath>
#include <ctime>

namespace wf
{
	namespace
	{
		constexpr float kHealthBase = 8000000.0f;
		uint32_t g_last_periodic = 0;

		float AngleBetweenPoints(float x1, float y1, float x2, float y2)
		{
			return -(90.0f - std::atan2(y1 - y2, x1 - x2) * 180.0f / 3.14159265358979323846f);
		}

		// Real health is kept at 8000000 + x while the plugin is active
		float ReadRealHealth(int playerid)
		{
			float health = server::GetPlayerHealth(playerid);

			if (health >= kHealthBase - 1000.0f)
			{
				health -= kHealthBase;
			}

			return health;
		}

		void InitConnectedPlayer(int playerid, uint32_t tick)
		{
			Player &p = P(playerid);

			SetFakeHealth(playerid, 255);
			SetFakeArmour(playerid, 255);

			p.team = server::GetPlayerTeam(playerid);
			server::SetPlayerTeam(playerid, p.team);
			FreezeSyncPacket(playerid, false);
			SetFakeFacingAngle(playerid);
			DamageFeedUpdate(playerid);

			int world = server::GetPlayerVirtualWorld(playerid);

			if (world == kDeathWorld)
			{
				world = 0;
				server::SetPlayerVirtualWorld(playerid, world);
			}

			p.world = world;
			p.last_update_tick = tick;
			p.feed_update_tick = tick;
			p.last_stop_tick = tick;
			p.last_vehicle_enter_time = 0;
			p.cbug_allowed = g_config.cbug_global;

			if (g_config.custom_vending_machines)
			{
				p.already_connected = true;
				p.first_spawn = true;
				RemoveDefaultVendingMachines(playerid);
			}

			int state = server::GetPlayerState(playerid);

			if (state >= PLAYER_STATE_ONFOOT && state <= PLAYER_STATE_PASSENGER)
			{
				p.health = ReadRealHealth(playerid);
				p.armour = server::GetPlayerArmour(playerid);

				if (p.health <= 0.0f || p.health > p.max_health)
				{
					p.health = p.max_health;
				}

				if (p.armour > p.max_armour)
				{
					p.armour = p.max_armour;
				}

				UpdateHealthBar(playerid);
			}

			switch (state)
			{
			case PLAYER_STATE_ONFOOT:
			case PLAYER_STATE_DRIVER:
			case PLAYER_STATE_PASSENGER:
			case PLAYER_STATE_SPAWNED:
				p.true_death = false;
				p.in_class_selection = false;
				SetHealthBarVisible(playerid, true);
				break;

			default:
				p.true_death = true;
				p.in_class_selection = true;
				SetHealthBarVisible(playerid, false);
			}
		}
	}

	void ResetPlayer(int playerid)
	{
		g_players[playerid] = Player();
	}

	void ScriptInit()
	{
		std::string missing = server::Missing();

		if (!missing.empty())
		{
			Log("ERROR: server natives not found: %s", missing.c_str());
		}

		int mode = g_open_mp ? server::GetConsoleVarAsInt("game.lag_compensation_mode") : server::GetConsoleVarAsInt("lagcompmode");
		g_config.lag_comp_mode = mode;

		if (mode)
		{
			g_config.knife_sync = false;
		}
		else
		{
			g_config.damage_taken_sound = 0;
			g_config.knife_sync = true;
		}

		if (g_config.custom_vending_machines)
		{
			CreateVendingMachines();
		}

		g_max_players = server::GetMaxPlayers();

		if (g_max_players <= 0 || g_max_players > kMaxPlayers)
		{
			g_max_players = kMaxPlayers;
		}

		uint32_t tick = Tick();

		for (int i = 0; i < g_max_players; ++i)
		{
			if (!server::IsPlayerConnected(i))
			{
				if (P(i).connected)
				{
					ResetPlayer(i);
				}
				continue;
			}

			Player &p = P(i);

			if (!p.connected)
			{
				ResetPlayer(i);
				p.connected = true;
				p.npc = server::IsPlayerNPC(i);
			}

			InitConnectedPlayer(i, tick);
		}

		Log("Lag compensation: %s | open.mp: %s | max players: %d", mode ? "on" : "off", g_open_mp ? "yes" : "no", g_max_players);
	}

	void ScriptExit()
	{
		g_config.knife_sync = true;
		DestroyVendingMachines();

		for (int i = 0; i < g_max_players; ++i)
		{
			Player &p = P(i);

			if (!p.connected)
			{
				continue;
			}

			// Put things back the way they were
			server::SetPlayerTeam(i, p.team);

			int state = server::GetPlayerState(i);

			if (state >= PLAYER_STATE_ONFOOT && state <= PLAYER_STATE_PASSENGER)
			{
				server::SetPlayerHealth(i, p.health == 0.0f ? p.max_health : p.health);
				server::SetPlayerArmour(i, p.armour);
			}

			SetFakeHealth(i, 255);
			SetFakeArmour(i, 255);
			UpdatePlayerVirtualWorld(i);
			FreezeSyncPacket(i, false);
			SetFakeFacingAngle(i);
			DestroyHud(i);
			p.last_sent_health = -1;
			p.last_sent_armour = -1;

			// Every player goes back to class selection
			ClearDying(i);
			p.bleed_until = 0;
			p.protected_until = 0;
		}

		// The next gamemode starts from the defaults, like weapon-config
		timers::Clear();
		weapons::Reset();
		g_config = Config();

		for (Vehicle &v : g_vehicles)
		{
			v = Vehicle();
		}
	}

	void UpdatePlayerVirtualWorld(int playerid)
	{
		Player &p = P(playerid);
		int world = server::GetPlayerVirtualWorld(playerid);

		if (world == kDeathWorld)
		{
			world = p.world;
		}
		else if (world != p.world)
		{
			p.world = world;
		}

		server::SetPlayerVirtualWorld(playerid, world);
	}

	void SetHealth(int playerid, float health, float armour)
	{
		Player &p = P(playerid);

		if (health <= 0.0f)
		{
			p.armour = 0.0f;
			p.health = 0.0f;

			if (p.npc)
			{
				UpdateHealthBar(playerid, true);
			}
			else
			{
				InflictDamage(playerid, 0.0f, kInvalidPlayer, weapons::UNKNOWN, 0, false, true);
			}
			return;
		}

		if (armour >= 0.0f)
		{
			p.armour = armour > p.max_armour ? p.max_armour : armour;
		}

		p.health = health > p.max_health ? p.max_health : health;
		UpdateHealthBar(playerid, true);
	}

	void SetArmour(int playerid, float armour)
	{
		Player &p = P(playerid);

		if (armour <= 0.0f)
		{
			p.armour = 0.0f;
		}
		else
		{
			p.armour = armour > p.max_armour ? p.max_armour : armour;
		}

		UpdateHealthBar(playerid, true);
	}

	void OnPlayerConnect(int playerid)
	{
		// A cleanup still pending from the previous player with this id
		if (timers::Active(timers::DISCONNECT_CLEANUP, playerid))
		{
			timers::Stop(timers::DISCONNECT_CLEANUP, playerid);
			CleanupPlayer(playerid);
		}

		bool already_connected = P(playerid).already_connected;
		uint32_t tick = Tick();

		timers::StopAll(playerid);
		ResetPlayer(playerid);

		Player &p = P(playerid);
		p.connected = true;
		p.npc = server::IsPlayerNPC(playerid);
		p.last_update_tick = tick;
		p.feed_update_tick = tick;
		p.last_stop_tick = tick;
		p.cbug_allowed = g_config.cbug_global;

		server::SetPlayerTeam(playerid, p.team);
		FreezeSyncPacket(playerid, false);
		SetFakeFacingAngle(playerid);
		DamageFeedUpdate(playerid);

		if (g_config.custom_vending_machines)
		{
			if (!already_connected)
			{
				RemoveDefaultVendingMachines(playerid);
			}

			p.first_spawn = true;
		}
	}

	void OnPlayerDisconnect(int playerid)
	{
		// The scripts' OnPlayerDisconnect still runs after this (and may read
		// the player's health), so the state is cleared on the next tick.
		timers::StopAll(playerid);
		timers::Start(timers::DISCONNECT_CLEANUP, playerid, 0);
	}

	void CleanupPlayer(int playerid)
	{
		DestroyHud(playerid);

		for (int v = 1; v <= kMaxVehicles; ++v)
		{
			if (g_vehicles[v].last_shooter == playerid)
			{
				g_vehicles[v].last_shooter = kInvalidPlayer;
			}
		}

		for (int i = 0; i < g_max_players; ++i)
		{
			Player &other = P(i);

			if (other.spectating == playerid)
			{
				other.spectating = kInvalidPlayer;
			}

			if (other.last_attacker == playerid)
			{
				other.last_attacker = kInvalidPlayer;
			}

			if (other.bleed_issuer == playerid)
			{
				other.bleed_issuer = kInvalidPlayer;
			}

			for (HitInfo &hit : other.prev_hits)
			{
				if (hit.issuer == playerid)
				{
					hit.issuer = kInvalidPlayer;
				}
			}
		}

		ResetPlayer(playerid);
	}

	int OnPlayerSpawn(int playerid)
	{
		Player &p = P(playerid);
		uint32_t tick = Tick();

		p.true_death = false;
		p.in_class_selection = false;
		p.cbug_froze = 0;

		timers::Stop(timers::DEATH_RESPAWN, playerid);
		timers::Stop(timers::KNIFE_ANIM, playerid);
		timers::Stop(timers::DELAYED_DEATH, playerid);
		timers::Stop(timers::REAL_DEATH_FINISHED, playerid);

		p.last_update_tick = tick;
		p.last_stop_tick = tick;

		if (p.dying)
		{
			ClearDying(playerid);
		}

		if (p.health == 0.0f)
		{
			p.health = p.max_health;
		}

		p.bleed_until = 0;

		if (g_config.spawn_protection > 0)
		{
			p.protected_until = tick + static_cast<uint32_t>(g_config.spawn_protection);
		}

		UpdatePlayerVirtualWorld(playerid);
		UpdateHealthBar(playerid, true);
		FreezeSyncPacket(playerid, false);
		SetFakeFacingAngle(playerid);
		DamageFeedUpdate(playerid);

		if (server::GetPlayerTeam(playerid) != p.team)
		{
			server::SetPlayerTeam(playerid, p.team);
		}

		if (g_config.custom_vending_machines && p.first_spawn)
		{
			p.first_spawn = false;
			server::ApplyAnimation(playerid, "VENDING", "null", 0.0f, false, false, false, false, 0, 0);
		}

		return 1;
	}

	int OnPlayerRequestClass(int playerid)
	{
		Player &p = P(playerid);

		timers::Stop(timers::DEATH_RESPAWN, playerid);
		timers::Stop(timers::KNIFE_ANIM, playerid);

		if (p.dying)
		{
			if (!p.true_death)
			{
				CallDeathFinished(playerid, false);
			}

			ClearDying(playerid);
		}

		if (!p.in_class_selection)
		{
			float x, y, z;
			server::GetPlayerPos(playerid, x, y, z);

			// Bottles that appear in class selection after a death
			server::RemoveBuildingForPlayer(playerid, 1484, x, y, z, 350.0f);
			server::RemoveBuildingForPlayer(playerid, 1485, x, y, z, 350.0f);
			server::RemoveBuildingForPlayer(playerid, 1486, x, y, z, 350.0f);

			p.in_class_selection = true;
		}

		UpdatePlayerVirtualWorld(playerid);
		return 1;
	}

	int OnPlayerKeyStateChange(int playerid, int newkeys, int oldkeys)
	{
		Player &p = P(playerid);
		int state = server::GetPlayerState(playerid);

		if (!p.cbug_allowed && !p.dying && state == PLAYER_STATE_ONFOOT && (newkeys & KEY_CROUCH))
		{
			uint32_t tick = Tick();
			int diff = TickDiff(tick, p.last_shot.tick);

			if (p.last_shot.tick && diff < 1200 && !p.cbug_froze)
			{
				cell punish = scripts::Call("OnPlayerCbug").Int(playerid).Int(p.last_shot.weapon).Int(diff).Exec(1, true);

				if (punish)
				{
					server::PlayerPlaySound(playerid, 1055, 0.0f, 0.0f, 0.0f);

					if (p.last_shot.valid && std::fabs(p.last_shot.hx) > 1.0f && std::fabs(p.last_shot.hy) > 1.0f)
					{
						server::SetPlayerFacingAngle(playerid, AngleBetweenPoints(p.last_shot.hx, p.last_shot.hy, p.last_shot.ox, p.last_shot.oy));
					}

					int slot_weapon, slot_ammo;
					server::GetPlayerWeaponData(playerid, 0, slot_weapon, slot_ammo);

					server::ClearAnimations(playerid, 1);
					server::ApplyAnimation(playerid, "PED", "IDLE_stance", 4.1f, true, false, false, false, 0, 1);
					FreezeSyncPacket(playerid, true);
					server::SetPlayerArmedWeapon(playerid, slot_weapon);

					timers::Args args;
					args.a = server::GetPlayerWeapon(playerid);
					timers::Start(timers::CBUG_PUNISH, playerid, 600, args);
					p.cbug_froze = tick;

					// Undo the damage done by the c-bugger in the last 1200ms
					for (int i = 0; i < g_max_players; ++i)
					{
						Player &other = P(i);

						if (!other.connected)
						{
							continue;
						}

						for (HitInfo &hit : other.prev_hits)
						{
							if (hit.issuer != playerid || TickDiff(tick, hit.tick) > 1200)
							{
								continue;
							}

							hit.issuer = kInvalidPlayer;

							float health = other.dying ? 0.0f : other.health;
							float armour = other.dying ? 0.0f : other.armour;

							if (other.dying)
							{
								if (!timers::Active(timers::DELAYED_DEATH, i))
								{
									continue;
								}

								timers::Stop(timers::DELAYED_DEATH, i);
								server::ClearAnimations(i, 1);
								SetFakeFacingAngle(i);
								FreezeSyncPacket(i, false);
								ClearDying(i);
								timers::Stop(timers::DEATH_RESPAWN, i);
							}

							SetHealth(i, health + hit.health, armour + hit.armour);
						}
					}
				}
			}
		}

		if (state == PLAYER_STATE_ONFOOT)
		{
			if (g_config.custom_vending_machines && newkeys == KEY_SECONDARY_ATTACK && !oldkeys && !timers::Active(timers::VENDING_USE, playerid) && server::GetPlayerAnimationIndex(playerid) != 1660)
			{
				TryUseVendingMachine(playerid);
			}

			if (newkeys & KEY_FIRE)
			{
				int weapon = server::GetPlayerWeapon(playerid);

				switch (weapon)
				{
				case weapons::BOMB:
				case weapons::SATCHEL:
					p.last_explosive = weapons::SATCHEL;
					break;

				case weapons::ROCKETLAUNCHER:
				case weapons::HEATSEEKER:
				case weapons::GRENADE:
					p.last_explosive = weapon;
					break;
				}
			}
		}

		return p.dying ? 0 : 1;
	}

	void OnPlayerEnterVehicle(int playerid)
	{
		Player &p = P(playerid);

		timers::Stop(timers::VENDING_USE, playerid);
		p.last_vehicle_enter_time = static_cast<int64_t>(std::time(nullptr));
		p.last_vehicle_tick = Tick();

		if (p.dying)
		{
			server::TogglePlayerControllable(playerid, false);
			server::ApplyAnimation(playerid, "PED", "KO_skid_back", 4.1f, false, false, false, true, 0, 1);
		}
	}

	void OnPlayerExitVehicle(int playerid)
	{
		P(playerid).last_vehicle_tick = Tick();
	}

	void OnPlayerStateChange(int playerid, int newstate, int oldstate)
	{
		Player &p = P(playerid);

		if (p.spectating != kInvalidPlayer && newstate != PLAYER_STATE_SPECTATING)
		{
			p.spectating = kInvalidPlayer;
		}

		timers::Stop(timers::VENDING_USE, playerid);

		if (p.dying && (newstate == PLAYER_STATE_DRIVER || newstate == PLAYER_STATE_PASSENGER))
		{
			server::TogglePlayerControllable(playerid, false);
		}

		if (oldstate == PLAYER_STATE_DRIVER || oldstate == PLAYER_STATE_PASSENGER)
		{
			p.last_vehicle_tick = Tick();
		}

		switch (newstate)
		{
		case PLAYER_STATE_ONFOOT:
		case PLAYER_STATE_DRIVER:
		case PLAYER_STATE_PASSENGER:
			SetHealthBarVisible(playerid, true);
			break;

		default:
			SetHealthBarVisible(playerid, false);
		}
	}

	int OnPlayerUpdate(int playerid)
	{
		Player &p = P(playerid);

		if (p.temp_written)
		{
			CommitSyncData(playerid);
		}

		// 2: keep the update, but don't call the script's OnPlayerUpdate
		if (p.dying)
		{
			return 2;
		}

		uint32_t tick = Tick();
		p.last_update_tick = tick;

		// Detect fall damage based on velocity and animation
		if (!g_config.custom_fall_damage)
		{
			return 1;
		}

		float vx, vy, vz, x, y, z;
		server::GetPlayerVelocity(playerid, vx, vy, vz);
		server::GetPlayerPos(playerid, x, y, z);

		bool surfing = server::GetPlayerSurfingVehicleID(playerid) != kInvalidVehicle || server::GetPlayerSurfingObjectID(playerid) != kInvalidObject;

		if (surfing || TickDiff(tick, p.last_stop_tick) < 2000)
		{
			vz = 0.0f;
			p.last_z_velo = 0.0f;
		}
		else
		{
			if (vz != 0.0f)
			{
				p.last_z_velo = vz;
			}

			if (z - p.last_z > 1.0f)
			{
				p.last_z_velo = 0.1f;
				vz = 0.1f;
			}
		}

		p.last_z = z;

		int anim = server::GetPlayerAnimationIndex(playerid);

		if (anim != p.last_anim)
		{
			int prev = p.last_anim;
			p.last_anim = anim;

			if ((prev == 1130 && vz == 0.0f) || (anim >= 1128 && anim <= 1134) || anim == 1208)
			{
				float amount = -1.0f;
				float multiplier = weapons::Get(weapons::COLLISION)->damage;

				vz = p.last_z_velo;

				if (vz <= g_config.fall_death_velocity)
				{
					amount = 0.0f;
				}
				else if (vz <= -0.2f)
				{
					if (vz == -0.2f)
					{
						amount = multiplier * 0.2f;
					}
					else
					{
						amount = (vz + 0.2f) / (g_config.fall_death_velocity + 0.2f) * multiplier;
					}
				}

				if (server::GetPlayerWeapon(playerid) == weapons::PARACHUTE && anim == 1134)
				{
					amount = -1.0f;
				}

				if (amount >= 0.0f)
				{
					Debug("%d: fall damage %.5f (vz: %f, anim: %d, prev: %d)", playerid, amount, vz, anim, prev);
					InflictDamage(playerid, amount, kInvalidPlayer, weapons::COLLISION, 3);
				}
			}
		}

		return 1;
	}

	void OnVehicleSpawn(int vehicleid)
	{
		if (!ValidVehicle(vehicleid))
		{
			return;
		}

		timers::Stop(timers::VEHICLE_KILL, vehicleid);
		timers::Stop(timers::VEHICLE_RESPAWN, vehicleid);
		g_vehicles[vehicleid] = Vehicle();
	}

	int OnVehicleDeath(int vehicleid)
	{
		if (!ValidVehicle(vehicleid))
		{
			return 1;
		}

		timers::Stop(timers::VEHICLE_KILL, vehicleid);
		timers::Stop(timers::VEHICLE_RESPAWN, vehicleid);

		Vehicle &v = g_vehicles[vehicleid];

		if (v.death_reported)
		{
			return 0;
		}

		v.death_reported = true;
		return 1;
	}

	void ProcessPeriodic(uint32_t now)
	{
		float dt = static_cast<float>(TickDiff(now, g_last_periodic)) / 1000.0f;
		g_last_periodic = now;

		if (dt <= 0.0f || dt > 1.0f)
		{
			dt = 0.1f;
		}

		for (int i = 0; i < g_max_players; ++i)
		{
			Player &p = P(i);

			if (!p.connected)
			{
				continue;
			}

			// Expire timestamps before they wrap around (24.8 days)
			if (p.protected_until && TickDiff(now, p.protected_until) >= 0)
			{
				p.protected_until = 0;
			}

			if (p.last_damage_tick && TickDiff(now, p.last_damage_tick) > 0x40000000)
			{
				p.last_damage_tick = 0;
			}

			// Combat state
			if (p.in_combat && TickDiff(now, p.last_damage_tick) > g_config.combat_timeout)
			{
				p.in_combat = false;
				scripts::Call("OnPlayerLeaveCombat").Int(i).Exec();
			}

			if (p.dying || p.npc)
			{
				continue;
			}

			// Bleeding (damage over time)
			if (p.bleed_until)
			{
				if (TickDiff(now, p.bleed_until) >= 0)
				{
					p.bleed_until = 0;
				}
				else if (TickDiff(now, p.bleed_next) >= 0)
				{
					p.bleed_next += 1000;
					InflictDamage(i, p.bleed_dps, p.bleed_issuer, p.bleed_weapon, 3, true);

					if (p.dying)
					{
						continue;
					}
				}
			}

			// Health regeneration
			float rate = p.regen_rate >= 0.0f ? p.regen_rate : g_config.regen_rate;

			if (rate > 0.0f && p.health > 0.0f && p.health < p.max_health)
			{
				int delay = p.regen_delay >= 0 ? p.regen_delay : g_config.regen_delay;

				if (!p.last_damage_tick || TickDiff(now, p.last_damage_tick) >= delay)
				{
					if (IsPlayerSpawned(i))
					{
						p.health += rate * dt;

						if (p.health > p.max_health)
						{
							p.health = p.max_health;
						}

						UpdateHealthBar(i);
					}
				}
			}
		}
	}

	void ProcessTick()
	{
		uint32_t now = Tick();

		amxhooks::NextFrame();

		timers::Process(now);

		if (TickDiff(now, g_last_periodic) >= 100)
		{
			ProcessPeriodic(now);
		}
	}

	void OnTimer(timers::Type type, int id, const timers::Args &args)
	{
		switch (type)
		{
		case timers::DEATH_RESPAWN:
			if (P(id).connected)
			{
				DeathRespawn(id);
			}
			break;

		case timers::DELAYED_DEATH:
			if (P(id).connected)
			{
				EmitDeath(id, args.a, args.b);
			}
			break;

		case timers::CBUG_PUNISH:
			if (P(id).connected)
			{
				FreezeSyncPacket(id, false);
				server::ClearAnimations(id, 1);
				server::SetPlayerArmedWeapon(id, args.a);
			}
			break;

		case timers::KNIFE_ANIM:
			if (P(id).connected)
			{
				server::ApplyAnimation(id, "KNIFE", "KILL_Knife_Ped_Die", 4.1f, false, true, true, true, 3000, 1);
			}
			break;

		case timers::VENDING_USE:
			if (P(id).connected)
			{
				FinishVendingMachine(id, args.f);
			}
			break;

		case timers::FEED_UPDATE:
			if (P(id).connected && IsDamageFeedActive(id))
			{
				DamageFeedUpdate(id, true);
			}
			break;

		case timers::REAL_DEATH_FINISHED:
			if (P(id).connected)
			{
				CallDeathFinished(id, false);
			}
			break;

		case timers::VEHICLE_KILL:
		{
			Vehicle &v = g_vehicles[id];
			v.kill_pending = false;

			if (!v.death_reported)
			{
				v.death_reported = true;

				bool previous = g_emitting;
				g_emitting = true;
				scripts::Call("OnVehicleDeath").Int(id).Int(args.a).Exec();
				g_emitting = previous;
			}

			timers::Start(timers::VEHICLE_RESPAWN, id, 10000);
			break;
		}

		case timers::DISCONNECT_CLEANUP:
			CleanupPlayer(id);
			break;

		case timers::VEHICLE_RESPAWN:
			server::SetVehicleToRespawn(id);
			break;

		default:
			break;
		}
	}
}
