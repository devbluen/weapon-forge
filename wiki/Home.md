# weapon-forge

Server-side **health, armour, damage, death and weapon configuration** for **SA-MP 0.3.7 / 0.3.DL** and **open.mp**, implemented entirely as a native plugin. It is a drop-in replacement for [weapon-config](https://github.com/oscar-broman/samp-weapon-config) without its performance cost.

## Highlights

- **Everything runs in the plugin.** It hooks the server callbacks and natives by itself; the include only declares natives, constants and callbacks.
- **One binary per OS for both servers.** The same `weapon-forge.dll` / `.so` is a SA-MP plugin and an open.mp component.
- **No dependencies.** Sync packets are accessed natively, without Pawn.RakNet.
- **weapon-config compatible.** Same functions, constants and callbacks.
- **Extras.** God mode, spawn protection, damage multipliers, body part multipliers, friendly fire, combat state, hit history, assists, bleeding, regeneration and statistics.

## Getting started

1. [Installation](Installation): put the binary in `plugins/` (SA-MP) or `components/` (open.mp).
2. [Quick Start](Quick-Start): configure weapons and react to damage in a few lines.
3. Coming from weapon-config? Read [Migrating from weapon-config](Migrating-from-weapon-config).

## Reference

| Page | Contents |
|---|---|
| [Callbacks](Callbacks) | `OnPlayerDamage`, `OnPlayerDeathFinished`, `OnPlayerKillAssist`, ... |
| [Natives](Natives) | Every function, grouped by category |
| [Constants](Constants) | Damage types, rejected hit reasons, statistics, custom weapon ids |
| [Default Weapon Values](Default-Weapon-Values) | Damage, range and fire rate of every weapon |
| [How It Works](How-It-Works) | Hooks, event processing, server-side health, sync packets |
| [Examples](Examples) | Injured system, headshots, kill assists, PvP arenas, ... |

## Development

| Page | Contents |
|---|---|
| [Building](Building) | `build.ps1` (Windows + Linux), `build.sh`, CMake |
| [CI and Releases](CI-and-Releases) | GitHub Actions workflow and releases |
| [Testing](Testing) | Runtime test with an NPC on a real server |
| [FAQ](FAQ) | Common questions and troubleshooting |
