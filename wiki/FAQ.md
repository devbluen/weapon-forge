# FAQ

### Do I need to include weapon-forge in every script?

No. The plugin works in every script without the include. Add the include only to scripts that call `WF_*` natives or implement weapon-forge callbacks such as `OnPlayerDamage`.

### Do I still need Pawn.RakNet?

No. weapon-forge reads the sync packets by itself on SA-MP and, as a component, on open.mp. You can keep Pawn.RakNet installed if other code of yours uses it.

### Can I use it as a legacy plugin on open.mp?

It works, but open.mp gives legacy plugins no network access. The nametag health and the sync fixes are then disabled. Put it in `components/` instead.

### Why is one file for Windows and Linux impossible?

Windows uses DLLs (PE format) and Linux uses shared objects (ELF format), so no single file can be both. weapon-forge has one binary per OS, and each one works on both SA-MP and open.mp.

### `GetPlayerHealth` returns 30000 in another plugin / external tool

Scripts get the server-side health because the plugin replaces the native. Other plugins that read the server's internal health directly see what the server stores. With sync packet processing that is the value the client reports, otherwise the 30000+ value. `WF_GetPlayerRawHealth` shows it.

### Players show full health in the nametag

The sync packets aren't being processed. Check `WF_IsRakNetActive()` and the log for `Network hook installed` (SA-MP) or `loaded (open.mp component)`. On open.mp, the plugin must be in `components/`.

### The health bar overlaps my HUD

Move it with `SetHealthBarPosition` / `WF_SetHealthBarPosForPlayer`, or hide it with `WF_EnableHealthBarForPlayer(playerid, false)` and draw your own from `GetPlayerHealth`.

### My OnPlayerGiveDamage / OnPlayerTakeDamage is never called

As in weapon-config, the plugin consumes them. Use `OnPlayerDamage` / `OnPlayerDamageDone`, or call `WF_SetForwardDamageCallbacks(true)`.

### My settings are lost after gmx

On `gmx` every setting goes back to the default, so the new gamemode starts clean. Configure weapon-forge in `OnGameModeInit`.

### NPCs don't respawn on open.mp

NPCs created with `NPC_Create` must be respawned with `NPC_Respawn` in `OnPlayerDeathFinished`. See [Migrating from weapon-config](Migrating-from-weapon-config#openmp-npcs).

### A player died and `OnPlayerDeath` arrived 1.2 seconds later

C-bug is forbidden (`SetCbugAllowed(false)`) and `SetCbugDeathDelay` is on (default). The delay lets the plugin undo kills made with c-bug. Disable it with `SetCbugDeathDelay(false)`.

### Does it work with streamer and other sampgdk plugins?

Yes, from 1.1.4. Older versions crashed the server inside `streamer.dll` when a player connected and the gamemode lacked a callback the plugin needs (e.g. `OnPlayerRequestClass`). weapon-forge now hooks `amx_Exec`/`amx_FindPublic` when the first script loads, after every plugin, so it sits over their hooks and both keep receiving every callback.

### "CrashDetect must be loaded before 'weapon-forge.dll'" / `Function not registered: PrintBacktrace`

Fixed in 1.1.4. CrashDetect refuses to load when `amx_Exec` is already hooked, and on open.mp components always load before legacy plugins, so its natives (`PrintBacktrace`, ...) were missing and gamemodes compiled with `crashdetect.inc` (e.g. through YSI) didn't start. weapon-forge now hooks `amx_Exec` after the plugins load, in any plugin order. CrashDetect itself still has to be listed before streamer and other sampgdk plugins.

### Health and armour stay 0 / the health bar never shows (YSI y_hooks)

Fixed in 1.1.3 (callbacks) and 1.1.4 (publics the plugin calls itself). YSI `y_hooks` rewrites the script's public table at runtime and older versions then stopped recognising `OnPlayerConnect`, so the player was never tracked. From 1.1.4, `WF_SetDebug(1)` logs `healthbar <id>: ...` lines for every player the plugin tracks.

### My script destroys or hides the health bar

As in weapon-config, scripts can't modify the plugin's own player textdraws (health bar, damage feed): `PlayerTextDraw*` calls on them are ignored. A gamemode keeping a stale textdraw id would otherwise destroy the health bar once the id got reused. `WF_SetDebug(1)` logs each blocked call.

### The server crashes or the plugin says "could not hook the AMX functions"

The plugin couldn't patch the AMX functions. This can happen with other plugins that hook the same functions in an incompatible way. Please open an issue with the server log and the list of plugins.

### How do I debug damage?

`WF_SetDebug(1)` prints damage, shots and rejected hits to the console. `GetRejectedHit(playerid, idx, text)` describes the last rejected hits.
