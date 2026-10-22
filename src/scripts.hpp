#pragma once

#include "sdk/amx.h"

#include <string>
#include <vector>

// Tracks loaded scripts and calls Pawn publics in them.
namespace scripts
{
	void OnLoad(AMX *amx);
	void OnUnload(AMX *amx);

	// The gamemode is the script whose main() the server runs.
	void SetGameMode(AMX *amx);
	AMX *GameMode();
	// Any script usable as a context for native calls (gamemode first).
	AMX *AnyContext();

	// A call to a public function in every script that implements it,
	// filterscripts first and the gamemode last. References and arrays are
	// copied into each script's heap and copied back afterwards, so changes
	// made by one script are seen by the next one.
	class Call
	{
	public:
		explicit Call(const char *name) : name_(name) {}

		Call &Int(cell value);
		Call &Float(float value);
		Call &Ref(cell &value);
		Call &RefFloat(float &value);
		Call &Array(cell *data, int size, bool copy_back);
		Call &String(const char *str, int size = 0, std::string *out = nullptr);

		// Returns `fallback` if no script implements the public. With
		// `stop_on_zero`, stops and returns 0 as soon as a script returns 0;
		// otherwise returns 0 if any script returned 0.
		cell Exec(cell fallback = 1, bool stop_on_zero = false);

	private:
		struct Arg
		{
			enum class Type
			{
				Value,
				Ref,
				Array,
				String,
			} type;
			cell value;
			cell *ref;
			float *ref_float;
			cell *array;
			int size;
			bool copy_back;
			std::string str;
			std::string *str_out;
		};

		bool ExecIn(AMX *amx, cell &retval);

		const char *name_;
		std::vector<Arg> args_;
	};
}
