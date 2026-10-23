# weapon-forge

**English** | [Português](README.pt-BR.md)

Server-side **health, armour, damage, death and weapon configuration** for **SA-MP 0.3.7 / 0.3.DL** and **open.mp**, implemented entirely as a native plugin. A drop-in replacement for [weapon-config](https://github.com/oscar-broman/samp-weapon-config) without its performance cost.

- **Everything runs in the plugin.** It hooks the server callbacks and natives by itself. The include only *declares* natives, constants and callbacks: no hooks, no publics, no macros, no code added to your scripts.
- **One binary per OS for both servers.** The same `weapon-forge.dll` / `weapon-forge.so` is a SA-MP plugin *and* an open.mp component.
- **No dependencies.** No Pawn.RakNet or any other plugin needed: sync packets are accessed natively on both servers.
- **weapon-config compatible API.** Same functions, constants and callbacks (`OnPlayerDamage`, `OnPlayerDamageDone`, `OnPlayerPrepareDeath`, `OnPlayerDeathFinished`, `OnRejectedHit`, `SetWeaponDamage`, `SetCbugAllowed`, `DamagePlayer`...).
- **New features.** God mode, spawn protection, per-player and per-body-part damage multipliers, friendly fire toggle, combat state, hit history, kill assists, bleeding, health regeneration, statistics (accuracy, headshots, kills...), `WF_KillPlayer` / `WF_RevivePlayer` and more.

📖 **Full documentation: see the [wiki](../../wiki)** (sources in [`wiki/`](wiki)).

## Why it is faster

weapon-config does everything in Pawn. That includes processing **every sync packet** (on-foot, vehicle, passenger, aim) through Pawn.RakNet, with dozens of `BS_*` calls per packet per player, plus ALS hooks, `format` calls and dozens of `SetTimerEx` timers.

| | weapon-config | weapon-forge |
|---|---|---|
| Sync packets | Read and rewritten in Pawn | Bytes patched in C++ before the server reads the packet. **No Pawn code per packet** |
| Callbacks (damage, shots, updates...) | ALS hooks in Pawn, in every script | Processed in the plugin before any script |
| Damage/shot validation, damage feed, health bar | Pawn | C++ |
| Timers (death, c-bug, feed, knife...) | `SetTimerEx` | Native timer queue |
| State (health, armour, death) | Per-script arrays, lost on `gmx` | In the plugin, shared by the gamemode and filterscripts |

## Compatibility

| Server | Windows | Linux | How it is loaded |
|---|---|---|---|
| SA-MP 0.3.7 / 0.3.DL | `plugins/weapon-forge.dll` | `plugins/weapon-forge.so` | `plugins weapon-forge` in `server.cfg` |
| open.mp | `components/weapon-forge.dll` | `components/weapon-forge.so` | automatically (`components/` folder) |

The files in `plugins/` and `components/` are **the same binary**; releases ship both folders. On open.mp the plugin also works as a *legacy plugin* (`legacy_plugins`), but open.mp gives legacy plugins no network access. In that mode the nametag health and the sync fixes are disabled, and the plugin says so in the log.

Tested on real servers: SA-MP 0.3.7-R2 and open.mp 1.6.7. The Linux build targets glibc 2.17+ and links the C++ runtime statically.

> A single file can't run on both Windows **and** Linux; they use different executable formats. What you get is one binary per OS, identical for SA-MP and open.mp.

## Installation

1. Download a release (`weapon-forge-X.Y.Z-windows.zip` or `-linux.tar.gz`), or [build it](#building).
2. **SA-MP:** copy `plugins/weapon-forge.dll` (or `.so`) to `plugins/` and add `weapon-forge` to the `plugins` line of `server.cfg`.
   **open.mp:** copy `components/weapon-forge.dll` (or `.so`) to `components/`. Nothing else to configure.
3. To use the weapon-forge functions and callbacks, copy `include/weapon-forge.inc` to your include folder:
   ```pawn
   #include <open.mp>      // or <a_samp>
   #include <weapon-forge>
   ```

The include is optional. Without it, the plugin still works the same in **every** script: server-side health, deaths, damage validation, damage feed and health bar. You only need it to call `WF_*` natives or implement the callbacks (`OnPlayerDamage`, ...).

### Migrating from weapon-config

1. Replace `#include <weapon-config>` with `#include <weapon-forge>`.
2. Remove Pawn.RakNet/SKY if weapon-config was their only user.
3. Recompile the gamemode and the filterscripts.

See [Migrating from weapon-config](../../wiki/Migrating-from-weapon-config) for the few differences.

## Quick example

```pawn
#include <open.mp>
#include <weapon-forge>

public OnGameModeInit()
{
    SetWeaponDamage(WEAPON_DEAGLE, DAMAGE_TYPE_STATIC, 50.0);
    WF_SetWeaponBodypartMultiplier(-1, 9, 1.5);   // +50% damage on headshots
    SetCbugAllowed(false);
    SetRespawnTime(4000);
    WF_SetSpawnProtection(3000);
    WF_SetHealthRegen(1.0, 10000);                // 1 HP/s, 10 s after the last hit
    return 1;
}

public OnPlayerDamage(&playerid, &Float:amount, &issuerid, &WEAPON:weapon, &bodypart)
{
    if (weapon == WEAPON_SNIPER && bodypart == 9) {
        amount = 1000.0; // sniper headshots kill
    }
    return 1;
}

public OnPlayerDeathFinished(playerid, bool:cancelable)
{
    // "Injured" system: keep the player down instead of respawning
    if (cancelable && IsPlayerInHospitalZone(playerid)) {
        return 0;
    }
    return 1;
}
```

## Building

`build.ps1` builds **both** platforms from Windows: the Windows DLL with Visual Studio, and the Linux `.so` cross-compiled with [Zig](https://ziglang.org). The script downloads Zig automatically, so there is no need for WSL, Docker or a Linux machine.

Requirements: Visual Studio 2019/2022 (or Build Tools) with "Desktop development with C++", CMake (the one bundled with Visual Studio works) and git (to download the open.mp SDK once).

```powershell
.\build.ps1                          # Windows + Linux -> dist\
.\build.ps1 -Platform Linux          # only weapon-forge.so
.\build.ps1 -Platform Windows
.\build.ps1 -Clean -Package          # clean build + .zip/.tar.gz in dist\
.\build.ps1 -Version 1.2.0 -Package
```

The output goes to `dist/plugins/` (SA-MP) and `dist/components/` (open.mp), plus `dist/include/weapon-forge.inc`. If PowerShell blocks the script, run `powershell -ExecutionPolicy Bypass -File .\build.ps1`.

`build.ps1 -Platform Linux` also works on Linux with PowerShell (`pwsh`). Alternatively, `build.sh` builds with the system's g++ and CMake (`sudo apt install cmake git g++-multilib`). See [Building](../../wiki/Building) for details.

## CI and releases

`.github/workflows/build.yml` runs **only** on pull requests and when started manually:

- builds Windows and Linux with `build.ps1` and checks the binaries (exports, ELF format, glibc requirement);
- compiles the test scripts against the SA-MP and open.mp includes (any warning fails the build);
- uploads the binaries as artifacts (the download contains `plugins/`, `components/` and `include/` directly).

To publish a release, go to **Actions → Build → Run workflow**, tick **"Create a GitHub release with the binaries"** and enter the version (empty = the `VERSION` file). The workflow creates the `vX.Y.Z` tag with `weapon-forge-X.Y.Z-windows.zip`, `weapon-forge-X.Y.Z-linux.tar.gz` and `weapon-forge.inc`.

The wiki pages live in [`wiki/`](wiki) and are published to the GitHub wiki by **Actions → Wiki → Run workflow** (see [`wiki/README.md`](wiki/README.md)).

## Credits and licenses

- Based on [weapon-config](https://github.com/oscar-broman/samp-weapon-config) by Oscar Broman and contributors (Apache License 2.0). The validation rules, default weapon values and vending machine data come from it.
- The SA-MP RakServer method indices and RakNet `Packet` layout follow [Pawn.RakNet](https://github.com/katursis/Pawn.RakNet) (MIT).
- The [open.mp SDK](https://github.com/openmultiplayer/open.mp-sdk) (MPL-2.0) is downloaded at build time.

See `NOTICE` and `third_party/`.
