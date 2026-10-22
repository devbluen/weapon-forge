#pragma once

#include "sdk/amx.h"

#include <cmath>
#include <cstdint>
#include <limits>
#include <string>

namespace wf
{
	constexpr int kMaxPlayers = 1000;
	constexpr int kMaxVehicles = 2000;
	constexpr int kInvalidPlayer = 0xFFFF;
	constexpr int kInvalidVehicle = 0xFFFF;
	constexpr int kInvalidTextDraw = 0xFFFF;
	constexpr int kInvalidObject = 0xFFFF;
	constexpr int kNoTeam = 255;
	constexpr int kFeedHeight = 5;
	constexpr int kMaxRejectedHits = 15;
	constexpr int kPrevHits = 10;
	constexpr int kRateSamples = 10;
	constexpr int kDeathWorld = 0x00DEAD00;
	constexpr float kNaN = std::numeric_limits<float>::quiet_NaN();

	// Player states
	enum
	{
		PLAYER_STATE_NONE = 0,
		PLAYER_STATE_ONFOOT = 1,
		PLAYER_STATE_DRIVER = 2,
		PLAYER_STATE_PASSENGER = 3,
		PLAYER_STATE_WASTED = 7,
		PLAYER_STATE_SPAWNED = 8,
		PLAYER_STATE_SPECTATING = 9,
	};

	enum
	{
		KEY_ACTION = 1,
		KEY_CROUCH = 2,
		KEY_FIRE = 4,
		KEY_SECONDARY_ATTACK = 16,
		KEY_HANDBRAKE = 128,
		KEY_LOOK_RIGHT = 256,
		KEY_LOOK_LEFT = 1024,
	};

	enum
	{
		SPECIAL_ACTION_NONE = 0,
		SPECIAL_ACTION_DUCK = 1,
		SPECIAL_ACTION_USEJETPACK = 2,
	};

	enum
	{
		BULLET_HIT_TYPE_NONE = 0,
		BULLET_HIT_TYPE_PLAYER = 1,
		BULLET_HIT_TYPE_VEHICLE = 2,
		BULLET_HIT_TYPE_OBJECT = 3,
		BULLET_HIT_TYPE_PLAYER_OBJECT = 4,
	};

	// OnInvalidWeaponDamage errors (same values as weapon-config)
	enum
	{
		WC_NO_ERROR,
		WC_NO_ISSUER,
		WC_NO_DAMAGED,
		WC_INVALID_DAMAGE,
		WC_INVALID_DISTANCE,
	};

	// Rejected hit reasons (same values as weapon-config)
	enum
	{
		HIT_NO_DAMAGEDID,
		HIT_INVALID_WEAPON,
		HIT_LAST_SHOT_INVALID,
		HIT_MULTIPLE_PLAYERS,
		HIT_MULTIPLE_PLAYERS_SHOTGUN,
		HIT_DYING_PLAYER,
		HIT_SAME_TEAM,
		HIT_UNSTREAMED,
		HIT_INVALID_HITTYPE,
		HIT_BEING_RESYNCED,
		HIT_NOT_SPAWNED,
		HIT_OUT_OF_RANGE,
		HIT_TOO_FAR_FROM_SHOT,
		SHOOTING_RATE_TOO_FAST,
		SHOOTING_RATE_TOO_FAST_MULTIPLE,
		HIT_RATE_TOO_FAST,
		HIT_RATE_TOO_FAST_MULTIPLE,
		HIT_TOO_FAR_FROM_ORIGIN,
		HIT_INVALID_DAMAGE,
		HIT_SAME_VEHICLE,
		HIT_OWN_VEHICLE,
		HIT_INVALID_VEHICLE,
		HIT_DISCONNECTED,
		HIT_PROTECTED, // weapon-forge: target has god mode / spawn protection
		HIT_REASON_COUNT
	};

	// Integer statistics (WF_GetPlayerStat)
	enum
	{
		STAT_SHOTS_FIRED,
		STAT_SHOTS_HIT,
		STAT_HITS_TAKEN,
		STAT_HEADSHOTS_GIVEN,
		STAT_HEADSHOTS_TAKEN,
		STAT_KILLS,
		STAT_DEATHS,
		STAT_ASSISTS,
		STAT_REJECTED_HITS,
		STAT_INT_COUNT
	};

	// Float statistics (WF_GetPlayerStatFloat)
	enum
	{
		STATF_DAMAGE_GIVEN,
		STATF_DAMAGE_TAKEN,
		STATF_HEALTH_LOST,
		STATF_ARMOUR_LOST,
		STATF_COUNT
	};

	struct ShotInfo
	{
		uint32_t tick = 0;
		int weapon = 0;
		int hittype = 0;
		int hitid = 0;
		int hits = 0;
		float x = 0, y = 0, z = 0;
		float ox = 0, oy = 0, oz = 0;
		float hx = 0, hy = 0, hz = 0;
		float length = 0;
		bool valid = false;
	};

	struct HitInfo
	{
		uint32_t tick = 0;
		int issuer = kInvalidPlayer;
		int weapon = 0;
		float amount = 0;
		float health = 0;
		float armour = 0;
		int bodypart = 0;
	};

	struct FeedHit
	{
		int issuer = kInvalidPlayer;
		std::string name;
		float amount = 0;
		int weapon = 0;
		uint32_t tick = 0;
	};

	struct RejectedHit
	{
		int64_t time = 0;
		int hour = 0, minute = 0, second = 0;
		int weapon = 0;
		int reason = 0;
		cell info1 = 0, info2 = 0, info3 = 0;
		std::string name;
	};

	struct Player
	{
		bool connected = false;
		bool npc = false;

		float max_health = 100.0f;
		float health = 100.0f;
		float max_armour = 100.0f;
		float armour = 0.0f;
		int last_sent_health = -1;
		int last_sent_armour = -1;
		float damage_done_health = 0.0f;
		float damage_done_armour = 0.0f;

		int last_explosive = 0;
		int team = kNoTeam;
		int spectating = kInvalidPlayer;
		int world = 0;

		uint32_t last_update_tick = 0;
		uint32_t last_stop_tick = 0;
		uint32_t last_vehicle_tick = 0;
		uint32_t last_death_tick = 0;
		int64_t last_vehicle_enter_time = 0;

		bool dying = false;
		bool true_death = true;
		bool in_class_selection = false;
		bool restore_teleport = false;
		bool already_connected = false;
		bool first_spawn = true;

		// Health bar
		bool health_bar_enabled = true;
		bool health_bar_visible = false;
		int hb_border = kInvalidTextDraw;
		int hb_background = kInvalidTextDraw;
		int hb_foreground = kInvalidTextDraw;
		float hb_pos[2] = {kNaN, kNaN};
		float hb_size[2] = {kNaN, kNaN};
		float hb_padding[4] = {kNaN, kNaN, kNaN, kNaN};
		int hb_border_color = 0;
		int hb_bg_color = 0;
		int hb_fg_color = 0;

		// Damage feed
		int feed_mode = -1; // -1 = global setting, 0 = off, 1 = on
		int feed_given_td = kInvalidTextDraw;
		int feed_taken_td = kInvalidTextDraw;
		FeedHit feed_given[kFeedHeight];
		FeedHit feed_taken[kFeedHeight];
		uint32_t feed_update_tick = 0;

		// Fall damage
		int last_anim = -1;
		float last_z_velo = 0.0f;
		float last_z = 0.0f;

		// C-bug
		bool cbug_allowed = true;
		uint32_t cbug_froze = 0;

		// Shots / hits
		int shots_fired = 0;
		int hits_issued = 0;
		ShotInfo last_shot;
		uint32_t shot_ticks[kRateSamples] = {};
		int shot_weapons[kRateSamples] = {};
		int shot_idx = 0;
		uint32_t hit_ticks[kRateSamples] = {};
		int hit_weapons[kRateSamples] = {};
		int hit_idx = 0;

		RejectedHit rejected[kMaxRejectedHits];
		int rejected_idx = 0;

		HitInfo prev_hits[kPrevHits];
		int prev_hit_idx = 0;

		// Sync (Pawn.RakNet)
		bool sync_frozen = false;
		bool temp_written = false;
		uint8_t last_sync[68] = {};
		uint8_t temp_sync[68] = {};
		int fake_health = 255;
		int fake_armour = 255;
		float fake_quat[4] = {kNaN, kNaN, kNaN, kNaN};
		int goggles_used = 0;
		uint32_t goggles_tick = 0;

		// weapon-forge extras
		bool god_mode = false;
		uint32_t protected_until = 0;
		float given_mul = 1.0f;
		float taken_mul = 1.0f;
		uint32_t last_damage_tick = 0;
		int last_attacker = kInvalidPlayer;
		bool in_combat = false;
		float regen_rate = -1.0f; // -1 = global
		int regen_delay = -1;
		float regen_accum = 0.0f;
		float bleed_dps = 0.0f;
		uint32_t bleed_until = 0;
		uint32_t bleed_next = 0;
		int bleed_issuer = kInvalidPlayer;
		int bleed_weapon = 55;
		int stats[STAT_INT_COUNT] = {};
		float stats_f[STATF_COUNT] = {};
	};

	struct Vehicle
	{
		int last_shooter = kInvalidPlayer;
		bool death_reported = false;
		bool kill_pending = false;
	};

	struct Config
	{
		int lag_comp_mode = 1;
		bool armour_rules = false;
		bool torso_rules = false;
		int damage_taken_sound = 1190;
		int damage_given_sound = 17802;
		bool custom_fall_damage = false;
		float fall_death_velocity = -0.6f;
		bool cbug_global = true;
		bool cbug_death_delay = true;
		int max_shoot_rate_samples = 4;
		int max_hit_rate_samples = 4;
		int respawn_time = 3000;
		bool vehicle_passenger_damage = false;
		bool vehicle_unoccupied_damage = false;
		bool knife_sync = true;
		bool disable_sync_bugs = true;
		bool custom_vending_machines = true;

		bool damage_feed = true;
		int feed_hide_delay = 3000;
		int feed_max_update_rate = 250;
		int feed_given_color = 0x30FF50FF;
		int feed_taken_color = 0x33CCFFFF;
		float feed_given_pos[2] = {200.0f, 365.0f};
		float feed_taken_pos[2] = {440.0f, 365.0f};

		float hb_pos[2] = {546.0f, 66.7f};
		float hb_size[2] = {61.7f, 8.4f};
		float hb_padding[4] = {2.1f, 1.9f, 1.6f, 2.0f};
		int hb_border_color = 0x000000FF;
		int hb_bg_color = 0x5A0C0EFF;
		int hb_fg_color = 0xB4191DFF;

		// weapon-forge extras
		bool friendly_fire = false;
		int spawn_protection = 0;
		int combat_timeout = 10000;
		float regen_rate = 0.0f;
		int regen_delay = 5000;
		float assist_min_damage = 10.0f;
		int assist_window = 15000;
		bool forward_damage_callbacks = false;
		int debug = 0;
	};

	extern Config g_config;
	extern Player g_players[kMaxPlayers];
	extern Vehicle g_vehicles[kMaxVehicles + 1];
	extern bool g_open_mp;
	extern bool g_raknet;

	inline bool ValidPlayer(int playerid)
	{
		return playerid >= 0 && playerid < kMaxPlayers;
	}

	inline bool ValidVehicle(int vehicleid)
	{
		return vehicleid > 0 && vehicleid <= kMaxVehicles;
	}

	uint32_t Tick();
	// Signed difference (a - b) robust to tick wrap-around
	inline int32_t TickDiff(uint32_t a, uint32_t b)
	{
		return static_cast<int32_t>(a - b);
	}

	void Log(const char *fmt, ...);
	void Debug(const char *fmt, ...);
}
