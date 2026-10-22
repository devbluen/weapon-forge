#pragma once

#include <string>

namespace weapons
{
	enum : int
	{
		UNARMED = 0,
		BRASSKNUCKLE = 1,
		KNIFE = 4,
		CHAINSAW = 9,
		CANE = 15,
		GRENADE = 16,
		TEARGAS = 17,
		MOLOTOV = 18,
		VEHICLE_M4 = 19,
		VEHICLE_MINIGUN = 20,
		VEHICLE_ROCKETLAUNCHER = 21,
		COLT45 = 22,
		SILENCED = 23,
		DEAGLE = 24,
		SHOTGUN = 25,
		SAWEDOFF = 26,
		SHOTGSPA = 27,
		UZI = 28,
		MP5 = 29,
		AK47 = 30,
		M4 = 31,
		TEC9 = 32,
		RIFLE = 33,
		SNIPER = 34,
		ROCKETLAUNCHER = 35,
		HEATSEEKER = 36,
		FLAMETHROWER = 37,
		MINIGUN = 38,
		SATCHEL = 39,
		BOMB = 40,
		SPRAYCAN = 41,
		FIREEXTINGUISHER = 42,
		CAMERA = 43,
		NIGHTVISION = 44,
		INFRARED = 45,
		PARACHUTE = 46,
		PISTOLWHIP = 48,
		VEHICLE = 49,
		HELIBLADES = 50,
		EXPLOSION = 51,
		CARPARK = 52,
		DROWN = 53,
		COLLISION = 54,
		UNKNOWN = 55,
		COUNT = 56,
	};

	enum DamageType : int
	{
		DAMAGE_TYPE_MULTIPLIER = 0,
		DAMAGE_TYPE_STATIC = 1,
		DAMAGE_TYPE_RANGE_MULTIPLIER = 2,
		DAMAGE_TYPE_RANGE = 3,
	};

	constexpr int kMaxDamageRanges = 8;
	constexpr int kBodyParts = 10; // 0..9 (3 torso .. 9 head)

	struct Weapon
	{
		bool valid_given;    // accepted from OnPlayerGiveDamage
		int valid_taken;     // 0: no, 1: OnPlayerTakeDamage, 2: both lagcomp modes
		float damage;
		int damage_type;
		float range;         // 0 = no range check
		int shoot_rate;      // fastest possible gap between shots (ms)
		bool armour_affects; // custom armour rule: damage hits armour
		bool armour_torso;   // custom armour rule: only on torso hits
		int range_steps;
		float range_ranges[kMaxDamageRanges];
		float range_values[kMaxDamageRanges];
		float bodypart_mul[kBodyParts];
		std::string name;
	};

	void Reset();
	Weapon *Get(int weaponid);
	bool Valid(int weaponid);

	bool IsBullet(int weaponid);
	bool IsMelee(int weaponid);
	bool IsHighRate(int weaponid);
	bool IsShotgun(int weaponid);

	std::string Name(int weaponid);
	// Weapon id usable in SendDeathMessage (custom ids mapped to real ones)
	int DeathMessageId(int weaponid);
}
