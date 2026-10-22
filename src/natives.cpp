#include "natives.hpp"

#include "amx_api.hpp"
#include "core.hpp"
#include "server.hpp"
#include "version.hpp"
#include "weapons.hpp"

#include <cmath>
#include <cstring>

namespace wf
{
	namespace
	{
		bool Args(cell *params, int count)
		{
			return params[0] >= static_cast<cell>(count * sizeof(cell));
		}

		float F(cell *params, int i)
		{
			return amx_ctof(params[i]);
		}

		bool Connected(int playerid)
		{
			return ValidPlayer(playerid) && P(playerid).connected;
		}

		void SetRef(AMX *amx, cell addr, cell value)
		{
			if (cell *p = amx::Addr(amx, addr))
			{
				*p = value;
			}
		}

		void SetRefF(AMX *amx, cell addr, float value)
		{
			SetRef(amx, addr, amx_ftoc(value));
		}

		cell GetRef(AMX *amx, cell addr)
		{
			cell *p = amx::Addr(amx, addr);
			return p ? *p : 0;
		}

		bool IsVisibleState(int playerid)
		{
			int state = server::GetPlayerState(playerid);
			return state == PLAYER_STATE_ONFOOT || state == PLAYER_STATE_DRIVER || state == PLAYER_STATE_PASSENGER;
		}

		void RefreshAllHealthBars()
		{
			for (int i = 0; i < g_max_players; ++i)
			{
				if (P(i).connected)
				{
					UpdateHealthBarSize(i);
				}
			}
		}

#define NATIVE(name) cell name([[maybe_unused]] AMX *amx, [[maybe_unused]] cell *params)
#define REQUIRE(count) \
	if (!Args(params, count)) \
	{ \
		Log("%s: expected %d parameters", __func__, count); \
		return 0; \
	}

		// ---------------------------------------------------------------
		// Replacements for server natives. The plugin swaps them in when the
		// server registers its natives, so every script uses them.
		// ---------------------------------------------------------------

		NATIVE(WF_SetPlayerHealth)
		{
			REQUIRE(2);
			int playerid = params[1];

			if (!Connected(playerid))
			{
				return 0;
			}

			SetHealth(playerid, F(params, 2), Args(params, 3) ? F(params, 3) : -1.0f);
			return 1;
		}

		// GetPlayerHealth/GetPlayerArmour keep the server's return value (1/0)
		NATIVE(Hook_GetPlayerHealth)
		{
			REQUIRE(2);
			int playerid = params[1];
			bool ok = Connected(playerid);

			SetRefF(amx, params[2], ok ? P(playerid).health : 0.0f);
			return ok ? 1 : 0;
		}

		NATIVE(Hook_GetPlayerArmour)
		{
			REQUIRE(2);
			int playerid = params[1];
			bool ok = Connected(playerid);

			SetRefF(amx, params[2], ok ? P(playerid).armour : 0.0f);
			return ok ? 1 : 0;
		}

		NATIVE(WF_GetPlayerHealth)
		{
			REQUIRE(2);
			int playerid = params[1];
			float health = Connected(playerid) ? P(playerid).health : 0.0f;

			SetRefF(amx, params[2], health);
			return amx_ftoc(health);
		}

		NATIVE(WF_SetPlayerArmour)
		{
			REQUIRE(2);
			int playerid = params[1];

			if (!Connected(playerid))
			{
				return 0;
			}

			SetArmour(playerid, F(params, 2));
			return 1;
		}

		NATIVE(WF_GetPlayerArmour)
		{
			REQUIRE(2);
			int playerid = params[1];
			float armour = Connected(playerid) ? P(playerid).armour : 0.0f;

			SetRefF(amx, params[2], armour);
			return amx_ftoc(armour);
		}

		NATIVE(WF_SetPlayerTeam)
		{
			REQUIRE(2);
			int playerid = params[1];

			if (!ValidPlayer(playerid))
			{
				return 0;
			}

			P(playerid).team = params[2];
			server::SetPlayerTeam(playerid, params[2]);
			return 1;
		}

		NATIVE(WF_GetPlayerTeam)
		{
			REQUIRE(1);
			int playerid = params[1];
			return Connected(playerid) ? P(playerid).team : kNoTeam;
		}

		NATIVE(WF_SpawnPlayer)
		{
			REQUIRE(1);
			int playerid = params[1];

			if (!ValidPlayer(playerid) || P(playerid).dying)
			{
				return 0;
			}

			if (P(playerid).health == 0.0f)
			{
				P(playerid).health = P(playerid).max_health;
			}

			server::SpawnPlayer(playerid);
			return 1;
		}

		NATIVE(WF_GetPlayerState)
		{
			REQUIRE(1);
			int playerid = params[1];

			if (!ValidPlayer(playerid))
			{
				return PLAYER_STATE_NONE;
			}

			if (P(playerid).dying)
			{
				return PLAYER_STATE_WASTED;
			}

			return server::GetPlayerState(playerid);
		}

		NATIVE(WF_SendDeathMessage)
		{
			REQUIRE(3);
			return server::SendDeathMessage(params[1], params[2], weapons::DeathMessageId(params[3])) ? 1 : 0;
		}

		NATIVE(WF_ApplyAnimation)
		{
			REQUIRE(10);
			int playerid = params[1];

			if (!ValidPlayer(playerid) || P(playerid).dying)
			{
				return 0;
			}

			std::string lib = amx::ReadString(amx, params[2]);
			std::string name = amx::ReadString(amx, params[3]);

			return server::ApplyAnimation(playerid, lib.c_str(), name.c_str(), F(params, 4), params[5] != 0, params[6] != 0, params[7] != 0, params[8] != 0, params[9], params[10]) ? 1 : 0;
		}

		NATIVE(WF_ClearAnimations)
		{
			REQUIRE(2);
			int playerid = params[1];

			if (!ValidPlayer(playerid) || P(playerid).dying)
			{
				return 0;
			}

			P(playerid).last_stop_tick = Tick();
			return server::ClearAnimations(playerid, params[2]) ? 1 : 0;
		}

		NATIVE(WF_TogglePlayerSpectating)
		{
			REQUIRE(2);
			int playerid = params[1];
			bool toggle = params[2] != 0;

			if (!server::TogglePlayerSpectating(playerid, toggle))
			{
				return 0;
			}

			if (toggle && ValidPlayer(playerid))
			{
				timers::Stop(timers::DELAYED_DEATH, playerid);
				timers::Stop(timers::DEATH_RESPAWN, playerid);
				timers::Stop(timers::VENDING_USE, playerid);
				ClearDying(playerid);
			}

			return 1;
		}

		NATIVE(WF_TogglePlayerControllable)
		{
			REQUIRE(2);
			int playerid = params[1];

			if (!ValidPlayer(playerid) || P(playerid).dying)
			{
				return 0;
			}

			P(playerid).last_stop_tick = Tick();
			return server::TogglePlayerControllable(playerid, params[2] != 0) ? 1 : 0;
		}

		NATIVE(WF_SetPlayerPos)
		{
			REQUIRE(4);
			int playerid = params[1];

			if (!ValidPlayer(playerid) || P(playerid).dying)
			{
				return 0;
			}

			P(playerid).last_stop_tick = Tick();
			return server::SetPlayerPos(playerid, F(params, 2), F(params, 3), F(params, 4)) ? 1 : 0;
		}

		NATIVE(WF_SetPlayerPosFindZ)
		{
			REQUIRE(4);
			int playerid = params[1];

			if (!ValidPlayer(playerid) || P(playerid).dying)
			{
				return 0;
			}

			P(playerid).last_stop_tick = Tick();
			return server::SetPlayerPosFindZ(playerid, F(params, 2), F(params, 3), F(params, 4)) ? 1 : 0;
		}

		NATIVE(WF_SetPlayerVelocity)
		{
			REQUIRE(4);
			int playerid = params[1];

			if (!ValidPlayer(playerid) || P(playerid).dying)
			{
				return 0;
			}

			float x = F(params, 2), y = F(params, 3), z = F(params, 4);

			if (x == 0.0f && y == 0.0f && z == 0.0f)
			{
				P(playerid).last_stop_tick = Tick();
			}

			return server::SetPlayerVelocity(playerid, x, y, z) ? 1 : 0;
		}

		NATIVE(WF_SetPlayerVirtualWorld)
		{
			REQUIRE(2);
			int playerid = params[1];

			if (!ValidPlayer(playerid))
			{
				return 0;
			}

			P(playerid).world = params[2];

			if (P(playerid).dying)
			{
				return 1;
			}

			return server::SetPlayerVirtualWorld(playerid, params[2]) ? 1 : 0;
		}

		NATIVE(WF_GetPlayerVirtualWorld)
		{
			REQUIRE(1);
			int playerid = params[1];

			if (!ValidPlayer(playerid))
			{
				return 0;
			}

			int world = server::GetPlayerVirtualWorld(playerid);
			return world == kDeathWorld ? P(playerid).world : world;
		}

		NATIVE(WF_PlayerSpectatePlayer)
		{
			REQUIRE(3);
			int playerid = params[1];

			if (!server::PlayerSpectatePlayer(playerid, params[2], params[3]))
			{
				return 0;
			}

			if (ValidPlayer(playerid))
			{
				P(playerid).spectating = params[2];
			}

			return 1;
		}

		NATIVE(WF_IsPlayerInCheckpoint)
		{
			REQUIRE(1);
			return IsPlayerSpawned(params[1]) && server::IsPlayerInCheckpoint(params[1]) ? 1 : 0;
		}

		NATIVE(WF_IsPlayerInRaceCheckpoint)
		{
			REQUIRE(1);
			return IsPlayerSpawned(params[1]) && server::IsPlayerInRaceCheckpoint(params[1]) ? 1 : 0;
		}

		NATIVE(WF_SetPlayerSpecialAction)
		{
			REQUIRE(2);

			if (!IsPlayerSpawned(params[1]))
			{
				return 0;
			}

			return server::SetPlayerSpecialAction(params[1], params[2]) ? 1 : 0;
		}

		NATIVE(WF_DestroyVehicle)
		{
			REQUIRE(1);
			int vehicleid = params[1];

			if (!server::DestroyVehicle(vehicleid))
			{
				return 0;
			}

			if (ValidVehicle(vehicleid))
			{
				timers::Stop(timers::VEHICLE_KILL, vehicleid);
				timers::Stop(timers::VEHICLE_RESPAWN, vehicleid);
				g_vehicles[vehicleid] = Vehicle();
			}

			return 1;
		}

		// ---------------------------------------------------------------
		// weapon-config API
		// ---------------------------------------------------------------

		NATIVE(WF_SetWeaponDamage)
		{
			REQUIRE(3);
			int weaponid = params[1];
			int type = params[2];
			float amount = F(params, 3);
			weapons::Weapon *w = weapons::Get(weaponid);

			if (!w)
			{
				return 0;
			}

			if (type == weapons::DAMAGE_TYPE_RANGE || type == weapons::DAMAGE_TYPE_RANGE_MULTIPLIER)
			{
				if (!weapons::IsBullet(weaponid))
				{
					return 0;
				}

				int args = static_cast<int>(params[0] / sizeof(cell));

				if (!(args & 1))
				{
					return 0;
				}

				int steps = (args - 1) / 2;

				if (steps > weapons::kMaxDamageRanges)
				{
					return 0;
				}

				w->damage_type = type;
				w->range_steps = steps;

				for (int i = 0; i < steps; ++i)
				{
					if (i)
					{
						w->range_ranges[i] = amx_ctof(GetRef(amx, params[1 + 1 + i * 2]));
						w->range_values[i] = amx_ctof(GetRef(amx, params[1 + 2 + i * 2]));
					}
					else
					{
						w->range_values[i] = amount;
					}
				}

				return 1;
			}

			if (type == weapons::DAMAGE_TYPE_MULTIPLIER || type == weapons::DAMAGE_TYPE_STATIC)
			{
				w->damage_type = type;
				w->range_steps = 0;
				w->damage = amount;
				return 1;
			}

			return 0;
		}

		NATIVE(WF_GetWeaponDamage)
		{
			REQUIRE(1);
			weapons::Weapon *w = weapons::Get(params[1]);
			return amx_ftoc(w ? w->damage : 0.0f);
		}

		NATIVE(WF_GetWeaponDamageType)
		{
			REQUIRE(1);
			weapons::Weapon *w = weapons::Get(params[1]);
			return w ? w->damage_type : -1;
		}

		NATIVE(WF_SetCustomArmourRules)
		{
			REQUIRE(2);
			g_config.armour_rules = params[1] != 0;
			g_config.torso_rules = params[2] != 0;
			return 1;
		}

		NATIVE(WF_SetWeaponArmourRule)
		{
			REQUIRE(3);
			weapons::Weapon *w = weapons::Get(params[1]);

			if (!w)
			{
				return 0;
			}

			w->armour_affects = params[2] != 0;
			w->armour_torso = params[3] != 0;
			return 1;
		}

		NATIVE(WF_SetDamageSounds)
		{
			REQUIRE(2);
			g_config.damage_taken_sound = params[1];
			g_config.damage_given_sound = params[2];
			return 1;
		}

		NATIVE(WF_SetCbugAllowed)
		{
			REQUIRE(2);
			bool enabled = params[1] != 0;
			int playerid = params[2];

			if (ValidPlayer(playerid))
			{
				P(playerid).cbug_allowed = enabled;
			}
			else
			{
				g_config.cbug_global = enabled;

				for (auto &p : g_players)
				{
					p.cbug_allowed = enabled;
				}
			}

			return enabled ? 1 : 0;
		}

		NATIVE(WF_GetCbugAllowed)
		{
			REQUIRE(1);
			int playerid = params[1];
			return (ValidPlayer(playerid) ? P(playerid).cbug_allowed : g_config.cbug_global) ? 1 : 0;
		}

		NATIVE(WF_SetCbugDeathDelay)
		{
			REQUIRE(1);
			g_config.cbug_death_delay = params[1] != 0;
			return 1;
		}

		NATIVE(WF_SetCustomFallDamage)
		{
			REQUIRE(3);
			g_config.custom_fall_damage = params[1] != 0;

			if (g_config.custom_fall_damage)
			{
				weapons::Get(weapons::COLLISION)->damage = F(params, 2);
				g_config.fall_death_velocity = -std::fabs(F(params, 3));
			}

			return 1;
		}

		NATIVE(WF_SetCustomVendingMachines)
		{
			REQUIRE(1);
			g_config.custom_vending_machines = params[1] != 0;

			if (g_config.custom_vending_machines)
			{
				CreateVendingMachines();
			}
			else
			{
				DestroyVendingMachines();
			}

			return 1;
		}

		NATIVE(WF_SetVehiclePassengerDamage)
		{
			REQUIRE(1);
			g_config.vehicle_passenger_damage = params[1] != 0;
			return 1;
		}

		NATIVE(WF_SetVehicleUnoccupiedDamage)
		{
			REQUIRE(1);
			g_config.vehicle_unoccupied_damage = params[1] != 0;
			return 1;
		}

		NATIVE(WF_SetDamageFeed)
		{
			REQUIRE(1);
			g_config.damage_feed = params[1] != 0;

			for (int i = 0; i < g_max_players; ++i)
			{
				if (P(i).connected)
				{
					DamageFeedUpdate(i);
				}
			}

			return 1;
		}

		NATIVE(WF_SetDamageFeedForPlayer)
		{
			REQUIRE(2);
			int playerid = params[1];

			if (!Connected(playerid))
			{
				return 0;
			}

			int toggle = params[2];
			P(playerid).feed_mode = toggle < 0 ? -1 : (toggle ? 1 : 0);
			DamageFeedUpdate(playerid);
			return 1;
		}

		NATIVE(WF_IsDamageFeedActive)
		{
			REQUIRE(1);
			return IsDamageFeedActive(params[1]) ? 1 : 0;
		}

		NATIVE(WF_SetDamageFeedHideDelay)
		{
			REQUIRE(1);
			g_config.feed_hide_delay = params[1] < 100 ? 100 : params[1];
			return 1;
		}

		NATIVE(WF_SetDamageFeedColors)
		{
			REQUIRE(2);
			g_config.feed_given_color = params[1];
			g_config.feed_taken_color = params[2];

			for (int i = 0; i < g_max_players; ++i)
			{
				Player &p = P(i);

				if (!p.connected)
				{
					continue;
				}

				if (p.feed_given_td != kInvalidTextDraw)
				{
					server::PlayerTextDrawColor(i, p.feed_given_td, g_config.feed_given_color);
				}

				if (p.feed_taken_td != kInvalidTextDraw)
				{
					server::PlayerTextDrawColor(i, p.feed_taken_td, g_config.feed_taken_color);
				}

				DamageFeedUpdate(i, true);
			}

			return 1;
		}

		NATIVE(WF_SetDamageFeedPosition)
		{
			REQUIRE(4);
			g_config.feed_given_pos[0] = F(params, 1);
			g_config.feed_given_pos[1] = F(params, 2);
			g_config.feed_taken_pos[0] = F(params, 3);
			g_config.feed_taken_pos[1] = F(params, 4);

			for (int i = 0; i < g_max_players; ++i)
			{
				Player &p = P(i);

				if (!p.connected)
				{
					continue;
				}

				// Recreated at the new position on the next update
				if (p.feed_given_td != kInvalidTextDraw)
				{
					server::PlayerTextDrawDestroy(i, p.feed_given_td);
					p.feed_given_td = kInvalidTextDraw;
				}

				if (p.feed_taken_td != kInvalidTextDraw)
				{
					server::PlayerTextDrawDestroy(i, p.feed_taken_td);
					p.feed_taken_td = kInvalidTextDraw;
				}

				DamageFeedUpdate(i, true);
			}

			return 1;
		}

		NATIVE(WF_SetWeaponShootRate)
		{
			REQUIRE(2);
			weapons::Weapon *w = weapons::Get(params[1]);

			if (!w)
			{
				return 0;
			}

			w->shoot_rate = params[2];
			return 1;
		}

		NATIVE(WF_GetWeaponShootRate)
		{
			REQUIRE(1);
			weapons::Weapon *w = weapons::Get(params[1]);
			return w ? w->shoot_rate : 0;
		}

		NATIVE(WF_IsPlayerDying)
		{
			REQUIRE(1);
			return Connected(params[1]) && P(params[1]).dying ? 1 : 0;
		}

		NATIVE(WF_SetWeaponMaxRange)
		{
			REQUIRE(2);

			if (!weapons::IsBullet(params[1]))
			{
				return 0;
			}

			weapons::Get(params[1])->range = F(params, 2);
			return 1;
		}

		NATIVE(WF_GetWeaponMaxRange)
		{
			REQUIRE(1);

			if (!weapons::IsBullet(params[1]))
			{
				return amx_ftoc(0.0f);
			}

			return amx_ftoc(weapons::Get(params[1])->range);
		}

		NATIVE(WF_SetPlayerMaxHealth)
		{
			REQUIRE(2);
			int playerid = params[1];
			float value = F(params, 2);

			if (!ValidPlayer(playerid) || value <= 0.0f)
			{
				return 0;
			}

			Player &p = P(playerid);
			p.max_health = value;

			if (p.health > value)
			{
				p.health = value;
			}

			UpdateHealthBar(playerid, true);
			return 1;
		}

		NATIVE(WF_SetPlayerMaxArmour)
		{
			REQUIRE(2);
			int playerid = params[1];
			float value = F(params, 2);

			if (!ValidPlayer(playerid) || value <= 0.0f)
			{
				return 0;
			}

			Player &p = P(playerid);
			p.max_armour = value;

			if (p.armour > value)
			{
				p.armour = value;
			}

			UpdateHealthBar(playerid, true);
			return 1;
		}

		NATIVE(WF_GetPlayerMaxHealth)
		{
			REQUIRE(1);
			return amx_ftoc(ValidPlayer(params[1]) ? P(params[1]).max_health : 0.0f);
		}

		NATIVE(WF_GetPlayerMaxArmour)
		{
			REQUIRE(1);
			return amx_ftoc(ValidPlayer(params[1]) ? P(params[1]).max_armour : 0.0f);
		}

		NATIVE(WF_GetLastDamageHealth)
		{
			REQUIRE(1);
			return amx_ftoc(ValidPlayer(params[1]) ? P(params[1]).damage_done_health : 0.0f);
		}

		NATIVE(WF_GetLastDamageArmour)
		{
			REQUIRE(1);
			return amx_ftoc(ValidPlayer(params[1]) ? P(params[1]).damage_done_armour : 0.0f);
		}

		NATIVE(WF_DamagePlayer)
		{
			REQUIRE(6);
			int playerid = params[1];
			float amount = F(params, 2);
			int issuerid = params[3];
			int weaponid = params[4];

			if (!Connected(playerid) || amount < 0.0f)
			{
				return 0;
			}

			if (weaponid < 0 || weaponid > weapons::UNKNOWN)
			{
				weaponid = weapons::UNKNOWN;
			}

			if (!Connected(issuerid))
			{
				issuerid = kInvalidPlayer;
			}

			InflictDamage(playerid, amount, issuerid, weaponid, params[5], params[6] != 0);
			return 1;
		}

		NATIVE(WF_GetRejectedHit)
		{
			REQUIRE(4);
			bool ok = false;
			std::string text = FormatRejectedHit(params[1], params[2], &ok);

			if (!ok)
			{
				return 0;
			}

			amx::WriteString(amx, params[3], text.c_str(), static_cast<size_t>(params[4]));
			return 1;
		}

		NATIVE(WF_ResyncPlayer)
		{
			REQUIRE(1);
			int playerid = params[1];

			if (!Connected(playerid))
			{
				return 0;
			}

			if (!P(playerid).dying)
			{
				server::ClearAnimations(playerid, 1);
			}

			FreezeSyncPacket(playerid, P(playerid).dying);
			UpdateHealthBar(playerid, true);
			return 1;
		}

		NATIVE(WF_SetRespawnTime)
		{
			REQUIRE(1);
			g_config.respawn_time = params[1] < 0 ? 0 : params[1];
			return 1;
		}

		NATIVE(WF_GetRespawnTime)
		{
			return g_config.respawn_time;
		}

		NATIVE(WF_SetWeaponName)
		{
			REQUIRE(2);
			weapons::Weapon *w = weapons::Get(params[1]);

			if (!w)
			{
				return 0;
			}

			w->name = amx::ReadString(amx, params[2]);
			return 1;
		}

		NATIVE(WF_GetWeaponName)
		{
			REQUIRE(3);
			std::string name = weapons::Name(params[1]);
			amx::WriteString(amx, params[2], name.c_str(), static_cast<size_t>(params[3]));
			return 1;
		}

		NATIVE(WF_IsBulletWeapon)
		{
			REQUIRE(1);
			return weapons::IsBullet(params[1]) ? 1 : 0;
		}

		NATIVE(WF_IsMeleeWeapon)
		{
			REQUIRE(1);
			return weapons::IsMelee(params[1]) ? 1 : 0;
		}

		NATIVE(WF_IsHighRateWeapon)
		{
			REQUIRE(1);
			return weapons::IsHighRate(params[1]) ? 1 : 0;
		}

		NATIVE(WF_IsPlayerSpawned)
		{
			REQUIRE(1);
			return IsPlayerSpawned(params[1]) ? 1 : 0;
		}

		NATIVE(WF_IsPlayerPaused)
		{
			REQUIRE(1);
			return Connected(params[1]) && IsPlayerPaused(params[1]) ? 1 : 0;
		}

		NATIVE(WF_AverageShootRate)
		{
			REQUIRE(3);
			bool multiple = false;
			int rate = AverageShootRate(params[1], params[2], &multiple);
			SetRef(amx, params[3], multiple ? 1 : 0);
			return rate;
		}

		NATIVE(WF_AverageHitRate)
		{
			REQUIRE(3);
			bool multiple = false;
			int rate = AverageHitRate(params[1], params[2], &multiple);
			SetRef(amx, params[3], multiple ? 1 : 0);
			return rate;
		}

		NATIVE(WF_SetKnifeSync)
		{
			REQUIRE(1);
			g_config.knife_sync = params[1] != 0;
			return 1;
		}

		NATIVE(WF_SetDisableSyncBugs)
		{
			REQUIRE(1);
			g_config.disable_sync_bugs = params[1] != 0;
			return 1;
		}

		NATIVE(WF_SetLagCompMode)
		{
			REQUIRE(1);
			g_config.lag_comp_mode = params[1];
			return 1;
		}

		NATIVE(WF_GetLagCompMode)
		{
			return g_config.lag_comp_mode;
		}

		// ---------------------------------------------------------------
		// Health bar
		// ---------------------------------------------------------------

		NATIVE(WF_EnableHealthBarForPlayer)
		{
			REQUIRE(2);
			int playerid = params[1];

			if (!Connected(playerid))
			{
				return 0;
			}

			P(playerid).health_bar_enabled = params[2] != 0;
			SetHealthBarVisible(playerid, params[2] != 0 && IsVisibleState(playerid));
			return 1;
		}

		NATIVE(WF_GetHealthBarPosition)
		{
			REQUIRE(3);
			float x, y;
			GetHealthBarPosition(params[3], x, y);
			SetRefF(amx, params[1], x);
			SetRefF(amx, params[2], y);
			return 1;
		}

		NATIVE(WF_GetHealthBarSize)
		{
			REQUIRE(3);
			float x, y;
			GetHealthBarSize(params[3], x, y);
			SetRefF(amx, params[1], x);
			SetRefF(amx, params[2], y);
			return 1;
		}

		NATIVE(WF_GetHealthBarPadding)
		{
			REQUIRE(2);
			float padding[4];
			GetHealthBarPadding(params[2], padding);

			if (cell *out = amx::Addr(amx, params[1]))
			{
				for (int i = 0; i < 4; ++i)
				{
					out[i] = amx_ftoc(padding[i]);
				}
			}

			return 1;
		}

		NATIVE(WF_SetHealthBarPosition)
		{
			REQUIRE(2);
			g_config.hb_pos[0] = F(params, 1);
			g_config.hb_pos[1] = F(params, 2);
			RefreshAllHealthBars();
			return 1;
		}

		NATIVE(WF_SetHealthBarSize)
		{
			REQUIRE(2);
			g_config.hb_size[0] = F(params, 1);
			g_config.hb_size[1] = F(params, 2);
			RefreshAllHealthBars();
			return 1;
		}

		NATIVE(WF_SetHealthBarPadding)
		{
			REQUIRE(1);

			if (cell *in = amx::Addr(amx, params[1]))
			{
				for (int i = 0; i < 4; ++i)
				{
					g_config.hb_padding[i] = amx_ctof(in[i]);
				}
			}

			RefreshAllHealthBars();
			return 1;
		}

		NATIVE(WF_SetHealthBarPosForPlayer)
		{
			REQUIRE(3);
			int playerid = params[1];

			if (!Connected(playerid))
			{
				return 0;
			}

			P(playerid).hb_pos[0] = F(params, 2);
			P(playerid).hb_pos[1] = F(params, 3);
			UpdateHealthBarSize(playerid);
			return 1;
		}

		NATIVE(WF_SetHealthBarSizeForPlayer)
		{
			REQUIRE(3);
			int playerid = params[1];

			if (!Connected(playerid))
			{
				return 0;
			}

			P(playerid).hb_size[0] = F(params, 2);
			P(playerid).hb_size[1] = F(params, 3);
			UpdateHealthBarSize(playerid);
			return 1;
		}

		NATIVE(WF_SetHealthBarPaddingForPlayer)
		{
			REQUIRE(2);
			int playerid = params[1];

			if (!Connected(playerid))
			{
				return 0;
			}

			if (cell *in = amx::Addr(amx, params[2]))
			{
				for (int i = 0; i < 4; ++i)
				{
					P(playerid).hb_padding[i] = amx_ctof(in[i]);
				}
			}

			UpdateHealthBarSize(playerid);
			return 1;
		}

		NATIVE(WF_GetHealthBarColor)
		{
			REQUIRE(4);
			int border, bg, fg;
			GetHealthBarColor(params[4], border, bg, fg);
			SetRef(amx, params[1], border);
			SetRef(amx, params[2], bg);
			SetRef(amx, params[3], fg);
			return 1;
		}

		NATIVE(WF_SetHealthBarColor)
		{
			REQUIRE(3);

			if (params[1])
			{
				g_config.hb_border_color = params[1];
			}
			if (params[2])
			{
				g_config.hb_bg_color = params[2];
			}
			if (params[3])
			{
				g_config.hb_fg_color = params[3];
			}

			RefreshAllHealthBars();
			return 1;
		}

		NATIVE(WF_SetHealthBarColorForPlayer)
		{
			REQUIRE(4);
			int playerid = params[1];

			if (!Connected(playerid))
			{
				return 0;
			}

			P(playerid).hb_border_color = params[2];
			P(playerid).hb_bg_color = params[3];
			P(playerid).hb_fg_color = params[4];
			UpdateHealthBarSize(playerid);
			return 1;
		}

		// ---------------------------------------------------------------
		// weapon-forge extras
		// ---------------------------------------------------------------

		NATIVE(WF_SetPlayerGodMode)
		{
			REQUIRE(2);

			if (!Connected(params[1]))
			{
				return 0;
			}

			P(params[1]).god_mode = params[2] != 0;
			return 1;
		}

		NATIVE(WF_IsPlayerGodMode)
		{
			REQUIRE(1);
			return Connected(params[1]) && P(params[1]).god_mode ? 1 : 0;
		}

		NATIVE(WF_SetSpawnProtection)
		{
			REQUIRE(1);
			g_config.spawn_protection = params[1] < 0 ? 0 : params[1];
			return 1;
		}

		NATIVE(WF_GetSpawnProtection)
		{
			return g_config.spawn_protection;
		}

		NATIVE(WF_SetPlayerProtection)
		{
			REQUIRE(2);

			if (!Connected(params[1]))
			{
				return 0;
			}

			P(params[1]).protected_until = params[2] > 0 ? Tick() + static_cast<uint32_t>(params[2]) : 0;
			return 1;
		}

		NATIVE(WF_GetPlayerProtectionTime)
		{
			REQUIRE(1);

			if (!Connected(params[1]) || !P(params[1]).protected_until)
			{
				return 0;
			}

			int left = TickDiff(P(params[1]).protected_until, Tick());
			return left > 0 ? left : 0;
		}

		NATIVE(WF_SetPlayerDamageMultiplier)
		{
			REQUIRE(3);

			if (!Connected(params[1]))
			{
				return 0;
			}

			P(params[1]).given_mul = F(params, 2);
			P(params[1]).taken_mul = F(params, 3);
			return 1;
		}

		NATIVE(WF_GetPlayerDamageMultiplier)
		{
			REQUIRE(3);

			if (!Connected(params[1]))
			{
				return 0;
			}

			SetRefF(amx, params[2], P(params[1]).given_mul);
			SetRefF(amx, params[3], P(params[1]).taken_mul);
			return 1;
		}

		NATIVE(WF_SetWeaponBodypartMultiplier)
		{
			REQUIRE(3);
			int weaponid = params[1];
			int bodypart = params[2];
			float mul = F(params, 3);

			if (bodypart < 0 || bodypart >= weapons::kBodyParts)
			{
				return 0;
			}

			if (weaponid == -1)
			{
				for (int i = 0; i < weapons::COUNT; ++i)
				{
					weapons::Get(i)->bodypart_mul[bodypart] = mul;
				}
				return 1;
			}

			weapons::Weapon *w = weapons::Get(weaponid);

			if (!w)
			{
				return 0;
			}

			w->bodypart_mul[bodypart] = mul;
			return 1;
		}

		NATIVE(WF_GetWeaponBodypartMultiplier)
		{
			REQUIRE(2);
			weapons::Weapon *w = weapons::Get(params[1]);
			int bodypart = params[2];

			if (!w || bodypart < 0 || bodypart >= weapons::kBodyParts)
			{
				return amx_ftoc(1.0f);
			}

			return amx_ftoc(w->bodypart_mul[bodypart]);
		}

		NATIVE(WF_SetFriendlyFire)
		{
			REQUIRE(1);
			g_config.friendly_fire = params[1] != 0;
			return 1;
		}

		NATIVE(WF_GetFriendlyFire)
		{
			return g_config.friendly_fire ? 1 : 0;
		}

		NATIVE(WF_SetCombatTimeout)
		{
			REQUIRE(1);
			g_config.combat_timeout = params[1] < 0 ? 0 : params[1];
			return 1;
		}

		NATIVE(WF_IsPlayerInCombat)
		{
			REQUIRE(1);
			return Connected(params[1]) && P(params[1]).in_combat ? 1 : 0;
		}

		NATIVE(WF_GetLastAttacker)
		{
			REQUIRE(1);
			return Connected(params[1]) ? P(params[1]).last_attacker : kInvalidPlayer;
		}

		NATIVE(WF_GetTimeSinceLastDamage)
		{
			REQUIRE(1);

			if (!Connected(params[1]) || !P(params[1]).last_damage_tick)
			{
				return -1;
			}

			return TickDiff(Tick(), P(params[1]).last_damage_tick);
		}

		NATIVE(WF_GetPlayerHitHistory)
		{
			REQUIRE(7);
			int playerid = params[1];
			int idx = params[2];

			if (!Connected(playerid) || idx < 0 || idx >= kPrevHits)
			{
				return 0;
			}

			Player &p = P(playerid);
			const HitInfo &hit = p.prev_hits[(p.prev_hit_idx + 1 + idx) % kPrevHits];

			if (!hit.tick)
			{
				return 0;
			}

			SetRef(amx, params[3], hit.issuer);
			SetRef(amx, params[4], hit.weapon);
			SetRefF(amx, params[5], hit.amount);
			SetRef(amx, params[6], hit.bodypart);
			SetRef(amx, params[7], TickDiff(Tick(), hit.tick));
			return 1;
		}

		NATIVE(WF_SetHealthRegen)
		{
			REQUIRE(2);
			g_config.regen_rate = F(params, 1);
			g_config.regen_delay = params[2] < 0 ? 0 : params[2];
			return 1;
		}

		NATIVE(WF_SetPlayerHealthRegen)
		{
			REQUIRE(3);

			if (!Connected(params[1]))
			{
				return 0;
			}

			P(params[1]).regen_rate = F(params, 2);
			P(params[1]).regen_delay = params[3];
			return 1;
		}

		NATIVE(WF_SetPlayerBleeding)
		{
			REQUIRE(5);
			int playerid = params[1];
			float dps = F(params, 2);
			int duration = params[3];

			if (!Connected(playerid))
			{
				return 0;
			}

			Player &p = P(playerid);

			if (dps <= 0.0f || duration <= 0)
			{
				p.bleed_until = 0;
				return 1;
			}

			uint32_t now = Tick();
			p.bleed_dps = dps;
			p.bleed_until = now + static_cast<uint32_t>(duration);
			p.bleed_next = now + 1000;
			p.bleed_issuer = Connected(params[4]) ? params[4] : kInvalidPlayer;
			p.bleed_weapon = (params[5] >= 0 && params[5] <= weapons::UNKNOWN) ? params[5] : weapons::UNKNOWN;
			return 1;
		}

		NATIVE(WF_StopPlayerBleeding)
		{
			REQUIRE(1);

			if (!Connected(params[1]))
			{
				return 0;
			}

			P(params[1]).bleed_until = 0;
			return 1;
		}

		NATIVE(WF_IsPlayerBleeding)
		{
			REQUIRE(1);
			return Connected(params[1]) && P(params[1]).bleed_until ? 1 : 0;
		}

		NATIVE(WF_SetAssistParams)
		{
			REQUIRE(2);
			g_config.assist_min_damage = F(params, 1);
			g_config.assist_window = params[2] < 0 ? 0 : params[2];
			return 1;
		}

		NATIVE(WF_GetPlayerStat)
		{
			REQUIRE(2);

			if (!Connected(params[1]) || params[2] < 0 || params[2] >= STAT_INT_COUNT)
			{
				return 0;
			}

			return P(params[1]).stats[params[2]];
		}

		NATIVE(WF_GetPlayerStatFloat)
		{
			REQUIRE(2);

			if (!Connected(params[1]) || params[2] < 0 || params[2] >= STATF_COUNT)
			{
				return amx_ftoc(0.0f);
			}

			return amx_ftoc(P(params[1]).stats_f[params[2]]);
		}

		NATIVE(WF_ResetPlayerStats)
		{
			REQUIRE(1);

			if (!Connected(params[1]))
			{
				return 0;
			}

			Player &p = P(params[1]);

			for (int &s : p.stats)
			{
				s = 0;
			}

			for (float &s : p.stats_f)
			{
				s = 0.0f;
			}

			return 1;
		}

		NATIVE(WF_GetPlayerAccuracy)
		{
			REQUIRE(1);

			if (!Connected(params[1]))
			{
				return amx_ftoc(0.0f);
			}

			const Player &p = P(params[1]);
			int fired = p.stats[STAT_SHOTS_FIRED];

			if (fired <= 0)
			{
				return amx_ftoc(0.0f);
			}

			float accuracy = static_cast<float>(p.stats[STAT_SHOTS_HIT]) / static_cast<float>(fired) * 100.0f;
			return amx_ftoc(accuracy > 100.0f ? 100.0f : accuracy);
		}

		NATIVE(WF_GetLastShotInfo)
		{
			REQUIRE(8);
			int playerid = params[1];

			if (!Connected(playerid) || !P(playerid).last_shot.tick)
			{
				return -1;
			}

			const ShotInfo &shot = P(playerid).last_shot;
			SetRef(amx, params[2], shot.weapon);
			SetRef(amx, params[3], shot.hittype);
			SetRef(amx, params[4], shot.hitid);
			SetRefF(amx, params[5], shot.hx);
			SetRefF(amx, params[6], shot.hy);
			SetRefF(amx, params[7], shot.hz);
			SetRefF(amx, params[8], shot.length);
			return TickDiff(Tick(), shot.tick);
		}

		NATIVE(WF_KillPlayer)
		{
			REQUIRE(3);
			int playerid = params[1];
			int killerid = Connected(params[2]) ? params[2] : kInvalidPlayer;
			int weaponid = (params[3] >= 0 && params[3] <= weapons::UNKNOWN) ? params[3] : weapons::UNKNOWN;

			if (!Connected(playerid) || !IsPlayerSpawned(playerid))
			{
				return 0;
			}

			Player &p = P(playerid);
			InflictDamage(playerid, p.health + p.armour, killerid, weaponid, 3, true, true);
			return p.dying ? 1 : 0;
		}

		NATIVE(WF_RevivePlayer)
		{
			REQUIRE(2);

			if (!Connected(params[1]))
			{
				return 0;
			}

			RevivePlayer(params[1], F(params, 2));
			return 1;
		}

		// The value stored by the server (what other clients see)
		NATIVE(WF_GetPlayerRawHealth)
		{
			REQUIRE(2);
			float health = Connected(params[1]) ? server::GetPlayerHealth(params[1]) : 0.0f;
			SetRefF(amx, params[2], health);
			return amx_ftoc(health);
		}

		NATIVE(WF_SetForwardDamageCallbacks)
		{
			REQUIRE(1);
			g_config.forward_damage_callbacks = params[1] != 0;
			return 1;
		}

		NATIVE(WF_SetDebug)
		{
			REQUIRE(1);
			g_config.debug = params[1];
			return 1;
		}

		NATIVE(WF_IsOpenMP)
		{
			return g_open_mp ? 1 : 0;
		}

		NATIVE(WF_IsRakNetActive)
		{
			return g_raknet ? 1 : 0;
		}

		NATIVE(WF_GetVersion)
		{
			REQUIRE(2);
			amx::WriteString(amx, params[1], WF_VERSION, static_cast<size_t>(params[2]));
			return WF_VERSION_NUMBER;
		}

#undef NATIVE
#undef REQUIRE

		const AMX_NATIVE_INFO kNatives[] = {

			{"WF_SetPlayerHealth", WF_SetPlayerHealth},
			{"WF_GetPlayerHealth", WF_GetPlayerHealth},
			{"WF_SetPlayerArmour", WF_SetPlayerArmour},
			{"WF_GetPlayerArmour", WF_GetPlayerArmour},
			{"WF_SetPlayerTeam", WF_SetPlayerTeam},
			{"WF_GetPlayerTeam", WF_GetPlayerTeam},
			{"WF_SpawnPlayer", WF_SpawnPlayer},
			{"WF_GetPlayerState", WF_GetPlayerState},
			{"WF_SendDeathMessage", WF_SendDeathMessage},
			{"WF_ApplyAnimation", WF_ApplyAnimation},
			{"WF_ClearAnimations", WF_ClearAnimations},
			{"WF_TogglePlayerSpectating", WF_TogglePlayerSpectating},
			{"WF_TogglePlayerControllable", WF_TogglePlayerControllable},
			{"WF_SetPlayerPos", WF_SetPlayerPos},
			{"WF_SetPlayerPosFindZ", WF_SetPlayerPosFindZ},
			{"WF_SetPlayerVelocity", WF_SetPlayerVelocity},
			{"WF_SetPlayerVirtualWorld", WF_SetPlayerVirtualWorld},
			{"WF_GetPlayerVirtualWorld", WF_GetPlayerVirtualWorld},
			{"WF_PlayerSpectatePlayer", WF_PlayerSpectatePlayer},
			{"WF_IsPlayerInCheckpoint", WF_IsPlayerInCheckpoint},
			{"WF_IsPlayerInRaceCheckpoint", WF_IsPlayerInRaceCheckpoint},
			{"WF_SetPlayerSpecialAction", WF_SetPlayerSpecialAction},
			{"WF_DestroyVehicle", WF_DestroyVehicle},

			{"WF_SetWeaponDamage", WF_SetWeaponDamage},
			{"WF_GetWeaponDamage", WF_GetWeaponDamage},
			{"WF_GetWeaponDamageType", WF_GetWeaponDamageType},
			{"WF_SetCustomArmourRules", WF_SetCustomArmourRules},
			{"WF_SetWeaponArmourRule", WF_SetWeaponArmourRule},
			{"WF_SetDamageSounds", WF_SetDamageSounds},
			{"WF_SetCbugAllowed", WF_SetCbugAllowed},
			{"WF_GetCbugAllowed", WF_GetCbugAllowed},
			{"WF_SetCbugDeathDelay", WF_SetCbugDeathDelay},
			{"WF_SetCustomFallDamage", WF_SetCustomFallDamage},
			{"WF_SetCustomVendingMachines", WF_SetCustomVendingMachines},
			{"WF_SetVehiclePassengerDamage", WF_SetVehiclePassengerDamage},
			{"WF_SetVehicleUnoccupiedDamage", WF_SetVehicleUnoccupiedDamage},
			{"WF_SetDamageFeed", WF_SetDamageFeed},
			{"WF_SetDamageFeedForPlayer", WF_SetDamageFeedForPlayer},
			{"WF_IsDamageFeedActive", WF_IsDamageFeedActive},
			{"WF_SetDamageFeedHideDelay", WF_SetDamageFeedHideDelay},
			{"WF_SetDamageFeedColors", WF_SetDamageFeedColors},
			{"WF_SetDamageFeedPosition", WF_SetDamageFeedPosition},
			{"WF_SetWeaponShootRate", WF_SetWeaponShootRate},
			{"WF_GetWeaponShootRate", WF_GetWeaponShootRate},
			{"WF_IsPlayerDying", WF_IsPlayerDying},
			{"WF_SetWeaponMaxRange", WF_SetWeaponMaxRange},
			{"WF_GetWeaponMaxRange", WF_GetWeaponMaxRange},
			{"WF_SetPlayerMaxHealth", WF_SetPlayerMaxHealth},
			{"WF_SetPlayerMaxArmour", WF_SetPlayerMaxArmour},
			{"WF_GetPlayerMaxHealth", WF_GetPlayerMaxHealth},
			{"WF_GetPlayerMaxArmour", WF_GetPlayerMaxArmour},
			{"WF_GetLastDamageHealth", WF_GetLastDamageHealth},
			{"WF_GetLastDamageArmour", WF_GetLastDamageArmour},
			{"WF_DamagePlayer", WF_DamagePlayer},
			{"WF_GetRejectedHit", WF_GetRejectedHit},
			{"WF_ResyncPlayer", WF_ResyncPlayer},
			{"WF_SetRespawnTime", WF_SetRespawnTime},
			{"WF_GetRespawnTime", WF_GetRespawnTime},
			{"WF_SetWeaponName", WF_SetWeaponName},
			{"WF_GetWeaponName", WF_GetWeaponName},
			{"WF_IsBulletWeapon", WF_IsBulletWeapon},
			{"WF_IsMeleeWeapon", WF_IsMeleeWeapon},
			{"WF_IsHighRateWeapon", WF_IsHighRateWeapon},
			{"WF_IsPlayerSpawned", WF_IsPlayerSpawned},
			{"WF_IsPlayerPaused", WF_IsPlayerPaused},
			{"WF_AverageShootRate", WF_AverageShootRate},
			{"WF_AverageHitRate", WF_AverageHitRate},
			{"WF_SetKnifeSync", WF_SetKnifeSync},
			{"WF_SetDisableSyncBugs", WF_SetDisableSyncBugs},
			{"WF_SetLagCompMode", WF_SetLagCompMode},
			{"WF_GetLagCompMode", WF_GetLagCompMode},

			{"WF_EnableHealthBarForPlayer", WF_EnableHealthBarForPlayer},
			{"WF_GetHealthBarPosition", WF_GetHealthBarPosition},
			{"WF_GetHealthBarSize", WF_GetHealthBarSize},
			{"WF_GetHealthBarPadding", WF_GetHealthBarPadding},
			{"WF_SetHealthBarPosition", WF_SetHealthBarPosition},
			{"WF_SetHealthBarSize", WF_SetHealthBarSize},
			{"WF_SetHealthBarPadding", WF_SetHealthBarPadding},
			{"WF_SetHealthBarPosForPlayer", WF_SetHealthBarPosForPlayer},
			{"WF_SetHealthBarSizeForPlayer", WF_SetHealthBarSizeForPlayer},
			{"WF_SetHealthBarPaddingForPlayer", WF_SetHealthBarPaddingForPlayer},
			{"WF_GetHealthBarColor", WF_GetHealthBarColor},
			{"WF_SetHealthBarColor", WF_SetHealthBarColor},
			{"WF_SetHealthBarColorForPlayer", WF_SetHealthBarColorForPlayer},

			{"WF_SetPlayerGodMode", WF_SetPlayerGodMode},
			{"WF_IsPlayerGodMode", WF_IsPlayerGodMode},
			{"WF_SetSpawnProtection", WF_SetSpawnProtection},
			{"WF_GetSpawnProtection", WF_GetSpawnProtection},
			{"WF_SetPlayerProtection", WF_SetPlayerProtection},
			{"WF_GetPlayerProtectionTime", WF_GetPlayerProtectionTime},
			{"WF_SetPlayerDamageMultiplier", WF_SetPlayerDamageMultiplier},
			{"WF_GetPlayerDamageMultiplier", WF_GetPlayerDamageMultiplier},
			{"WF_SetWeaponBodypartMultiplier", WF_SetWeaponBodypartMultiplier},
			{"WF_GetWeaponBodypartMultiplier", WF_GetWeaponBodypartMultiplier},
			{"WF_SetFriendlyFire", WF_SetFriendlyFire},
			{"WF_GetFriendlyFire", WF_GetFriendlyFire},
			{"WF_SetCombatTimeout", WF_SetCombatTimeout},
			{"WF_IsPlayerInCombat", WF_IsPlayerInCombat},
			{"WF_GetLastAttacker", WF_GetLastAttacker},
			{"WF_GetTimeSinceLastDamage", WF_GetTimeSinceLastDamage},
			{"WF_GetPlayerHitHistory", WF_GetPlayerHitHistory},
			{"WF_SetHealthRegen", WF_SetHealthRegen},
			{"WF_SetPlayerHealthRegen", WF_SetPlayerHealthRegen},
			{"WF_SetPlayerBleeding", WF_SetPlayerBleeding},
			{"WF_StopPlayerBleeding", WF_StopPlayerBleeding},
			{"WF_IsPlayerBleeding", WF_IsPlayerBleeding},
			{"WF_SetAssistParams", WF_SetAssistParams},
			{"WF_GetPlayerStat", WF_GetPlayerStat},
			{"WF_GetPlayerStatFloat", WF_GetPlayerStatFloat},
			{"WF_ResetPlayerStats", WF_ResetPlayerStats},
			{"WF_GetPlayerAccuracy", WF_GetPlayerAccuracy},
			{"WF_GetLastShotInfo", WF_GetLastShotInfo},
			{"WF_KillPlayer", WF_KillPlayer},
			{"WF_RevivePlayer", WF_RevivePlayer},
			{"WF_SetForwardDamageCallbacks", WF_SetForwardDamageCallbacks},
			{"WF_GetPlayerRawHealth", WF_GetPlayerRawHealth},
			{"WF_SetDebug", WF_SetDebug},
			{"WF_IsOpenMP", WF_IsOpenMP},
			{"WF_IsRakNetActive", WF_IsRakNetActive},
			{"WF_GetVersion", WF_GetVersion},
		};
	}

	namespace
	{
		const AMX_NATIVE_INFO kOverrides[] = {
			{"SetPlayerHealth", WF_SetPlayerHealth},
			{"GetPlayerHealth", Hook_GetPlayerHealth},
			{"SetPlayerArmour", WF_SetPlayerArmour},
			{"GetPlayerArmour", Hook_GetPlayerArmour},
			{"SetPlayerTeam", WF_SetPlayerTeam},
			{"GetPlayerTeam", WF_GetPlayerTeam},
			{"SpawnPlayer", WF_SpawnPlayer},
			{"GetPlayerState", WF_GetPlayerState},
			{"SendDeathMessage", WF_SendDeathMessage},
			{"GetWeaponName", WF_GetWeaponName},
			{"ApplyAnimation", WF_ApplyAnimation},
			{"ClearAnimations", WF_ClearAnimations},
			{"TogglePlayerSpectating", WF_TogglePlayerSpectating},
			{"TogglePlayerControllable", WF_TogglePlayerControllable},
			{"SetPlayerPos", WF_SetPlayerPos},
			{"SetPlayerPosFindZ", WF_SetPlayerPosFindZ},
			{"SetPlayerVelocity", WF_SetPlayerVelocity},
			{"SetPlayerVirtualWorld", WF_SetPlayerVirtualWorld},
			{"GetPlayerVirtualWorld", WF_GetPlayerVirtualWorld},
			{"PlayerSpectatePlayer", WF_PlayerSpectatePlayer},
			{"DestroyVehicle", WF_DestroyVehicle},
			{"IsPlayerInCheckpoint", WF_IsPlayerInCheckpoint},
			{"IsPlayerInRaceCheckpoint", WF_IsPlayerInRaceCheckpoint},
			{"SetPlayerSpecialAction", WF_SetPlayerSpecialAction},
		};
	}

	AMX_NATIVE NativeOverride(const char *name)
	{
		for (const auto &native : kOverrides)
		{
			if (!std::strcmp(native.name, name))
			{
				return native.func;
			}
		}

		return nullptr;
	}

	int RegisterNatives(AMX *amx)
	{
		return amx::Register(amx, kNatives, static_cast<int>(sizeof(kNatives) / sizeof(kNatives[0])));
	}
}
