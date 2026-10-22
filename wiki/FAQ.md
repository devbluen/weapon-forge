# FAQ

### Do I need to include weapon-forge in every script?

No. The plugin works in every script without the include. Add the include only to scripts that call `WF_*` natives or implement weapon-forge callbacks such as `OnPlayerDamage`.

### Do I still need Pawn.RakNet?

No. weapon-forge reads the sync packets by itself on SA-MP and, as a component, on open.mp. You can keep Pawn.RakNet installed if other code of yours uses it.

### Can I use it as a legacy plugin on open.mp?

It works, but open.mp gives legacy plugins no network access. The nametag health and the sync fixes are then disabled. Put it in `components/` instead.

### Why is one file for Windows and Linux impossible?

Windows uses DLLs (PE format) and Linux uses shared objects (ELF format), so no single file can be both. weapon-forge has one binary per OS, and each one works on both SA-MP and open.mp.

### `GetPlayerHealth` returns 8000000 in another plugin / external tool

Scripts get the server-side health because the plugin replaces the native. Other plugins that read the server's internal health directly see what the server stores. With sync packet processing that is the value the client reports, otherwise the 8000000+ value. `WF_GetPlayerRawHealth` shows it.

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

### The server crashes or the plugin says "could not hook the AMX functions"

The plugin couldn't patch the AMX functions. This can happen with other plugins that hook the same functions in an incompatible way. Please open an issue with the server log and the list of plugins.

### How do I debug damage?

`WF_SetDebug(1)` prints damage, shots and rejected hits to the console. `GetRejectedHit(playerid, idx, text)` describes the last rejected hits.
