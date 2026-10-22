// Damage processing. The validation rules and magic numbers come from
// weapon-config; the float literals are the exact values sent by the client.
#include "core.hpp"
#include "scripts.hpp"
#include "server.hpp"
#include "weapons.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <limits>

namespace wf
{
	namespace
	{
		constexpr int FIGHT_STYLE_KNEEHEAD = 6;

		bool Is(float a, float b)
		{
			uint32_t x, y;
			std::memcpy(&x, &a, sizeof(x));
			std::memcpy(&y, &b, sizeof(y));
			return x == y;
		}

		float VectorSize(float x, float y, float z)
		{
			return std::sqrt(x * x + y * y + z * z);
		}

		float DistanceToPlayer(int playerid, int otherid)
		{
			float x, y, z;
			server::GetPlayerPos(otherid, x, y, z);
			return server::GetPlayerDistanceFromPoint(playerid, x, y, z);
		}

		bool InVehicleOrSurfing(int playerid)
		{
			return server::IsPlayerInAnyVehicle(playerid) || server::GetPlayerSurfingVehicleID(playerid) != kInvalidVehicle;
		}

		int ProcessDamage(int &playerid, int &issuerid, float &amount, int &weaponid, int &bodypart, float &bullets)
		{
			if (amount < 0.0f)
			{
				return WC_INVALID_DAMAGE;
			}

			// Adjust invalid amounts caused by an animation bug
			if (Is(amount, 3.63000011444091796875f) || Is(amount, 5.940000057220458984375f) || Is(amount, 5.610000133514404296875f))
			{
				amount = 2.6400001049041748046875f;
			}
			else if (Is(amount, 3.30000019073486328125f))
			{
				if (weaponid != weapons::SHOTGUN && weaponid != weapons::SAWEDOFF)
				{
					amount = 2.6400001049041748046875f;
				}
			}
			else if (Is(amount, 4.950000286102294921875f))
			{
				if (weapons::IsMelee(weaponid))
				{
					amount = 2.6400001049041748046875f;
				}
			}
			else if (Is(amount, 6.270000457763671875f) || Is(amount, 6.93000030517578125f) || Is(amount, 7.2600002288818359375f) || Is(amount, 7.9200000762939453125f) || Is(amount, 8.5799999237060546875f) || Is(amount, 9.24000072479248046875f) || Is(amount, 11.88000011444091796875f) || Is(amount, 11.22000026702880859375f))
			{
				amount = 2.6400001049041748046875f;
			}
			else if (Is(amount, 9.90000057220458984375f))
			{
				switch (weaponid)
				{
				case weapons::VEHICLE:
				case weapons::VEHICLE_M4:
				case weapons::AK47:
				case weapons::M4:
				case weapons::SHOTGUN:
				case weapons::SAWEDOFF:
				case weapons::SHOTGSPA:
					break;
				default:
					amount = 6.6000003814697265625f;
				}
			}

			// Car parking
			if (weaponid == weapons::HELIBLADES && !Is(amount, 330.0f))
			{
				weaponid = weapons::CARPARK;
			}
			else if (weaponid >= weapons::VEHICLE && weaponid <= weapons::HELIBLADES)
			{
				if (issuerid != kInvalidPlayer)
				{
					float dist = DistanceToPlayer(playerid, issuerid);

					if (dist > 15.0f)
					{
						AddRejectedHit(issuerid, playerid, HIT_TOO_FAR_FROM_ORIGIN, weaponid, amx_ftoc(dist));
						return WC_INVALID_DISTANCE;
					}
				}
			}

			// Only torso from non-bullet weapons
			if (!weapons::IsBullet(weaponid) && !(weaponid >= weapons::VEHICLE_M4 && weaponid <= weapons::VEHICLE_MINIGUN) && bodypart != 3)
			{
				return WC_INVALID_DAMAGE;
			}

			weapons::Weapon *w = weapons::Get(weaponid);

			// Finish processing drown/fire/carpark quickly, since they are sent at very high rates
			if (weapons::IsHighRate(weaponid))
			{
				if (weaponid == weapons::DROWN)
				{
					if (amount > 10.0f)
					{
						amount = 10.0f;
					}
				}
				else if (amount > 1.0f)
				{
					amount = 1.0f;
				}

				if (!Is(w->damage, 1.0f))
				{
					amount *= w->damage;
				}

				// Carpark can be self-inflicted so it doesn't require an issuer
				if (weaponid == weapons::SPRAYCAN || weaponid == weapons::FIREEXTINGUISHER || (weaponid == weapons::CARPARK && issuerid != kInvalidPlayer))
				{
					if (issuerid == kInvalidPlayer)
					{
						return WC_NO_ISSUER;
					}

					float dist = DistanceToPlayer(playerid, issuerid);

					if (weaponid == weapons::CARPARK)
					{
						if (dist > 15.0f)
						{
							AddRejectedHit(issuerid, playerid, HIT_TOO_FAR_FROM_ORIGIN, weaponid, amx_ftoc(dist));
							return WC_INVALID_DISTANCE;
						}
					}
					else if (dist > w->range + 2.0f)
					{
						AddRejectedHit(issuerid, playerid, HIT_TOO_FAR_FROM_ORIGIN, weaponid, amx_ftoc(dist), amx_ftoc(w->range));
						return WC_INVALID_DISTANCE;
					}
				}

				return WC_NO_ERROR;
			}

			// Bullet or melee damage must have an issuerid, otherwise something has gone wrong (e.g. sniper bug)
			if (issuerid == kInvalidPlayer && (weapons::IsBullet(weaponid) || weapons::IsMelee(weaponid)))
			{
				return WC_NO_ISSUER;
			}

			if (weaponid == weapons::PARACHUTE)
			{
				// Punching with a parachute
				weaponid = weapons::UNARMED;
			}
			else if (weaponid == weapons::COLLISION)
			{
				// Collision damage should never be above 165
				amount = amount > 165.0f ? 1.0f : amount / 165.0f;
			}
			else if (weaponid == weapons::EXPLOSION)
			{
				// Explosions do at most 82.5 damage. This will later be multiplied by the damage value
				amount /= 82.5f;

				// Figure out what caused the explosion
				if (issuerid != kInvalidPlayer)
				{
					if (server::GetPlayerState(issuerid) == PLAYER_STATE_DRIVER)
					{
						if (IsVehicleArmedWithWeapon(server::GetPlayerVehicleID(issuerid), weapons::ROCKETLAUNCHER))
						{
							weaponid = weapons::VEHICLE_ROCKETLAUNCHER;
						}
					}
					else if (P(issuerid).last_explosive)
					{
						weaponid = P(issuerid).last_explosive;
					}
				}
				else if (server::GetPlayerState(playerid) == PLAYER_STATE_DRIVER)
				{
					if (IsVehicleArmedWithWeapon(server::GetPlayerVehicleID(playerid), weapons::ROCKETLAUNCHER))
					{
						weaponid = weapons::VEHICLE_ROCKETLAUNCHER;
					}
				}
			}

			// Check for pistol whip
			if ((weaponid >= weapons::COLT45 && weaponid <= weapons::SNIPER) || weaponid == weapons::MINIGUN || weaponid == weapons::SPRAYCAN || weaponid == weapons::FIREEXTINGUISHER)
			{
				// A pistol whip inflicts 2.64 damage
				if (Is(amount, 2.6400001049041748046875f))
				{
					// Save the weapon in the bodypart argument (it's always the torso)
					bodypart = weaponid;
					weaponid = weapons::PISTOLWHIP;
				}
			}

			bool melee = weapons::IsMelee(weaponid);

			// Can't punch from a vehicle
			if (melee && server::IsPlayerInAnyVehicle(issuerid))
			{
				return WC_INVALID_DAMAGE;
			}

			if (weaponid != weapons::PISTOLWHIP)
			{
				if (Is(amount, 1.32000005245208740234375f) || Is(amount, 1.650000095367431640625f) || Is(amount, 1.980000019073486328125f) || Is(amount, 2.3100001811981201171875f) || Is(amount, 2.6400001049041748046875f) || Is(amount, 2.9700000286102294921875f) || Is(amount, 3.96000003814697265625f) || Is(amount, 4.28999996185302734375f) || Is(amount, 4.62000036239624023437f) || Is(amount, 5.280000209808349609375f))
				{
					// Damage is most likely from punching and switching weapon quickly
					if (!melee)
					{
						weaponid = weapons::UNARMED;
						melee = true;
					}
				}
				else if (Is(amount, 6.6000003814697265625f))
				{
					if (!melee)
					{
						switch (weaponid)
						{
						case weapons::UZI:
						case weapons::TEC9:
						case weapons::SHOTGUN:
						case weapons::SAWEDOFF:
							break;
						default:
							weaponid = weapons::UNARMED;
							melee = true;
						}
					}
				}
				else if (Is(amount, 54.12000274658203125f))
				{
					if (!melee)
					{
						melee = true;
						weaponid = weapons::UNARMED;
						amount = 1.32000005245208740234375f;
					}

					// Be extra sure about this one
					if (server::GetPlayerFightingStyle(issuerid) != FIGHT_STYLE_KNEEHEAD)
					{
						return WC_INVALID_DAMAGE;
					}
				}
				else if (melee && weaponid != weapons::CHAINSAW)
				{
					// Melee damage has been tampered with (chainsaw is checked later)
					return WC_INVALID_DAMAGE;
				}
			}

			w = weapons::Get(weaponid);

			if (melee)
			{
				float dist = DistanceToPlayer(playerid, issuerid);
				int reason = weaponid == weapons::PISTOLWHIP ? weapons::UNARMED : weaponid;
				weapons::Weapon *rw = weapons::Get(reason);

				if (reason <= weapons::PARACHUTE && rw && dist > rw->range + 2.0f)
				{
					AddRejectedHit(issuerid, playerid, HIT_TOO_FAR_FROM_ORIGIN, weaponid, amx_ftoc(dist), amx_ftoc(rw->range));
					return WC_INVALID_DISTANCE;
				}
			}

			switch (weaponid)
			{
			case weapons::SHOTGSPA:
				// The spas shotgun shoots 8 bullets, each inflicting 4.95 damage
				bullets = amount / 4.950000286102294921875f;
				if (8.0f - bullets < -0.05f)
				{
					return WC_INVALID_DAMAGE;
				}
				break;

			case weapons::SHOTGUN:
			case weapons::SAWEDOFF:
				// Shotguns and sawed-off shotguns shoot 15 bullets, each inflicting 3.3 damage
				bullets = amount / 3.30000019073486328125f;
				if (15.0f - bullets < -0.05f)
				{
					return WC_INVALID_DAMAGE;
				}
				break;
			}

			if (bullets != 0.0f)
			{
				float f = bullets - std::floor(bullets);

				// The damage for each bullet has been tampered with
				if (f > 0.01f && f < 0.99f)
				{
					return WC_INVALID_DAMAGE;
				}

				// Divide the damage amount by the number of bullets
				amount /= bullets;
			}

			if (weaponid == weapons::CHAINSAW)
			{
				if (!Is(amount, 6.6000003814697265625f) && !Is(amount, 13.5300006866455078125f) && !Is(amount, 16.1700000762939453125f) && !Is(amount, 26.40000152587890625f) && !Is(amount, 27.060001373291015625f))
				{
					return WC_INVALID_DAMAGE;
				}
			}
			else if (weaponid == weapons::DEAGLE)
			{
				if (!Is(amount, 46.200000762939453125f) && !Is(amount, 23.1000003814697265625f))
				{
					return WC_INVALID_DAMAGE;
				}
			}

			// Check gun damage
			float def_amount = 0.0f;

			switch (weaponid)
			{
			case weapons::COLT45:
			case weapons::MP5:
				def_amount = 8.25f;
				break;
			case weapons::SILENCED:
				def_amount = 13.200000762939453125f;
				break;
			case weapons::UZI:
			case weapons::TEC9:
				def_amount = 6.6000003814697265625f;
				break;
			case weapons::AK47:
			case weapons::M4:
			case weapons::VEHICLE_M4:
				def_amount = 9.90000057220458984375f;
				break;
			case weapons::RIFLE:
				def_amount = 24.7500019073486328125f;
				break;
			case weapons::SNIPER:
				def_amount = 41.25f;
				break;
			case weapons::MINIGUN:
			case weapons::VEHICLE_MINIGUN:
				def_amount = 46.200000762939453125f;
				break;
			case weapons::VEHICLE:
				def_amount = 9.90000057220458984375f;
				break;
			}

			if (def_amount != 0.0f && !Is(def_amount, amount))
			{
				return WC_INVALID_DAMAGE;
			}

			// Adjust the damage
			switch (w->damage_type)
			{
			case weapons::DAMAGE_TYPE_MULTIPLIER:
				if (!Is(w->damage, 1.0f))
				{
					amount *= w->damage;
				}
				break;

			case weapons::DAMAGE_TYPE_STATIC:
				amount = bullets != 0.0f ? w->damage * bullets : w->damage;
				break;

			case weapons::DAMAGE_TYPE_RANGE:
			case weapons::DAMAGE_TYPE_RANGE_MULTIPLIER:
			{
				float length = 0.0f;

				if (issuerid != kInvalidPlayer)
				{
					if (g_config.lag_comp_mode)
					{
						length = P(issuerid).last_shot.length;
					}
					else
					{
						length = DistanceToPlayer(playerid, issuerid);
					}
				}

				for (int i = w->range_steps - 1; i >= 0; --i)
				{
					if (i == 0 || length >= w->range_ranges[i])
					{
						if (w->damage_type == weapons::DAMAGE_TYPE_RANGE_MULTIPLIER)
						{
							if (!Is(w->range_values[i], 1.0f))
							{
								amount *= w->range_values[i];
							}
						}
						else
						{
							amount = bullets != 0.0f ? w->range_values[i] * bullets : w->range_values[i];
						}
						break;
					}
				}
				break;
			}
			}

			return WC_NO_ERROR;
		}

		void PlayDamageSound(int playerid, int sound)
		{
			server::PlayerPlaySound(playerid, sound, 0.0f, 0.0f, 0.0f);

			for (int i = 0; i < g_max_players; ++i)
			{
				if (i != playerid && P(i).connected && P(i).spectating == playerid)
				{
					server::PlayerPlaySound(i, sound, 0.0f, 0.0f, 0.0f);
				}
			}
		}

		void RecordHit(Player &p, uint32_t tick, int weaponid)
		{
			int idx = (p.hit_idx + 1) % kRateSamples;
			p.hit_idx = idx;
			p.hit_ticks[idx] = tick;
			p.hit_weapons[idx] = weaponid;
			p.hits_issued += 1;
		}

		int AverageRate(int count, const uint32_t (&ticks)[kRateSamples], const int (&weapons_)[kRateSamples], int idx, int samples, bool *multiple)
		{
			if (multiple)
			{
				*multiple = false;
			}

			if (samples < 1 || samples > kRateSamples || count < samples)
			{
				return -1;
			}

			int64_t total = 0;

			for (int i = samples - 2; i >= 0; --i)
			{
				int prev_idx = ((idx - i - 1) % kRateSamples + kRateSamples) % kRateSamples;
				int this_idx = ((idx - i) % kRateSamples + kRateSamples) % kRateSamples;

				if (multiple && weapons_[prev_idx] != weapons_[this_idx])
				{
					*multiple = true;
				}

				total += TickDiff(ticks[this_idx], ticks[prev_idx]);
			}

			return samples == 1 ? 1 : static_cast<int>(total / (samples - 1));
		}
	}

	bool IsPlayerSpawned(int playerid)
	{
		if (!ValidPlayer(playerid))
		{
			return false;
		}

		Player &p = P(playerid);

		if (p.dying || p.in_class_selection)
		{
			return false;
		}

		switch (server::GetPlayerState(playerid))
		{
		case PLAYER_STATE_ONFOOT:
		case PLAYER_STATE_DRIVER:
		case PLAYER_STATE_PASSENGER:
		case PLAYER_STATE_SPAWNED:
			return true;
		default:
			return false;
		}
	}

	bool IsPlayerPaused(int playerid)
	{
		if (!ValidPlayer(playerid))
		{
			return false;
		}

		return TickDiff(Tick(), P(playerid).last_update_tick) > 2000;
	}

	bool HasSameTeam(int playerid, int otherid)
	{
		if (!ValidPlayer(playerid) || !ValidPlayer(otherid) || g_config.friendly_fire)
		{
			return false;
		}

		int a = P(playerid).team, b = P(otherid).team;

		if (a == kNoTeam || b == kNoTeam)
		{
			return false;
		}

		return a == b;
	}

	bool IsVehicleBike(int vehicleid)
	{
		switch (server::GetVehicleModel(vehicleid))
		{
		case 448:
		case 461:
		case 462:
		case 463:
		case 468:
		case 471:
		case 481:
		case 509:
		case 510:
		case 521:
		case 522:
		case 523:
		case 581:
		case 586:
			return true;
		default:
			return false;
		}
	}

	bool IsVehicleArmedWithWeapon(int vehicleid, int weaponid)
	{
		switch (server::GetVehicleModel(vehicleid))
		{
		case 425:
			return weaponid == weapons::MINIGUN || weaponid == weapons::ROCKETLAUNCHER;
		case 447:
		case 464:
		case 476:
			return weaponid == weapons::M4;
		case 432:
		case 520:
			return weaponid == weapons::ROCKETLAUNCHER;
		default:
			return false;
		}
	}

	int AverageShootRate(int playerid, int shots, bool *multiple_weapons)
	{
		if (!ValidPlayer(playerid))
		{
			return -1;
		}

		Player &p = P(playerid);
		return AverageRate(p.shots_fired, p.shot_ticks, p.shot_weapons, p.shot_idx, shots, multiple_weapons);
	}

	int AverageHitRate(int playerid, int hits, bool *multiple_weapons)
	{
		if (!ValidPlayer(playerid))
		{
			return -1;
		}

		Player &p = P(playerid);
		return AverageRate(p.hits_issued, p.hit_ticks, p.hit_weapons, p.hit_idx, hits, multiple_weapons);
	}

	void AddRejectedHit(int playerid, int damagedid, int reason, int weapon, cell i1, cell i2, cell i3)
	{
		if (!ValidPlayer(playerid))
		{
			return;
		}

		Player &p = P(playerid);
		int idx = p.rejected_idx;

		if (p.rejected[idx].time)
		{
			idx = (idx + 1) % kMaxRejectedHits;
			p.rejected_idx = idx;
		}

		std::time_t now = std::time(nullptr);
		std::tm tm_now{};
#if defined(_WIN32)
		localtime_s(&tm_now, &now);
#else
		localtime_r(&now, &tm_now);
#endif

		RejectedHit &hit = p.rejected[idx];
		hit.time = static_cast<int64_t>(now);
		hit.hour = tm_now.tm_hour;
		hit.minute = tm_now.tm_min;
		hit.second = tm_now.tm_sec;
		hit.weapon = weapon;
		hit.reason = reason;
		hit.info1 = i1;
		hit.info2 = i2;
		hit.info3 = i3;
		hit.name = ValidPlayer(damagedid) && P(damagedid).connected ? server::GetPlayerName(damagedid) : std::string("#");

		p.stats[STAT_REJECTED_HITS] += 1;

		if (g_config.debug)
		{
			bool ok = false;
			std::string text = FormatRejectedHit(playerid, 0, &ok);
			Debug("Rejected hit from %d: %s", playerid, text.c_str());
		}

		// enum E_REJECTED_HIT { e_Time, e_Hour, e_Minute, e_Second, e_Weapon, e_Reason, e_Info1, e_Info2, e_Info3, e_Name[24] }
		cell arr[9 + 24] = {};
		arr[0] = static_cast<cell>(hit.time);
		arr[1] = hit.hour;
		arr[2] = hit.minute;
		arr[3] = hit.second;
		arr[4] = hit.weapon;
		arr[5] = hit.reason;
		arr[6] = hit.info1;
		arr[7] = hit.info2;
		arr[8] = hit.info3;

		for (size_t i = 0; i < hit.name.size() && i < 23; ++i)
		{
			arr[9 + i] = static_cast<unsigned char>(hit.name[i]);
		}

		scripts::Call("OnRejectedHit").Int(playerid).Array(arr, 9 + 24, false).Exec();
	}

	std::string FormatRejectedHit(int playerid, int idx, bool *ok)
	{
		*ok = false;

		if (!ValidPlayer(playerid) || idx < 0 || idx >= kMaxRejectedHits)
		{
			return {};
		}

		Player &p = P(playerid);
		int real_idx = ((p.rejected_idx - idx) % kMaxRejectedHits + kMaxRejectedHits) % kMaxRejectedHits;
		const RejectedHit &hit = p.rejected[real_idx];

		if (!hit.time)
		{
			return {};
		}

		char text[160];
		const int i1 = hit.info1, i2 = hit.info2, i3 = hit.info3;
		const float f1 = amx_ctof(hit.info1), f2 = amx_ctof(hit.info2);

		switch (hit.reason)
		{
		case HIT_NO_DAMAGEDID:
			std::snprintf(text, sizeof(text), "None or invalid player shot");
			break;
		case HIT_INVALID_WEAPON:
			std::snprintf(text, sizeof(text), "Invalid weapon");
			break;
		case HIT_LAST_SHOT_INVALID:
			std::snprintf(text, sizeof(text), "Last shot invalid");
			break;
		case HIT_MULTIPLE_PLAYERS:
			std::snprintf(text, sizeof(text), "One bullet hit %d players", i1);
			break;
		case HIT_MULTIPLE_PLAYERS_SHOTGUN:
			std::snprintf(text, sizeof(text), "Hit too many players with shotgun: %d", i1);
			break;
		case HIT_DYING_PLAYER:
			std::snprintf(text, sizeof(text), "Hit a dying player");
			break;
		case HIT_SAME_TEAM:
			std::snprintf(text, sizeof(text), "Hit a teammate");
			break;
		case HIT_UNSTREAMED:
			std::snprintf(text, sizeof(text), "Hit someone that can't see you (not streamed in)");
			break;
		case HIT_INVALID_HITTYPE:
			std::snprintf(text, sizeof(text), "Invalid hit type: %d", i1);
			break;
		case HIT_BEING_RESYNCED:
			std::snprintf(text, sizeof(text), "Hit while being resynced");
			break;
		case HIT_NOT_SPAWNED:
			std::snprintf(text, sizeof(text), "Hit when not spawned or dying");
			break;
		case HIT_OUT_OF_RANGE:
			std::snprintf(text, sizeof(text), "Hit out of range (%.4f > %.4f)", f1, f2);
			break;
		case HIT_TOO_FAR_FROM_SHOT:
			std::snprintf(text, sizeof(text), "Hit player too far from hit position (dist %.4f)", f1);
			break;
		case SHOOTING_RATE_TOO_FAST:
			std::snprintf(text, sizeof(text), "Shooting rate too fast: %d (%d samples, max %d)", i1, i2, i3);
			break;
		case SHOOTING_RATE_TOO_FAST_MULTIPLE:
			std::snprintf(text, sizeof(text), "Shooting rate too fast: %d (%d samples, multiple weapons)", i1, i2);
			break;
		case HIT_RATE_TOO_FAST:
			std::snprintf(text, sizeof(text), "Hit rate too fast: %d (%d samples, max %d)", i1, i2, i3);
			break;
		case HIT_RATE_TOO_FAST_MULTIPLE:
			std::snprintf(text, sizeof(text), "Hit rate too fast: %d (%d samples, multiple weapons)", i1, i2);
			break;
		case HIT_TOO_FAR_FROM_ORIGIN:
			std::snprintf(text, sizeof(text), "Damage inflicted too far from current position (dist %.4f)", f1);
			break;
		case HIT_INVALID_DAMAGE:
			std::snprintf(text, sizeof(text), "Invalid weapon damage (%.4f)", f1);
			break;
		case HIT_SAME_VEHICLE:
			std::snprintf(text, sizeof(text), "Hit a player in the same vehicle");
			break;
		case HIT_OWN_VEHICLE:
			std::snprintf(text, sizeof(text), "Hit the vehicle you're in");
			break;
		case HIT_INVALID_VEHICLE:
			std::snprintf(text, sizeof(text), "Hit invalid vehicle: %d", i1);
			break;
		case HIT_DISCONNECTED:
			std::snprintf(text, sizeof(text), "Hit a disconnected player ID: %d", i1);
			break;
		case HIT_PROTECTED:
			std::snprintf(text, sizeof(text), "Hit a protected player");
			break;
		default:
			std::snprintf(text, sizeof(text), "Unknown reason %d", hit.reason);
		}

		char out[256];
		std::snprintf(out, sizeof(out), "[%02d:%02d:%02d] (%s -> %s) %s", hit.hour, hit.minute, hit.second, weapons::Name(hit.weapon).c_str(), hit.name.c_str(), text);

		*ok = true;
		return out;
	}

	bool CallOnPlayerDamage(int &playerid, float &amount, int &issuerid, int &weapon, int &bodypart)
	{
		cell c_playerid = playerid, c_issuerid = issuerid, c_weapon = weapon, c_bodypart = bodypart;

		cell ret = scripts::Call("OnPlayerDamage")
					   .Ref(c_playerid)
					   .RefFloat(amount)
					   .Ref(c_issuerid)
					   .Ref(c_weapon)
					   .Ref(c_bodypart)
					   .Exec(1, true);

		if (ValidPlayer(c_playerid) && P(c_playerid).connected)
		{
			playerid = c_playerid;
		}

		issuerid = (c_issuerid >= 0 && c_issuerid < kMaxPlayers) ? c_issuerid : kInvalidPlayer;
		weapon = (c_weapon >= 0 && c_weapon <= weapons::UNKNOWN) ? c_weapon : weapons::UNKNOWN;
		bodypart = c_bodypart;

		return ret != 0;
	}

	void CallOnInvalidWeaponDamage(int playerid, int damagedid, float amount, int weaponid, int bodypart, int error, bool given)
	{
		Debug("OnInvalidWeaponDamage(%d, %d, %f, %d, %d, %d, %d)", playerid, damagedid, amount, weaponid, bodypart, error, given);

		scripts::Call("OnInvalidWeaponDamage")
			.Int(playerid)
			.Int(damagedid)
			.Float(amount)
			.Int(weaponid)
			.Int(bodypart)
			.Int(error)
			.Int(given)
			.Exec();
	}

	void EnterCombat(int playerid)
	{
		if (!ValidPlayer(playerid))
		{
			return;
		}

		Player &p = P(playerid);
		p.last_damage_tick = Tick();

		if (!p.in_combat && g_config.combat_timeout > 0)
		{
			p.in_combat = true;
			scripts::Call("OnPlayerEnterCombat").Int(playerid).Exec();
		}
	}

	bool IsProtected(int playerid)
	{
		const Player &p = P(playerid);
		return p.god_mode || (p.protected_until && TickDiff(p.protected_until, Tick()) > 0);
	}

	void InflictDamage(int playerid, float amount, int issuerid, int weaponid, int bodypart, bool ignore_armour, bool bypass_protection)
	{
		if (!IsPlayerSpawned(playerid) || amount < 0.0f)
		{
			return;
		}

		Player &victim = P(playerid);

		// weapon-forge: god mode and spawn protection
		if (!bypass_protection && IsProtected(playerid))
		{
			// High rate weapons (fire, spray...) would flood the rejected hits
			if (issuerid != kInvalidPlayer && !weapons::IsHighRate(weaponid))
			{
				AddRejectedHit(issuerid, playerid, HIT_PROTECTED, weaponid);
			}

			UpdateHealthBar(playerid);
			return;
		}

		// weapon-forge: per player and per body part multipliers
		if (amount > 0.0f && !bypass_protection)
		{
			float mul = victim.taken_mul;

			if (ValidPlayer(issuerid) && issuerid != playerid)
			{
				mul *= P(issuerid).given_mul;
			}

			if (weapons::Weapon *w = weapons::Get(weaponid))
			{
				if (bodypart >= 0 && bodypart < weapons::kBodyParts)
				{
					mul *= w->bodypart_mul[bodypart];
				}
			}

			if (mul != 1.0f)
			{
				amount *= mul;

				if (amount <= 0.0f)
				{
					UpdateHealthBar(playerid, true);
					return;
				}
			}
		}

		int original_victim = playerid;

		if (!CallOnPlayerDamage(playerid, amount, issuerid, weaponid, bodypart))
		{
			UpdateHealthBar(original_victim, true);
			return;
		}

		// OnPlayerDamage may redirect the damage to another player
		if (playerid != original_victim && (!IsPlayerSpawned(playerid) || (!bypass_protection && IsProtected(playerid))))
		{
			UpdateHealthBar(original_victim, true);
			return;
		}

		Player &p = P(playerid);
		weapons::Weapon *w = weapons::Get(weaponid);

		bool hit_armour = !ignore_armour && weaponid != weapons::COLLISION && weaponid != weapons::DROWN && weaponid != weapons::CARPARK && weaponid != weapons::UNKNOWN && (!g_config.armour_rules || (w && w->armour_affects && (!g_config.torso_rules || (w->armour_torso && bodypart == 3) || !w->armour_torso)));

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

		OnDamageDone(playerid, amount, issuerid, weaponid, bodypart);

		if (p.health <= 0.0005f && !p.dying)
		{
			KillFromDamage(playerid, issuerid, weaponid, bodypart);
		}

		UpdateHealthBar(playerid, true);
	}

	void OnDamageDone(int playerid, float amount, int issuerid, int weapon, int bodypart)
	{
		Player &p = P(playerid);
		uint32_t tick = Tick();

		int idx = p.prev_hit_idx;
		p.prev_hit_idx = ((p.prev_hit_idx - 1) % kPrevHits + kPrevHits) % kPrevHits;

		HitInfo &hit = p.prev_hits[idx];
		hit.tick = tick;
		hit.issuer = issuerid;
		hit.weapon = weapon;
		hit.amount = amount;
		hit.bodypart = bodypart;
		hit.health = p.damage_done_health;
		hit.armour = p.damage_done_armour;

		// weapon-forge: statistics and combat state
		p.stats[STAT_HITS_TAKEN] += 1;
		p.stats_f[STATF_DAMAGE_TAKEN] += amount;
		p.stats_f[STATF_HEALTH_LOST] += p.damage_done_health;
		p.stats_f[STATF_ARMOUR_LOST] += p.damage_done_armour;

		bool headshot = bodypart == 9 && weapons::IsBullet(weapon) && ValidPlayer(issuerid);

		if (headshot)
		{
			p.stats[STAT_HEADSHOTS_TAKEN] += 1;
		}

		if (ValidPlayer(issuerid) && issuerid != playerid)
		{
			Player &issuer = P(issuerid);
			issuer.stats[STAT_SHOTS_HIT] += 1;
			issuer.stats_f[STATF_DAMAGE_GIVEN] += amount;

			if (headshot)
			{
				issuer.stats[STAT_HEADSHOTS_GIVEN] += 1;
			}

			p.last_attacker = issuerid;
			EnterCombat(issuerid);
		}

		EnterCombat(playerid);

		if (!weapons::IsHighRate(weapon))
		{
			Debug("OnPlayerDamageDone(%d did %f to %d with %d on bodypart %d)", issuerid, amount, playerid, weapon, bodypart);

			if (g_config.damage_taken_sound)
			{
				PlayDamageSound(playerid, g_config.damage_taken_sound);
			}

			if (g_config.damage_given_sound && issuerid != kInvalidPlayer)
			{
				PlayDamageSound(issuerid, g_config.damage_given_sound);
			}
		}

		if (issuerid != kInvalidPlayer)
		{
			DamageFeedAddHitGiven(issuerid, playerid, amount, weapon);
		}

		DamageFeedAddHitTaken(playerid, issuerid, amount, weapon);

		scripts::Call("OnPlayerDamageDone")
			.Int(playerid)
			.Float(amount)
			.Int(issuerid)
			.Int(weapon)
			.Int(bodypart)
			.Exec();

		if (headshot)
		{
			scripts::Call("OnPlayerHeadshot")
				.Int(playerid)
				.Int(issuerid)
				.Int(weapon)
				.Float(amount)
				.Exec();
		}
	}

	int OnGiveDamage(int playerid, int damagedid, float amount, int weaponid, int bodypart)
	{
		if (!weapons::IsHighRate(weaponid))
		{
			Debug("OnPlayerGiveDamage(%d gave %f to %d using %d on bodypart %d)", playerid, amount, damagedid, weaponid, bodypart);
		}

		// Nobody got damaged
		if (!ValidPlayer(damagedid) || !P(damagedid).connected)
		{
			CallOnInvalidWeaponDamage(playerid, damagedid, amount, weaponid, bodypart, WC_NO_DAMAGED, true);
			AddRejectedHit(playerid, damagedid, HIT_NO_DAMAGEDID, weaponid);
			return 0;
		}

		if (P(damagedid).dying)
		{
			AddRejectedHit(playerid, damagedid, HIT_DYING_PLAYER, weaponid);
			return 0;
		}

		bool npc = P(damagedid).npc;

		if (!g_config.lag_comp_mode && !npc)
		{
			return 0;
		}

		// Ignore unreliable and invalid damage
		weapons::Weapon *w = weapons::Get(weaponid);

		if (!w || !w->valid_given)
		{
			// Fire and vehicle damage are synced as taken damage, so no need to show a rejected hit.
			if (weaponid != weapons::FLAMETHROWER && weaponid != weapons::VEHICLE)
			{
				AddRejectedHit(playerid, damagedid, HIT_INVALID_WEAPON, weaponid);
			}
			return 0;
		}

		uint32_t tick = Tick();
		Player &p = P(playerid);

		if (!IsPlayerSpawned(playerid) && TickDiff(tick, p.last_death_tick) > 80)
		{
			// Make sure the rejected hit wasn't added in OnPlayerWeaponShot
			if (!weapons::IsBullet(weaponid) || p.last_shot.valid)
			{
				AddRejectedHit(playerid, damagedid, HIT_NOT_SPAWNED, weaponid);
			}
			return 0;
		}

		// From stealth knife, can be any weapon
		if (Is(amount, 1833.33154296875f))
		{
			return 0;
		}

		if (weaponid == weapons::KNIFE && Is(amount, 0.0f))
		{
			if (damagedid == playerid)
			{
				return 0;
			}

			int slot_weapon, slot_ammo;
			server::GetPlayerWeaponData(playerid, 0, slot_weapon, slot_ammo);

			bool reject = npc || HasSameTeam(playerid, damagedid);

			if (!reject)
			{
				int c_damagedid = damagedid, c_playerid = playerid, c_weapon = weaponid, c_bodypart = bodypart;
				float c_amount = amount;

				reject = DistanceToPlayer(damagedid, playerid) > w->range + 2.0f || !CallOnPlayerDamage(c_damagedid, c_amount, c_playerid, c_weapon, c_bodypart);
			}

			if (reject)
			{
				// Resync without bothering the player being knifed
				server::ClearAnimations(playerid, 1);
				server::ClearAnimations(damagedid, 1);
				server::SetPlayerArmedWeapon(playerid, slot_weapon);
				UpdateHealthBar(damagedid, true);
				return 0;
			}

			Player &victim = P(damagedid);
			victim.damage_done_health = victim.health;
			victim.damage_done_armour = victim.armour;

			OnDamageDone(damagedid, victim.health + victim.armour, playerid, weaponid, bodypart);

			server::ClearAnimations(damagedid, 1);
			PlayerDeath(damagedid, "KNIFE", "KILL_Knife_Ped_Damage", false, 5200);
			timers::Start(timers::KNIFE_ANIM, damagedid, 2200);
			EmitDeath(damagedid, playerid, weaponid);

			int anim = server::GetPlayerAnimationIndex(playerid);
			int forcesync = 2;

			server::SetPlayerFacingAngle(playerid, server::GetPlayerFacingAngle(damagedid));
			server::SetPlayerVelocity(damagedid, 0.0f, 0.0f, 0.0f);
			server::SetPlayerVelocity(playerid, 0.0f, 0.0f, 0.0f);

			if (anim != 747 && anim != 748)
			{
				forcesync = 1;
			}

			server::ApplyAnimation(playerid, "KNIFE", "KILL_Knife_Player", 4.1f, false, true, true, false, 1800, forcesync);
			return 0;
		}

		if (HasSameTeam(playerid, damagedid))
		{
			AddRejectedHit(playerid, damagedid, HIT_SAME_TEAM, weaponid);
			return 0;
		}

		// Both players should see each other
		if ((!server::IsPlayerStreamedIn(playerid, damagedid) && !IsPlayerPaused(damagedid)) || !server::IsPlayerStreamedIn(damagedid, playerid))
		{
			AddRejectedHit(playerid, damagedid, HIT_UNSTREAMED, weaponid, damagedid);
			return 0;
		}

		float bullets = 0.0f;
		int victim = damagedid, issuer = playerid;
		int err = ProcessDamage(victim, issuer, amount, weaponid, bodypart, bullets);

		if (err)
		{
			if (err == WC_INVALID_DAMAGE)
			{
				AddRejectedHit(playerid, damagedid, HIT_INVALID_DAMAGE, weaponid, amx_ftoc(amount));
			}

			if (err != WC_INVALID_DISTANCE)
			{
				CallOnInvalidWeaponDamage(playerid, damagedid, amount, weaponid, bodypart, err, true);
			}

			return 0;
		}

		RecordHit(p, tick, weaponid);

		bool multiple = false;
		int avg_rate = AverageHitRate(playerid, g_config.max_hit_rate_samples, &multiple);

		// Hit issue flood? Could be either a cheat or just lag
		if (avg_rate != -1)
		{
			if (multiple)
			{
				if (avg_rate < 100)
				{
					AddRejectedHit(playerid, damagedid, HIT_RATE_TOO_FAST_MULTIPLE, weaponid, avg_rate, g_config.max_hit_rate_samples);
					return 0;
				}
			}
			else if (weapons::Weapon *rw = weapons::Get(weaponid))
			{
				if (rw->shoot_rate - avg_rate > 20)
				{
					AddRejectedHit(playerid, damagedid, HIT_RATE_TOO_FAST, weaponid, avg_rate, g_config.max_hit_rate_samples, rw->shoot_rate);
					return 0;
				}
			}
		}

		bool valid = true;

		if (server::GetPlayerState(playerid) == PLAYER_STATE_DRIVER)
		{
			if ((weaponid >= weapons::UZI && weaponid <= weapons::MP5) || weaponid == weapons::TEC9)
			{
				int vehicleid = server::GetPlayerVehicleID(playerid);

				if (!IsVehicleBike(vehicleid))
				{
					int keys, ud, lr;
					server::GetPlayerKeys(playerid, keys, ud, lr);

					// It is only possible to shoot if you look right or left from car
					valid = (keys & KEY_LOOK_RIGHT) || (keys & KEY_LOOK_LEFT);
				}
			}
			else
			{
				// Damage from something unusable from the driver's seat
				valid = false;
			}
		}
		else if (weapons::IsBullet(weaponid) && !Is(amount, 2.6400001049041748046875f))
		{
			ShotInfo &shot = p.last_shot;

			if (!shot.valid || (TickDiff(tick, shot.tick) > 1500 && weaponid != weapons::SNIPER))
			{
				valid = false;
				Debug("%d: last shot not valid", playerid);
			}
			else if (weapons::IsShotgun(weaponid))
			{
				// Let's assume someone won't hit 2 players with 1 shotgun shot, and that one OnPlayerWeaponShot can be out of sync
				if (shot.hits >= 2)
				{
					valid = false;
					AddRejectedHit(playerid, damagedid, HIT_MULTIPLE_PLAYERS_SHOTGUN, weaponid, shot.hits + 1);
				}
			}
			else if (shot.hits > 0)
			{
				// Sniper doesn't always send OnPlayerWeaponShot, so let's raise the limits
				if (shot.hits >= 6 || (shot.hits >= 3 && weaponid != weapons::SNIPER))
				{
					valid = false;
					AddRejectedHit(playerid, damagedid, HIT_MULTIPLE_PLAYERS, weaponid, shot.hits + 1);
				}
			}

			if (valid && weaponid != weapons::SNIPER)
			{
				float dist = server::GetPlayerDistanceFromPoint(damagedid, shot.hx, shot.hy, shot.hz);

				if (dist > 20.0f)
				{
					if ((!InVehicleOrSurfing(damagedid) && server::GetPlayerSurfingObjectID(damagedid) == kInvalidObject) || dist > 50.0f)
					{
						valid = false;
						AddRejectedHit(playerid, damagedid, HIT_TOO_FAR_FROM_SHOT, weaponid, amx_ftoc(dist));
					}
				}
			}

			shot.hits += 1;
		}

		if (!valid)
		{
			return 0;
		}

		if (npc)
		{
			OnDamageDone(damagedid, amount, playerid, weaponid, bodypart);
		}
		else
		{
			InflictDamage(damagedid, amount, playerid, weaponid, bodypart);
		}

		return 0;
	}

	int OnTakeDamage(int playerid, int issuerid, float amount, int weaponid, int bodypart)
	{
		if (P(playerid).npc)
		{
			return 0;
		}

		UpdateHealthBar(playerid, true);

		if (!IsPlayerSpawned(playerid))
		{
			return 0;
		}

		if (!weapons::IsHighRate(weaponid))
		{
			Debug("OnPlayerTakeDamage(%d took %f from %d by %d on bodypart %d)", playerid, amount, issuerid, weaponid, bodypart);
		}

		// Ignore unreliable and invalid damage
		weapons::Weapon *w = weapons::Get(weaponid);

		if (!w || !w->valid_taken)
		{
			return 0;
		}

		// Carjack damage
		if (weaponid == weapons::COLLISION && Is(amount, 0.0f))
		{
			return 0;
		}

		// From stealth knife, can be any weaponid
		if (Is(amount, 1833.33154296875f))
		{
			return 0;
		}

		if (weaponid == weapons::COLLISION)
		{
			if (g_config.custom_fall_damage)
			{
				return 0;
			}

			// Climb bug
			int anim = server::GetPlayerAnimationIndex(playerid);

			if (anim >= 1061 && anim <= 1067)
			{
				return 0;
			}
		}
		else if (weaponid == weapons::KNIFE && Is(amount, 0.0f))
		{
			// Being knifed client-side
			if (issuerid == playerid)
			{
				return 0;
			}

			if (!ValidPlayer(issuerid) || HasSameTeam(playerid, issuerid))
			{
				UpdateHealthBar(playerid, true);
				return 0;
			}

			int c_playerid = playerid, c_issuerid = issuerid, c_weapon = weaponid, c_bodypart = bodypart;
			float c_amount = amount;

			if (DistanceToPlayer(playerid, issuerid) > w->range + 2.0f || !CallOnPlayerDamage(c_playerid, c_amount, c_issuerid, c_weapon, c_bodypart))
			{
				UpdateHealthBar(playerid, true);
				return 0;
			}

			Player &p = P(playerid);
			p.damage_done_health = p.health;
			p.damage_done_armour = p.armour;

			OnDamageDone(playerid, p.health + p.armour, issuerid, weapons::KNIFE, bodypart);

			int ping = server::GetPlayerPing(playerid);
			PlayerDeath(playerid, "KNIFE", "KILL_Knife_Ped_Die", false, (4000 - ping) > 3000 ? (4000 - ping) : 3000);
			EmitDeath(playerid, issuerid, weapons::KNIFE);

			// Keep the client alive while it plays the stealth kill animation
			server::SetPlayerHealth(playerid, std::numeric_limits<float>::max());
			P(playerid).last_sent_health = -1;

			int anim = server::GetPlayerAnimationIndex(issuerid);
			int forcesync = anim != 747 ? 1 : 2;

			server::SetPlayerFacingAngle(issuerid, server::GetPlayerFacingAngle(playerid));
			server::SetPlayerVelocity(playerid, 0.0f, 0.0f, 0.0f);
			server::SetPlayerVelocity(issuerid, 0.0f, 0.0f, 0.0f);
			server::ApplyAnimation(issuerid, "KNIFE", "KILL_Knife_Player", 4.1f, false, true, true, false, 1800, forcesync);
			return 0;
		}

		// If it's lagcomp, only allow damage that's valid for both modes
		if (g_config.lag_comp_mode && w->valid_taken != 2)
		{
			if (ValidPlayer(issuerid) && (weaponid == weapons::M4 || weaponid == weapons::MINIGUN) && server::GetPlayerState(issuerid) == PLAYER_STATE_DRIVER)
			{
				if (IsVehicleArmedWithWeapon(server::GetPlayerVehicleID(issuerid), weaponid))
				{
					weaponid = weaponid == weapons::MINIGUN ? weapons::VEHICLE_MINIGUN : weapons::VEHICLE_M4;
				}
				else
				{
					return 0;
				}
			}
			else if (!ValidPlayer(issuerid) || !P(issuerid).npc)
			{
				return 0;
			}
		}

		// Should still be damaged by grenades or fire after someone has died
		if (ValidPlayer(issuerid) && P(issuerid).connected)
		{
			if (HasSameTeam(playerid, issuerid))
			{
				return 0;
			}

			if (P(issuerid).dying && (weapons::IsBullet(weaponid) || weapons::IsMelee(weaponid)) && TickDiff(Tick(), P(issuerid).last_death_tick) > 80)
			{
				return 0;
			}

			if (weaponid == weapons::COLLISION || weaponid == weapons::DROWN)
			{
				return 0;
			}

			if (weaponid == weapons::VEHICLE || weaponid == weapons::HELIBLADES)
			{
				if (server::GetPlayerState(issuerid) != PLAYER_STATE_DRIVER)
				{
					return 0;
				}
			}

			// Both players should see each other, if playerid claims to keep issuerid valid
			if ((!server::IsPlayerStreamedIn(playerid, issuerid) && !IsPlayerPaused(issuerid)) || !server::IsPlayerStreamedIn(issuerid, playerid))
			{
				if (g_config.lag_comp_mode)
				{
					// Probably fake or belated damage, so let's just reset issuerid
					issuerid = kInvalidPlayer;
				}
				else
				{
					// Also could be a sniper bug, reject the hit then
					AddRejectedHit(playerid, issuerid, HIT_UNSTREAMED, weaponid, issuerid);
					return 0;
				}
			}
		}
		else if (issuerid != kInvalidPlayer)
		{
			// Probably fake damage, so let's just reset issuerid
			issuerid = kInvalidPlayer;
		}

		float bullets = 0.0f;
		int victim = playerid;
		int err = ProcessDamage(victim, issuerid, amount, weaponid, bodypart, bullets);

		if (err)
		{
			if (err == WC_INVALID_DAMAGE)
			{
				AddRejectedHit(issuerid, playerid, HIT_INVALID_DAMAGE, weaponid, amx_ftoc(amount));
			}

			if (err != WC_INVALID_DISTANCE)
			{
				CallOnInvalidWeaponDamage(issuerid, playerid, amount, weaponid, bodypart, err, false);
			}

			return 0;
		}

		if (weapons::IsBullet(weaponid) && ValidPlayer(issuerid))
		{
			float dist = DistanceToPlayer(playerid, issuerid);
			weapons::Weapon *bw = weapons::Get(weaponid);

			if (dist > bw->range + 2.0f)
			{
				AddRejectedHit(issuerid, playerid, HIT_OUT_OF_RANGE, weaponid, amx_ftoc(dist), amx_ftoc(bw->range));
				return 0;
			}
		}

		InflictDamage(playerid, amount, issuerid, weaponid, bodypart);
		return 0;
	}

	int OnWeaponShot(int playerid, int weaponid, int hittype, int hitid, float fx, float fy, float fz)
	{
		Player &p = P(playerid);

		timers::Stop(timers::VENDING_USE, playerid);
		p.last_shot.valid = false;

		uint32_t tick = Tick();

		if (p.cbug_froze && TickDiff(tick, p.cbug_froze) < 900)
		{
			return 0;
		}

		p.cbug_froze = 0;

		int damagedid = kInvalidPlayer;

		if (hittype == BULLET_HIT_TYPE_PLAYER && hitid != kInvalidPlayer)
		{
			if (!ValidPlayer(hitid) || !P(hitid).connected)
			{
				AddRejectedHit(playerid, hitid, HIT_DISCONNECTED, weaponid, hitid);
				return 0;
			}

			damagedid = hitid;
		}

		if (hittype < BULLET_HIT_TYPE_NONE || hittype > BULLET_HIT_TYPE_PLAYER_OBJECT)
		{
			AddRejectedHit(playerid, damagedid, HIT_INVALID_HITTYPE, weaponid, hittype);
			return 0;
		}

		if (!IsPlayerSpawned(playerid) && TickDiff(tick, p.last_death_tick) > 80)
		{
			AddRejectedHit(playerid, damagedid, HIT_NOT_SPAWNED, weaponid);
			return 0;
		}

		if (!weapons::IsBullet(weaponid))
		{
			AddRejectedHit(playerid, damagedid, HIT_INVALID_WEAPON, weaponid);
			return 0;
		}

		weapons::Weapon *w = weapons::Get(weaponid);

		float ox, oy, oz, hx, hy, hz, x, y, z;
		server::GetPlayerPos(playerid, x, y, z);
		server::GetPlayerLastShotVectors(playerid, ox, oy, oz, hx, hy, hz);

		float length = VectorSize(ox - hx, oy - hy, oz - hz);
		float origin_dist = VectorSize(ox - x, oy - y, oz - z);

		if (origin_dist > 15.0f)
		{
			if ((!InVehicleOrSurfing(playerid) && server::GetPlayerSurfingObjectID(playerid) == kInvalidObject) || origin_dist > 50.0f)
			{
				AddRejectedHit(playerid, damagedid, HIT_TOO_FAR_FROM_ORIGIN, weaponid, amx_ftoc(origin_dist));
				return 0;
			}
		}

		// Shot exceeding the max range?
		if (hittype != BULLET_HIT_TYPE_NONE)
		{
			if (length > w->range)
			{
				if (hittype == BULLET_HIT_TYPE_PLAYER)
				{
					AddRejectedHit(playerid, damagedid, HIT_OUT_OF_RANGE, weaponid, amx_ftoc(length), amx_ftoc(w->range));
				}
				return 0;
			}

			if (hittype == BULLET_HIT_TYPE_PLAYER)
			{
				if (server::IsPlayerInAnyVehicle(playerid) && server::GetPlayerVehicleID(playerid) == server::GetPlayerVehicleID(hitid))
				{
					AddRejectedHit(playerid, damagedid, HIT_SAME_VEHICLE, weaponid);
					return 0;
				}

				float dist = server::GetPlayerDistanceFromPoint(hitid, hx, hy, hz);

				if (dist > 20.0f)
				{
					if ((!InVehicleOrSurfing(hitid) && server::GetPlayerSurfingObjectID(hitid) == kInvalidObject) || dist > 50.0f)
					{
						AddRejectedHit(playerid, damagedid, HIT_TOO_FAR_FROM_SHOT, weaponid, amx_ftoc(dist));
						return 0;
					}
				}
			}
		}

		int idx = (p.shot_idx + 1) % kRateSamples;
		p.shot_idx = idx;
		p.shot_ticks[idx] = tick;
		p.shot_weapons[idx] = weaponid;
		p.shots_fired += 1;
		p.stats[STAT_SHOTS_FIRED] += 1;

		ShotInfo &shot = p.last_shot;
		shot.tick = tick;
		shot.weapon = weaponid;
		shot.hittype = hittype;
		shot.hitid = hitid;
		shot.x = fx;
		shot.y = fy;
		shot.z = fz;
		shot.ox = ox;
		shot.oy = oy;
		shot.oz = oz;
		shot.hx = hx;
		shot.hy = hy;
		shot.hz = hz;
		shot.length = length;
		shot.hits = 0;

		bool multiple = false;
		int avg_rate = AverageShootRate(playerid, g_config.max_shoot_rate_samples, &multiple);

		// Bullet flood? Could be either a cheat or just lag
		if (avg_rate != -1)
		{
			if (multiple)
			{
				if (avg_rate < 100)
				{
					AddRejectedHit(playerid, damagedid, SHOOTING_RATE_TOO_FAST_MULTIPLE, weaponid, avg_rate, g_config.max_shoot_rate_samples);
					return 0;
				}
			}
			else if (w->shoot_rate - avg_rate > 20)
			{
				AddRejectedHit(playerid, damagedid, SHOOTING_RATE_TOO_FAST, weaponid, avg_rate, g_config.max_shoot_rate_samples, w->shoot_rate);
				return 0;
			}
		}

		// Destroy vehicles with passengers in them
		if (hittype == BULLET_HIT_TYPE_VEHICLE)
		{
			if (!ValidVehicle(hitid) || !server::IsValidVehicle(hitid))
			{
				AddRejectedHit(playerid, damagedid, HIT_INVALID_VEHICLE, weaponid, hitid);
				return 0;
			}

			if (!server::IsVehicleStreamedIn(hitid, playerid))
			{
				AddRejectedHit(playerid, damagedid, HIT_UNSTREAMED, weaponid, hitid);
				return 0;
			}

			// Shouldn't be possible to damage the vehicle you're in
			if (hitid == server::GetPlayerVehicleID(playerid))
			{
				AddRejectedHit(playerid, damagedid, HIT_OWN_VEHICLE, weaponid);
				return 0;
			}

			if (g_config.vehicle_passenger_damage || g_config.vehicle_unoccupied_damage)
			{
				bool has_driver = false, has_passenger = false;

				for (int other = 0; other < g_max_players; ++other)
				{
					if (other == playerid || !P(other).connected)
					{
						continue;
					}

					if (server::GetPlayerVehicleID(other) != hitid)
					{
						continue;
					}

					if (server::GetPlayerVehicleSeat(other) != 0)
					{
						has_passenger = true;
					}
					else
					{
						has_driver = true;
						break;
					}
				}

				if (!has_driver)
				{
					float health = server::GetVehicleHealth(hitid);
					bool apply_damage = false;

					if (g_config.vehicle_passenger_damage && has_passenger)
					{
						apply_damage = true;
					}
					else if (g_config.vehicle_unoccupied_damage && !has_passenger)
					{
						// vehicles start on fire below 250 hp
						apply_damage = health >= 250.0f;
					}

					if (apply_damage)
					{
						health -= weapons::IsShotgun(weaponid) ? 120.0f : w->damage * 3.0f;

						if (has_passenger)
						{
							if (health <= 0.0f)
							{
								health = 0.0f;
							}
						}
						else if (health < 250.0f)
						{
							if (!g_vehicles[hitid].kill_pending)
							{
								health = 249.0f;
								g_vehicles[hitid].kill_pending = true;
								timers::Args args;
								args.a = playerid;
								timers::Start(timers::VEHICLE_KILL, hitid, 6000, args);
							}
						}

						server::SetVehicleHealth(hitid, health);
					}
				}
			}
		}

		return 1;
	}

	void OnWeaponShotResult(int playerid, int hittype, int hitid, bool valid)
	{
		P(playerid).last_shot.valid = valid;

		if (valid && hittype == BULLET_HIT_TYPE_VEHICLE && ValidVehicle(hitid))
		{
			g_vehicles[hitid].last_shooter = playerid;
		}
	}
}
