# How It Works

## One binary, two servers

The binary exports two sets of entry points:

- **SA-MP plugin interface:** `Supports`, `Load`, `Unload`, `AmxLoad`, `AmxUnload`, `ProcessTick`. SA-MP loads it from `plugins/`.
- **open.mp component interface:** `ComponentEntryPoint`. open.mp loads it from `components/` and gives it access to the Pawn component (the same AMX functions as SA-MP) and to the network events.

Both paths run the same code. The only server-specific parts are how the plugin starts and how it reads network packets.

## Hooking the AMX

SA-MP and open.mp run scripts with the Pawn abstract machine, and both expose its functions (`amx_Exec`, `amx_FindPublic`, `amx_Register`, ...) to plugins. These are the very functions the server uses internally. weapon-forge installs x86 detours on three of them.

### `amx_Register`: natives

When the server registers its natives in a script, the plugin:

1. Stores the address of every server native it needs, so it can call them directly (no script has to import them).
2. Replaces these natives with server-side versions, in **every** script:

   `SetPlayerHealth`, `GetPlayerHealth`, `SetPlayerArmour`, `GetPlayerArmour`, `GetPlayerState`, `SpawnPlayer`, `SetPlayerTeam`, `GetPlayerTeam`, `SendDeathMessage`, `GetWeaponName`, `ApplyAnimation`, `ClearAnimations`, `TogglePlayerSpectating`, `TogglePlayerControllable`, `SetPlayerPos`, `SetPlayerPosFindZ`, `SetPlayerVelocity`, `SetPlayerVirtualWorld`, `GetPlayerVirtualWorld`, `PlayerSpectatePlayer`, `DestroyVehicle`, `IsPlayerInCheckpoint`, `IsPlayerInRaceCheckpoint`, `SetPlayerSpecialAction`.

### `amx_Exec`: callbacks

The server raises a callback by calling it in every script that implements it: filterscripts first, the gamemode last. The plugin intercepts these calls and processes each event **once, before the first script runs**. Then it applies the same decision to every script:

| Callback | What the plugin does |
|---|---|
| `OnPlayerGiveDamage`, `OnPlayerTakeDamage` | Validates and applies the damage. Not passed to the scripts, unless `WF_SetForwardDamageCallbacks(true)` |
| `OnPlayerWeaponShot` | Validates the shot (range, rate, origin, ...). Invalid shots are blocked for every script |
| `OnPlayerUpdate` | Fall damage, sync data. Not passed to the scripts while the player is dying |
| `OnPlayerDeath` | Real client deaths: fixes the killer and reason (vehicle explosions, unstreamed killers) for every script |
| `OnPlayerSpawn`, `OnPlayerRequestClass`, `OnPlayerConnect`, `OnPlayerDisconnect`, `OnPlayerStateChange`, `OnPlayerEnter/ExitVehicle`, `OnPlayerKeyStateChange`, `OnVehicleSpawn`, `OnVehicleDeath` | Updates the player/vehicle state (c-bug detection, vending machines, health bar, ...) |
| Checkpoint and pickup callbacks | Blocked while the player is dead or dying |

`OnPlayerDisconnect` runs in the scripts *before* the plugin clears the player, so saving the player's health there works.

### `amx_FindPublic`: events no script implements

The server only raises a callback if a script implements it. When no script has, say, `OnPlayerGiveDamage`, the plugin reports it as present in the gamemode. The server then raises the event, and the plugin processes it without running any script code.

## Server-side health

weapon-forge reproduces weapon-config's model:

- The client's real health is kept at `8000000 + health`, so the client never dies on its own. Every death is decided by the server.
- The plugin stores the real health and armour. `GetPlayerHealth` returns that value in every script.
- The health bar on the HUD is a textdraw drawn by the plugin.
- When health reaches 0, the plugin plays a death animation chosen from the weapon and situation. It then calls `OnPlayerDeath` in every script and, after `SetRespawnTime` milliseconds, calls `OnPlayerDeathFinished` and respawns the player.

## Sync packets

Other players see a player's health in the nametag from the sync packets that player sends. Those packets carry the real client health (8000000+), so the plugin rewrites them with the server-side value before the server reads them.

- **SA-MP:** the plugin gets the server's `RakServer` (through the plugin data the server hands to plugins) and wraps its `Receive` method.
- **open.mp:** the component registers a network event handler.

For each on-foot, vehicle, passenger and aim packet, the plugin:

- writes the server-side health and armour (nametags show the right values);
- freezes the player's sync while dying or during the c-bug punishment;
- removes "ghost shooting" keys while running, the detonator crash, firing with night/thermal goggles and the knife instant kill when knife sync is off;
- fixes the first-person aim of sniper, rocket launcher and camera.

All of this is a few byte writes in C++, with no Pawn code per packet.

## Timers and per-tick work

There is no `SetTimer` involved. The plugin keeps its own timer queue (death respawn, delayed death, c-bug punishment, knife animation, vending machines, damage feed) and runs it on every server tick. Every 100 ms it also updates combat state, regeneration and bleeding.

## Script reloads

On `gmx` (`OnGameModeExit`), the plugin restores the players' real health, team and world. It also resets every setting to the defaults, so the next gamemode starts clean, as with weapon-config.
