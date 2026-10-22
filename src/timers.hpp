#pragma once

#include <cstdint>

// Lightweight one-shot timers processed in ProcessTick (replaces the many
// SetTimerEx calls weapon-config makes from Pawn).
namespace timers
{
	enum Type : int
	{
		DEATH_RESPAWN,
		DELAYED_DEATH,
		CBUG_PUNISH,
		KNIFE_ANIM,
		VENDING_USE,
		FEED_UPDATE,
		REAL_DEATH_FINISHED,
		DISCONNECT_CLEANUP,
		VEHICLE_KILL,
		VEHICLE_RESPAWN,
		TYPE_COUNT
	};

	struct Args
	{
		int a = 0;
		int b = 0;
		float f = 0.0f;
	};

	// `id` is a player id or vehicle id depending on the type.
	// Starting a timer cancels the previous one of the same type and id.
	void Start(Type type, int id, int delay_ms, Args args = {});
	void Stop(Type type, int id);
	bool Active(Type type, int id);
	void StopAll(int playerid);
	void Clear();

	void Process(uint32_t now);
}
