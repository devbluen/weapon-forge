// Incoming sync packet processing. weapon-config does this in Pawn with
// dozens of BS_* calls per packet (through Pawn.RakNet); here the network
// layer of each server (net_samp.cpp / omp_component.cpp) hands the packet
// bytes to the plugin and they are patched in place.
#include "core.hpp"
#include "server.hpp"
#include "weapons.hpp"

#include <cmath>
#include <cstring>

namespace wf
{
	namespace
	{
		constexpr int kPlayerSync = 207;
		constexpr int kVehicleSync = 200;
		constexpr int kPassengerSync = 211;
		constexpr int kAimSync = 203;

		// OnFoot sync offsets (after the packet id)
		enum
		{
			OF_LR = 0,
			OF_UD = 2,
			OF_KEYS = 4,
			OF_QUAT = 18,
			OF_HEALTH = 34,
			OF_ARMOUR = 35,
			OF_WEAPON = 36,
			OF_SPECIAL = 37,
			OF_VELOCITY = 38,
			OF_ANIM = 64,
			OF_SIZE = 68,
		};

		uint16_t Read16(const unsigned char *p)
		{
			uint16_t v;
			std::memcpy(&v, p, sizeof(v));
			return v;
		}

		void Write16(unsigned char *p, uint16_t v)
		{
			std::memcpy(p, &v, sizeof(v));
		}

		float ReadFloat(const unsigned char *p)
		{
			float v;
			std::memcpy(&v, p, sizeof(v));
			return v;
		}

		void WriteFloat(unsigned char *p, float v)
		{
			std::memcpy(p, &v, sizeof(v));
		}

		void ProcessOnFoot(int playerid, unsigned char *sync)
		{
			Player &p = P(playerid);
			uint16_t keys = Read16(sync + OF_KEYS);
			int weapon = sync[OF_WEAPON] & 0x3F;
			int anim = Read16(sync + OF_ANIM);

			// Detonator crasher: KEY_HANDBRAKE/KEY_AIM with the bomb
			if (weapon == weapons::BOMB)
			{
				keys &= ~KEY_HANDBRAKE;
			}

			if (g_config.disable_sync_bugs)
			{
				// Prevent "ghost shooting" bugs
				if (weapons::IsBullet(weapon))
				{
					if ((anim >= 1222 && anim <= 1236) // PED_RUN_*
						|| anim == 1249					// PED_SWAT_RUN
						|| (anim >= 1275 && anim <= 1287) // PED_WOMAN_(RUN/WALK)_*
						|| anim == 459					// FAT_FATRUN_ARMED
						|| (anim >= 908 && anim <= 909)	// MUSCULAR_MUSCLERUN*
						|| anim == 1274					// PED_WEAPON_CROUCH
						|| anim == 1266					// PED_WALK_PLAYER
						|| (anim >= 1241 && anim <= 1242) // PED_SHOT_PARTIAL(_B)
						|| (anim >= 17 && anim <= 27)	// Baseball bat
						|| (anim >= 745 && anim <= 760)	// Knife
						|| (anim >= 1545 && anim <= 1554) // Sword
						|| (anim >= 471 && anim <= 507) || (anim >= 1135 && anim <= 1151)) // Fight
					{
						// Only remove action key if holding aim
						if (keys & KEY_HANDBRAKE)
						{
							keys &= ~KEY_ACTION;
						}

						keys &= ~(KEY_FIRE | KEY_HANDBRAKE);
					}
				}
				else if (weapon == weapons::SPRAYCAN || weapon == weapons::FIREEXTINGUISHER || weapon == weapons::FLAMETHROWER)
				{
					if (!(anim >= 1160 && anim <= 1167))
					{
						if (keys & KEY_HANDBRAKE)
						{
							keys &= ~KEY_ACTION;
						}

						keys &= ~(KEY_FIRE | KEY_HANDBRAKE);
					}
				}
				else if (weapon == weapons::GRENADE)
				{
					if (!(anim >= 644 && anim <= 646))
					{
						keys &= ~KEY_ACTION;
					}
				}
			}

			Write16(sync + OF_KEYS, keys);

			if (p.sync_frozen)
			{
				std::memcpy(sync, p.last_sync, OF_SIZE);
				keys = Read16(sync + OF_KEYS);
				weapon = sync[OF_WEAPON] & 0x3F;
			}
			else
			{
				std::memcpy(p.temp_sync, sync, OF_SIZE);
				p.temp_written = true;
			}

			if (p.fake_health != 255)
			{
				sync[OF_HEALTH] = static_cast<unsigned char>(p.fake_health);
			}

			if (p.fake_armour != 255)
			{
				sync[OF_ARMOUR] = static_cast<unsigned char>(p.fake_armour);
			}

			if (!std::isnan(p.fake_quat[0]))
			{
				for (int i = 0; i < 4; ++i)
				{
					WriteFloat(sync + OF_QUAT + i * 4, p.fake_quat[i]);
				}
			}

			if (weapon == weapons::KNIFE && !g_config.knife_sync)
			{
				keys &= ~KEY_HANDBRAKE;
			}
			else if (weapon == weapons::NIGHTVISION || weapon == weapons::INFRARED)
			{
				keys &= ~KEY_FIRE;

				// Keep preventing for some more packets
				p.goggles_tick = Tick();
				p.goggles_used = 1;
			}
			else if (p.goggles_used)
			{
				if (p.goggles_used == 2 && TickDiff(Tick(), p.goggles_tick) > 40)
				{
					p.goggles_used = 0;
				}
				else
				{
					keys &= ~KEY_FIRE;
					p.goggles_tick = Tick();
					p.goggles_used = 2;
				}
			}

			Write16(sync + OF_KEYS, keys);
		}
	}

	void FreezeSyncPacket(int playerid, bool toggle)
	{
		Player &p = P(playerid);

		Write16(p.last_sync + OF_KEYS, 0);
		Write16(p.last_sync + OF_UD, 0);
		Write16(p.last_sync + OF_LR, 0);
		p.last_sync[OF_SPECIAL] = SPECIAL_ACTION_NONE;

		for (int i = 0; i < 3; ++i)
		{
			WriteFloat(p.last_sync + OF_VELOCITY + i * 4, 0.0f);
		}

		p.sync_frozen = toggle;
	}

	void SetFakeHealth(int playerid, int health)
	{
		P(playerid).fake_health = health;
	}

	void SetFakeArmour(int playerid, int armour)
	{
		P(playerid).fake_armour = armour;
	}

	void SetFakeFacingAngle(int playerid, float angle)
	{
		Player &p = P(playerid);

		if (std::isnan(angle))
		{
			for (float &q : p.fake_quat)
			{
				q = kNaN;
			}
			return;
		}

		// Rotation quaternion for (0, 0, angle) as computed by weapon-config
		constexpr float kDeg = 3.14159265358979323846f / 180.0f;
		float cx = std::cos(0.0f), sx = std::sin(0.0f);
		float cy = std::cos(0.0f), sy = std::sin(0.0f);
		float cz = std::cos(-0.5f * angle * kDeg), sz = std::sin(-0.5f * angle * kDeg);

		p.fake_quat[0] = cx * cy * cz + sx * sy * sz;
		p.fake_quat[1] = cx * sy * sz + sx * cy * cz;
		p.fake_quat[2] = cx * sy * cz - sx * cy * sz;
		p.fake_quat[3] = cx * cy * sz - sx * sy * cz;
	}

	void CommitSyncData(int playerid)
	{
		Player &p = P(playerid);

		if (p.temp_written && server::GetPlayerState(playerid) == PLAYER_STATE_ONFOOT)
		{
			std::memcpy(p.last_sync, p.temp_sync, OF_SIZE);
			p.temp_written = false;
		}
	}

	void ProcessIncomingPacket(int playerid, unsigned char *data, size_t bytes)
	{
		if (!ValidPlayer(playerid) || !data || bytes < 1)
		{
			return;
		}

		Player &p = P(playerid);
		int packetid = data[0];

		switch (packetid)
		{
		case kPlayerSync:
			if (bytes >= 1 + OF_SIZE)
			{
				ProcessOnFoot(playerid, data + 1);
			}
			break;

		case kVehicleSync:
			// vehicle(2) lr(2) ud(2) keys(2) quat(16) pos(12) vel(12) carhealth(4) health(1) armour(1)
			if (bytes >= 1 + 54)
			{
				if (p.fake_health != 255)
				{
					data[1 + 52] = static_cast<unsigned char>(p.fake_health);
				}
				if (p.fake_armour != 255)
				{
					data[1 + 53] = static_cast<unsigned char>(p.fake_armour);
				}
			}
			break;

		case kPassengerSync:
			// vehicle(2) seat(1) weapon(1) health(1) armour(1)
			if (bytes >= 1 + 6)
			{
				if (p.fake_health != 255)
				{
					data[1 + 4] = static_cast<unsigned char>(p.fake_health);
				}
				if (p.fake_armour != 255)
				{
					data[1 + 5] = static_cast<unsigned char>(p.fake_armour);
				}
			}
			break;

		case kAimSync:
			// cammode(1) front(12) pos(12) aimz(4)
			if (bytes >= 1 + 29)
			{
				int weapon = p.last_sync[OF_WEAPON] & 0x3F;

				// Fix first-person up/down aim sync
				if ((weapon >= weapons::SNIPER && weapon <= weapons::HEATSEEKER) || weapon == weapons::CAMERA)
				{
					float aim_z = -ReadFloat(data + 1 + 1 + 8);

					if (aim_z > 1.0f)
					{
						aim_z = 1.0f;
					}
					else if (aim_z < -1.0f)
					{
						aim_z = -1.0f;
					}

					WriteFloat(data + 1 + 25, aim_z);
				}
			}
			break;
		}
	}
}
