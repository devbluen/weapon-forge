#include "weapons.hpp"

namespace weapons
{
	namespace
	{
		// Defaults ported from weapon-config (GTA:SA weapon.dat values).
		struct Default
		{
			bool valid_given;
			int valid_taken;
			float damage;
			int type;
			float range;
			int rate;
			bool armour_affects;
			bool armour_torso;
			const char *name;
		};

		const Default kDefaults[COUNT] = {
			{true, 1, 1.0f, 0, 1.76f, 250, false, false, "Fist"},
			{true, 1, 1.0f, 0, 1.76f, 250, false, false, "Brass knuckles"},
			{true, 1, 1.0f, 0, 1.76f, 250, false, false, "Golf club"},
			{true, 1, 1.0f, 0, 1.76f, 250, false, false, "Nightstick"},
			{true, 1, 1.0f, 0, 1.76f, 250, false, false, "Knife"},
			{true, 1, 1.0f, 0, 1.76f, 250, false, false, "Bat"},
			{true, 1, 1.0f, 0, 1.6f, 250, false, false, "Shovel"},
			{true, 1, 1.0f, 0, 1.76f, 250, false, false, "Pool cue"},
			{true, 1, 1.0f, 0, 1.76f, 250, false, false, "Katana"},
			{true, 1, 1.0f, 0, 1.76f, 30, false, false, "Chainsaw"},
			{true, 1, 1.0f, 0, 1.76f, 250, false, false, "Purple dildo"},
			{true, 1, 1.0f, 0, 1.76f, 250, false, false, "Dildo"},
			{true, 1, 1.0f, 0, 1.76f, 250, false, false, "Vibrator"},
			{true, 1, 1.0f, 0, 1.76f, 250, false, false, "Vibrator"},
			{true, 1, 1.0f, 0, 1.76f, 250, false, false, "Flowers"},
			{true, 1, 1.0f, 0, 1.76f, 250, false, false, "Cane"},
			{false, 0, 82.5f, 0, 40.0f, 0, false, false, "Grenade"},
			{false, 0, 0.0f, 1, 40.0f, 0, false, false, "Tear gas"},
			{false, 0, 1.0f, 0, 40.0f, 0, false, false, "Molotov"},
			{false, 0, 9.9f, 1, 90.0f, 90, true, true, "Vehicle gun"},
			{false, 0, 46.2f, 1, 75.0f, 20, true, true, "Vehicle gun"},
			{false, 0, 82.5f, 0, 0.0f, 0, false, false, "Vehicle gun"},
			{true, 1, 8.25f, 1, 35.0f, 160, true, true, "Colt 45"},
			{true, 1, 13.2f, 1, 35.0f, 120, true, true, "Silenced pistol"},
			{true, 1, 46.2f, 1, 35.0f, 120, true, true, "Deagle"},
			{true, 1, 3.3f, 1, 40.0f, 800, true, true, "Shotgun"},
			{true, 1, 3.3f, 1, 35.0f, 120, true, true, "Sawn-off shotgun"},
			{true, 1, 4.95f, 1, 40.0f, 120, true, true, "Combat shotgun"},
			{true, 1, 6.6f, 1, 35.0f, 50, true, true, "Mac-10"},
			{true, 1, 8.25f, 1, 45.0f, 90, true, true, "MP5"},
			{true, 1, 9.9f, 1, 70.0f, 90, true, true, "AK-47"},
			{true, 1, 9.9f, 1, 90.0f, 90, true, true, "M4"},
			{true, 1, 6.6f, 1, 35.0f, 50, true, true, "Tec-9"},
			{true, 1, 24.75f, 1, 100.0f, 800, true, true, "Country rifle"},
			{true, 1, 41.25f, 1, 320.0f, 800, true, true, "Sniper"},
			{false, 0, 82.5f, 0, 55.0f, 0, false, false, "Rocket launcher"},
			{false, 0, 82.5f, 0, 55.0f, 0, false, false, "Heat seeking RPG"},
			{false, 2, 1.0f, 0, 5.1f, 0, false, false, "Flamethrower"},
			{true, 1, 46.2f, 1, 75.0f, 20, true, true, "Minigun"},
			{false, 0, 82.5f, 0, 40.0f, 0, false, false, "Satchel"},
			{false, 0, 0.0f, 0, 25.0f, 0, false, false, "Detonator"},
			{true, 1, 0.33f, 1, 6.1f, 10, false, false, "Spraycan"},
			{true, 1, 0.33f, 1, 10.1f, 10, false, false, "Fire extinguisher"},
			{false, 0, 0.0f, 0, 100.0f, 0, true, false, "Camera"},
			{false, 0, 0.0f, 0, 100.0f, 0, true, false, "Night vision goggles"},
			{false, 0, 0.0f, 0, 100.0f, 0, true, false, "Infrared goggles"},
			{true, 1, 0.0f, 0, 1.76f, 0, true, false, "Parachute"},
			{false, 0, 0.0f, 0, 0.0f, 0, true, false, "Fake pistol"},
			{false, 0, 2.64f, 1, 0.0f, 400, false, false, "Pistol whip"},
			{false, 2, 9.9f, 1, 0.0f, 0, false, false, "Vehicle"},
			{false, 2, 330.0f, 1, 0.0f, 0, false, true, "Helicopter blades"},
			{false, 2, 82.5f, 0, 0.0f, 0, false, false, "Explosion"},
			{false, 0, 1.0f, 0, 0.0f, 0, false, false, "Car parking"},
			{false, 2, 1.0f, 0, 0.0f, 0, false, false, "Drowning"},
			{false, 2, 165.0f, 0, 0.0f, 0, false, false, "Splat"},
			{false, 0, 1.0f, 0, 0.0f, 0, false, false, "Unknown"},
		};

		Weapon g_weapons[COUNT];
	}

	void Reset()
	{
		for (int i = 0; i < COUNT; ++i)
		{
			const Default &d = kDefaults[i];
			Weapon &w = g_weapons[i];

			w.valid_given = d.valid_given;
			w.valid_taken = d.valid_taken;
			w.damage = d.damage;
			w.damage_type = d.type;
			w.range = d.range;
			w.shoot_rate = d.rate;
			w.armour_affects = d.armour_affects;
			w.armour_torso = d.armour_torso;
			w.range_steps = 0;
			w.name = d.name;

			for (int j = 0; j < kMaxDamageRanges; ++j)
			{
				w.range_ranges[j] = 0.0f;
				w.range_values[j] = 0.0f;
			}

			for (float &m : w.bodypart_mul)
			{
				m = 1.0f;
			}
		}
	}

	Weapon *Get(int weaponid)
	{
		if (!Valid(weaponid))
		{
			return nullptr;
		}
		return &g_weapons[weaponid];
	}

	bool Valid(int weaponid)
	{
		return weaponid >= 0 && weaponid < COUNT;
	}

	bool IsBullet(int weaponid)
	{
		return (weaponid >= COLT45 && weaponid <= SNIPER) || weaponid == MINIGUN;
	}

	bool IsMelee(int weaponid)
	{
		return (weaponid >= UNARMED && weaponid <= CANE) || weaponid == PISTOLWHIP;
	}

	bool IsHighRate(int weaponid)
	{
		switch (weaponid)
		{
		case FLAMETHROWER:
		case SPRAYCAN:
		case FIREEXTINGUISHER:
		case CARPARK:
		case DROWN:
			return true;
		default:
			return false;
		}
	}

	bool IsShotgun(int weaponid)
	{
		return weaponid >= SHOTGUN && weaponid <= SHOTGSPA;
	}

	std::string Name(int weaponid)
	{
		if (!Valid(weaponid))
		{
			return "Weapon " + std::to_string(weaponid);
		}
		return g_weapons[weaponid].name;
	}

	int DeathMessageId(int weaponid)
	{
		switch (weaponid)
		{
		case VEHICLE_M4:
			return M4;
		case VEHICLE_MINIGUN:
			return MINIGUN;
		case VEHICLE_ROCKETLAUNCHER:
			return ROCKETLAUNCHER;
		case PISTOLWHIP:
			return UNARMED;
		case CARPARK:
			return VEHICLE;
		case UNKNOWN:
			return DROWN;
		default:
			return weaponid;
		}
	}
}
