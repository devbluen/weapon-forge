// Server-side deaths: death animation, OnPlayerDeath emulation and respawn.
#include "core.hpp"
#include "scripts.hpp"
#include "server.hpp"
#include "weapons.hpp"

#include <cmath>
#include <ctime>
#include <map>

namespace wf
{
	namespace
	{
		constexpr float kPi = 3.14159265358979323846f;

		float AngleBetweenPoints(float x1, float y1, float x2, float y2)
		{
			return -(90.0f - std::atan2(y1 - y2, x1 - x2) * 180.0f / kPi);
		}

		void RecordDeath(int playerid, int killerid)
		{
			Player &p = P(playerid);
			p.stats[STAT_DEATHS] += 1;

			if (ValidPlayer(killerid) && killerid != playerid && P(killerid).connected)
			{
				P(killerid).stats[STAT_KILLS] += 1;
			}

			p.bleed_until = 0;
		}

		void EmitAssists(int playerid, int killerid)
		{
			if (g_config.assist_min_damage <= 0.0f)
			{
				return;
			}

			uint32_t now = Tick();
			std::map<int, float> damage;

			for (const HitInfo &hit : P(playerid).prev_hits)
			{
				if (!hit.tick || !ValidPlayer(hit.issuer) || hit.issuer == playerid || hit.issuer == killerid)
				{
					continue;
				}

				if (TickDiff(now, hit.tick) > g_config.assist_window)
				{
					continue;
				}

				damage[hit.issuer] += hit.amount;
			}

			for (const auto &entry : damage)
			{
				if (entry.second < g_config.assist_min_damage || !P(entry.first).connected)
				{
					continue;
				}

				P(entry.first).stats[STAT_ASSISTS] += 1;

				scripts::Call("OnPlayerKillAssist")
					.Int(entry.first)
					.Int(playerid)
					.Int(killerid)
					.Float(entry.second)
					.Exec();
			}
		}
	}

	void MakePlayerFacePlayer(int playerid, int targetid, bool opposite)
	{
		if (!ValidPlayer(targetid))
		{
			return;
		}

		float x1, y1, z1, x2, y2, z2;
		server::GetPlayerPos(playerid, x1, y1, z1);
		server::GetPlayerPos(targetid, x2, y2, z2);

		float angle = AngleBetweenPoints(x2, y2, x1, y1);

		if (opposite)
		{
			angle += 180.0f;
			if (angle > 360.0f)
			{
				angle -= 360.0f;
			}
		}

		if (angle < 0.0f)
		{
			angle += 360.0f;
		}
		if (angle > 360.0f)
		{
			angle -= 360.0f;
		}

		server::SetPlayerFacingAngle(playerid, angle);
		SetFakeFacingAngle(playerid, angle);
	}

	bool IsPlayerBehindPlayer(int playerid, int targetid, float diff)
	{
		if (!ValidPlayer(playerid) || !ValidPlayer(targetid))
		{
			return false;
		}

		float x1, y1, z1, x2, y2, z2;
		server::GetPlayerPos(playerid, x1, y1, z1);
		server::GetPlayerPos(targetid, x2, y2, z2);
		float ang = server::GetPlayerFacingAngle(targetid);

		float angdiff = AngleBetweenPoints(x1, y1, x2, y2);

		if (angdiff < 0.0f)
		{
			angdiff += 360.0f;
		}
		if (angdiff > 360.0f)
		{
			angdiff -= 360.0f;
		}

		ang = ang - angdiff;

		if (ang > 180.0f)
		{
			ang -= 360.0f;
		}
		if (ang < -180.0f)
		{
			ang += 360.0f;
		}

		return std::fabs(ang) > diff;
	}

	void ClearDying(int playerid)
	{
		Player &p = P(playerid);
		p.dying = false;

		if (p.restore_teleport)
		{
			p.restore_teleport = false;
			server::AllowPlayerTeleport(playerid, true);
		}
	}

	void PlayerDeath(int playerid, const char *animlib_in, const char *animname_in, bool anim_lock, int respawn_time, bool freeze_sync, bool anim_freeze)
	{
		Player &p = P(playerid);

		p.health = 0.0f;
		p.armour = 0.0f;
		p.dying = true;
		p.bleed_until = 0;

		timers::Stop(timers::DELAYED_DEATH, playerid);
		timers::Stop(timers::CBUG_PUNISH, playerid);

		if (g_open_mp && server::Has(server::N::IsPlayerTeleportAllowed) && server::IsPlayerTeleportAllowed(playerid))
		{
			p.restore_teleport = true;
			server::AllowPlayerTeleport(playerid, false);
		}

		p.last_death_tick = Tick();

		int action = server::GetPlayerSpecialAction(playerid);

		if (action != SPECIAL_ACTION_NONE && action != SPECIAL_ACTION_DUCK)
		{
			if (action == SPECIAL_ACTION_USEJETPACK)
			{
				server::ClearAnimations(playerid, 0);
			}

			server::SetPlayerSpecialAction(playerid, SPECIAL_ACTION_NONE);

			if (action == SPECIAL_ACTION_USEJETPACK)
			{
				float vx, vy, vz;
				server::GetPlayerVelocity(playerid, vx, vy, vz);
				server::SetPlayerVelocity(playerid, vx, vy, vz);
			}
		}

		std::string animlib = animlib_in, animname = animname_in;
		cell lock = anim_lock ? 1 : 0;
		cell respawn = respawn_time;

		scripts::Call("OnPlayerPrepareDeath")
			.Int(playerid)
			.String(animlib.c_str(), 32, &animlib)
			.String(animname.c_str(), 32, &animname)
			.Ref(lock)
			.Ref(respawn)
			.Exec();

		UpdateHealthBar(playerid);
		FreezeSyncPacket(playerid, freeze_sync);

		if (respawn < 0)
		{
			respawn = g_config.respawn_time;
		}

		if (!animlib.empty() && !animname.empty())
		{
			server::ApplyAnimation(playerid, animlib.c_str(), animname.c_str(), 4.1f, false, lock != 0, lock != 0, anim_freeze, 0, 1);
		}

		timers::Start(timers::DEATH_RESPAWN, playerid, respawn);

		if (p.hb_foreground != kInvalidTextDraw)
		{
			server::PlayerTextDrawHide(playerid, p.hb_foreground);
		}

		bool previous = g_emitting;
		g_emitting = true;

		if (server::IsPlayerInCheckpoint(playerid))
		{
			scripts::Call("OnPlayerLeaveCheckpoint").Int(playerid).Exec();
		}

		if (server::IsPlayerInRaceCheckpoint(playerid))
		{
			scripts::Call("OnPlayerLeaveRaceCheckpoint").Int(playerid).Exec();
		}

		g_emitting = previous;
	}

	void KillFromDamage(int playerid, int issuerid, int weaponid, int bodypart)
	{
		int vehicleid = server::GetPlayerVehicleID(playerid);
		const char *animlib = "PED";

		if (vehicleid)
		{
			server::TogglePlayerControllable(playerid, false);

			if (IsVehicleBike(vehicleid))
			{
				float vx, vy, vz;
				server::GetVehicleVelocity(vehicleid, vx, vy, vz);

				PlayerDeath(playerid, animlib, vx * vx + vy * vy + vz * vz >= 0.4f ? "BIKE_fallR" : "BIKE_fall_off", false);
			}
			else
			{
				PlayerDeath(playerid, animlib, (server::GetPlayerVehicleSeat(playerid) & 1) ? "CAR_dead_LHS" : "CAR_dead_RHS");
			}
		}
		else if (server::GetPlayerSpecialAction(playerid) == SPECIAL_ACTION_USEJETPACK)
		{
			PlayerDeath(playerid, animlib, "KO_skid_back", false, -1, false);
		}
		else
		{
			if (static_cast<int64_t>(std::time(nullptr)) - P(playerid).last_vehicle_enter_time < 10)
			{
				server::TogglePlayerControllable(playerid, false);
			}

			int anim = server::GetPlayerAnimationIndex(playerid);

			if (anim == 1250 || (anim >= 1538 && anim <= 1544) || weaponid == weapons::DROWN)
			{
				// In water
				PlayerDeath(playerid, animlib, "Drown");
			}
			else if (anim >= 1195 && anim <= 1198)
			{
				// Jumping animation
				PlayerDeath(playerid, animlib, "KO_skid_back");
			}
			else if (weapons::IsShotgun(weaponid))
			{
				if (IsPlayerBehindPlayer(issuerid, playerid))
				{
					MakePlayerFacePlayer(playerid, issuerid, true);
					PlayerDeath(playerid, animlib, "KO_shot_front");
				}
				else
				{
					MakePlayerFacePlayer(playerid, issuerid);
					PlayerDeath(playerid, animlib, "BIKE_fall_off");
				}
			}
			else if (weaponid >= weapons::RIFLE && weaponid <= weapons::SNIPER)
			{
				if (bodypart == 9)
				{
					PlayerDeath(playerid, animlib, "KO_shot_face");
				}
				else if (IsPlayerBehindPlayer(issuerid, playerid))
				{
					PlayerDeath(playerid, animlib, "KO_shot_front");
				}
				else
				{
					PlayerDeath(playerid, animlib, "KO_shot_stom");
				}
			}
			else if (weapons::IsBullet(weaponid))
			{
				PlayerDeath(playerid, animlib, bodypart == 9 ? "KO_shot_face" : "KO_shot_front");
			}
			else if (weaponid == weapons::PISTOLWHIP)
			{
				PlayerDeath(playerid, animlib, "KO_spin_R");
			}
			else if (weaponid == weapons::CARPARK || (weapons::IsMelee(weaponid) && weaponid != weapons::CHAINSAW))
			{
				PlayerDeath(playerid, animlib, "KO_skid_front");
			}
			else if (weaponid == weapons::SPRAYCAN || weaponid == weapons::FIREEXTINGUISHER)
			{
				PlayerDeath(playerid, "KNIFE", "KILL_Knife_Ped_Die");
			}
			else
			{
				PlayerDeath(playerid, animlib, "KO_skid_back");
			}
		}

		if (P(playerid).cbug_allowed || !g_config.cbug_death_delay)
		{
			EmitDeath(playerid, issuerid, weaponid);
		}
		else
		{
			timers::Args args;
			args.a = issuerid;
			args.b = weaponid;
			timers::Start(timers::DELAYED_DEATH, playerid, 1200, args);
		}
	}

	void EmitDeath(int playerid, int killerid, int reason)
	{
		if (!ValidPlayer(playerid) || !P(playerid).connected)
		{
			return;
		}

		if (!ValidPlayer(killerid) || !P(killerid).connected)
		{
			killerid = kInvalidPlayer;
		}

		RecordDeath(playerid, killerid);

		bool previous = g_emitting;
		g_emitting = true;
		scripts::Call("OnPlayerDeath").Int(playerid).Int(killerid).Int(reason).Exec();
		g_emitting = previous;

		EmitAssists(playerid, killerid);
	}

	bool CallDeathFinished(int playerid, bool cancelable)
	{
		Player &p = P(playerid);

		if (p.health == 0.0f)
		{
			p.health = p.max_health;
		}

		timers::Stop(timers::DEATH_RESPAWN, playerid);

		cell ret = scripts::Call("OnPlayerDeathFinished").Int(playerid).Int(cancelable ? 1 : 0).Exec(1, true);

		if (!ret && cancelable)
		{
			return false;
		}

		server::ResetPlayerWeapons(playerid);
		return true;
	}

	void DeathRespawn(int playerid)
	{
		Player &p = P(playerid);

		if (!p.dying)
		{
			return;
		}

		p.dying = false;

		if (!CallDeathFinished(playerid, true))
		{
			UpdateHealthBar(playerid);
			SetFakeFacingAngle(playerid);
			FreezeSyncPacket(playerid, false);
			return;
		}

		p.dying = true;
		p.true_death = false;

		if (server::IsPlayerInAnyVehicle(playerid))
		{
			float x, y, z;
			server::GetPlayerPos(playerid, x, y, z);
			server::SetPlayerPos(playerid, x, y, z);
		}

		SetFakeFacingAngle(playerid);

		// NPC clients don't respawn when spectating is toggled off
		if (p.npc)
		{
			server::SpawnPlayer(playerid);
			return;
		}

		server::SetPlayerVirtualWorld(playerid, kDeathWorld);
		server::TogglePlayerSpectating(playerid, true);
		server::TogglePlayerSpectating(playerid, false);
	}

	void RevivePlayer(int playerid, float health)
	{
		Player &p = P(playerid);

		timers::Stop(timers::DEATH_RESPAWN, playerid);
		timers::Stop(timers::DELAYED_DEATH, playerid);
		timers::Stop(timers::KNIFE_ANIM, playerid);

		if (p.dying)
		{
			ClearDying(playerid);
		}

		// Also gets up a player kept down by OnPlayerDeathFinished returning 0
		server::ClearAnimations(playerid, 1);
		server::TogglePlayerControllable(playerid, true);

		SetFakeFacingAngle(playerid);
		FreezeSyncPacket(playerid, false);

		p.health = health > 0.0f ? (health > p.max_health ? p.max_health : health) : p.max_health;
		UpdateHealthBar(playerid, true);

		if (p.health_bar_visible && p.hb_foreground != kInvalidTextDraw)
		{
			server::PlayerTextDrawShow(playerid, p.hb_foreground);
		}
	}

	int OnRealDeath(int playerid, int &killerid, int &reason)
	{
		Player &p = P(playerid);

		p.true_death = true;
		p.in_class_selection = false;

		timers::Stop(timers::VENDING_USE, playerid);
		timers::Stop(timers::DELAYED_DEATH, playerid);
		timers::Stop(timers::DEATH_RESPAWN, playerid);
		timers::Stop(timers::CBUG_PUNISH, playerid);

		if (p.dying)
		{
			// Already reported when the death animation started
			return 0;
		}

		// Probably fake death
		if (killerid != kInvalidPlayer && (!ValidPlayer(killerid) || !server::IsPlayerStreamedIn(killerid, playerid)))
		{
			killerid = kInvalidPlayer;
		}

		if (reason < 0 || reason > weapons::UNKNOWN)
		{
			reason = weapons::UNKNOWN;
		}

		int vehicleid = server::GetPlayerVehicleID(playerid);

		// Let's assume they died from an exploding vehicle
		if (ValidVehicle(vehicleid) && server::IsValidVehicle(vehicleid))
		{
			reason = weapons::EXPLOSION;
			killerid = kInvalidPlayer;

			int shooter = g_vehicles[vehicleid].last_shooter;

			if (ValidPlayer(shooter) && !HasSameTeam(playerid, shooter))
			{
				killerid = shooter;
			}
		}

		if (reason == weapons::PARACHUTE)
		{
			reason = weapons::COLLISION;
		}

		float amount = 0.0f;
		int bodypart = 0;
		int victim = playerid;

		if (CallOnPlayerDamage(victim, amount, killerid, reason, bodypart))
		{
			if (amount == 0.0f)
			{
				amount = p.health + p.armour;
			}

			bool hit_armour = reason != weapons::COLLISION && reason != weapons::DROWN && reason != weapons::CARPARK;

			if (hit_armour)
			{
				if (amount <= 0.0f)
				{
					amount = p.health + p.armour;
				}
				p.armour -= amount;
			}
			else
			{
				if (amount <= 0.0f)
				{
					amount = p.health;
				}
				p.health -= amount;
			}

			if (hit_armour && p.armour < 0.0f)
			{
				p.damage_done_armour = amount + p.armour;
				p.damage_done_health = -p.armour;
				p.health += p.armour;
				p.armour = 0.0f;
			}
			else if (hit_armour)
			{
				p.damage_done_armour = amount;
				p.damage_done_health = 0.0f;
			}
			else
			{
				p.damage_done_armour = 0.0f;
				p.damage_done_health = amount;
			}

			if (p.health <= 0.0f)
			{
				amount += p.health;
				p.damage_done_health += p.health;
				p.health = 0.0f;
			}

			OnDamageDone(playerid, amount, killerid, reason, bodypart);
		}

		if (p.health > 0.0005f)
		{
			// The death was cancelled in OnPlayerDamage. The client is dead
			// anyway, so it goes through a normal respawn; just hide it.
			p.dying = false;
			UpdateHealthBar(playerid, true);
			return 0;
		}

		p.health = 0.0f;
		p.armour = 0.0f;
		p.dying = true;
		p.last_death_tick = Tick();

		if (g_open_mp && server::Has(server::N::IsPlayerTeleportAllowed) && server::IsPlayerTeleportAllowed(playerid))
		{
			p.restore_teleport = true;
			server::AllowPlayerTeleport(playerid, false);
		}

		std::string animlib, animname;
		cell lock = 0, respawn = 0;

		scripts::Call("OnPlayerPrepareDeath")
			.Int(playerid)
			.String("", 32, &animlib)
			.String("", 32, &animname)
			.Ref(lock)
			.Ref(respawn)
			.Exec();

		RecordDeath(playerid, killerid);
		EmitAssists(playerid, killerid);

		// OnPlayerDeathFinished runs after the script's OnPlayerDeath
		timers::Start(timers::REAL_DEATH_FINISHED, playerid, 0);

		UpdateHealthBar(playerid, true);
		return 1;
	}
}
