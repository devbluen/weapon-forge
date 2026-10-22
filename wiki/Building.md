# Building

The plugin is a 32-bit binary, because SA-MP and open.mp plugins/components run in 32-bit servers. One build per OS serves both servers.

| Output | Built with |
|---|---|
| `weapon-forge.dll` (Windows) | CMake + Visual Studio (MSVC), static runtime |
| `weapon-forge.so` (Linux) | Zig (clang) cross-compiler, glibc 2.17+, static C++ runtime; or g++ with `build.sh` |

The [open.mp SDK](https://github.com/openmultiplayer/open.mp-sdk) (header-only) is pinned to a commit in `CMakeLists.txt` (`WF_OMP_SDK_COMMIT`) and downloaded once with git.

## build.ps1 (recommended)

Builds Windows and Linux from a single Windows machine. Zig and the open.mp SDK are downloaded automatically into `build/tools` and `build/deps`.

**Requirements (Windows):** Visual Studio 2019/2022 or Build Tools with the "Desktop development with C++" workload, CMake (the copy bundled with Visual Studio works), and git.

```powershell
.\build.ps1                          # Windows + Linux, Release
.\build.ps1 -Platform Windows        # only weapon-forge.dll
.\build.ps1 -Platform Linux          # only weapon-forge.so (cross-compiled)
.\build.ps1 -Config Debug
.\build.ps1 -Clean                   # rebuild from scratch (keeps downloaded tools)
.\build.ps1 -Package                 # also create the release archives
.\build.ps1 -Version 1.2.0 -Package  # override the VERSION file
```

| Parameter | Values | Default |
|---|---|---|
| `-Platform` | `All`, `Windows`, `Linux` | `All` (only `Linux` when run on Linux) |
| `-Config` | `Release`, `Debug`, `RelWithDebInfo` | `Release` |
| `-Version` | `MAJOR.MINOR.PATCH[-suffix]` | contents of `VERSION` |
| `-Generator` | any CMake Visual Studio generator | newest Visual Studio found |
| `-Clean` | switch | |
| `-Package` | switch: creates `dist/weapon-forge-<version>-windows.zip` / `-linux.tar.gz` | |

Output:

```
dist/
├── plugins/weapon-forge.dll|.so      SA-MP
├── components/weapon-forge.dll|.so   open.mp (same file)
└── include/weapon-forge.inc
```

After a Linux build the script runs `tools/elfcheck.py` (when Python is installed). It checks that the `.so` is a 32-bit x86 ELF exporting the plugin and component entry points, and prints the newest glibc version it needs.

If PowerShell refuses to run the script, use `powershell -ExecutionPolicy Bypass -File .\build.ps1`.

### On Linux

`build.ps1` also runs on Linux with PowerShell 7 (`pwsh`) to build the `.so` (this is what CI does):

```bash
pwsh ./build.ps1 -Platform Linux -Package
```

## build.sh (Linux, system compiler)

Builds the `.so` with the system's g++ and CMake instead of Zig. The result requires the glibc of the build machine.

```bash
sudo apt install cmake git g++-multilib     # Debian/Ubuntu
./build.sh
VERSION=1.2.0 ./build.sh
```

## CMake directly

```bash
# Windows (Developer prompt)
cmake -S . -B build/win32 -A Win32
cmake --build build/win32 --config Release

# Linux
cmake -S . -B build/linux -DCMAKE_BUILD_TYPE=Release
cmake --build build/linux
```

Useful cache variables:

| Variable | Purpose |
|---|---|
| `WF_VERSION` | Version string (default: `VERSION` file) |
| `WF_OMP_SDK_COMMIT` | open.mp SDK commit |
| `FETCHCONTENT_SOURCE_DIR_OMP_SDK` | Use a local copy of the SDK instead of cloning it |

## Project layout

```
include/weapon-forge.inc   declarations (natives, constants, callbacks)
src/main.cpp               SA-MP plugin entry points + shared startup
src/omp_component.cpp      open.mp component entry point
src/amxhooks.cpp           amx_Exec / amx_FindPublic / amx_Register hooks, event dispatch
src/hook.cpp               x86 inline hooks (Windows/Linux)
src/net_samp.cpp           SA-MP network access (RakServer::Receive)
src/sync.cpp               sync packet patching
src/combat.cpp             damage validation and application, shots, rejected hits
src/death.cpp              deaths, animations, OnPlayerDeath, respawn
src/events.cpp             player/vehicle events, timers, regeneration, bleeding
src/hud.cpp                health bar and damage feed (textdraws)
src/vending.cpp            vending machines
src/natives.cpp            Pawn natives and replacements of server natives
src/scripts.cpp            calling Pawn callbacks
src/server.cpp             calling server natives
src/sdk/                   minimal SA-MP plugin SDK definitions
tools/elfcheck.py          Linux binary check
tests/                     compile and runtime tests
wiki/                      this wiki
```
