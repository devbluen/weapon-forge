# Installation

## 1. Get the binaries

Download the latest release from the repository's **Releases** page:

| Package | Contents |
|---|---|
| `weapon-forge-X.Y.Z-windows.zip` | `plugins/weapon-forge.dll`, `components/weapon-forge.dll`, `include/weapon-forge.inc` |
| `weapon-forge-X.Y.Z-linux.tar.gz` | `plugins/weapon-forge.so`, `components/weapon-forge.so`, `include/weapon-forge.inc` |

The file in `plugins/` and the one in `components/` are the **same binary**. Both folders are shipped so you can copy whichever matches your server.

You can also [build it yourself](Building).

## 2. Load the plugin

### SA-MP 0.3.7 / 0.3.DL

1. Copy `plugins/weapon-forge.dll` (Windows) or `plugins/weapon-forge.so` (Linux) into the server's `plugins/` folder.
2. Add it to the `plugins` line in `server.cfg`:
   ```
   plugins weapon-forge.dll        # Windows
   plugins weapon-forge.so         # Linux
   ```
   It can be listed together with other plugins, in any order.

### open.mp

1. Copy `components/weapon-forge.dll` (Windows) or `components/weapon-forge.so` (Linux) into the server's `components/` folder.
2. That's it: open.mp loads every component in that folder.

> Do **not** also add it to `legacy_plugins`. As a legacy plugin it still works, but open.mp gives legacy plugins no network access. The nametag health and the sync fixes are then disabled, and the plugin warns about it in the log.

## 3. Check the server log

On startup you should see:

```
[weapon-forge] v1.0.0 loaded (SA-MP)
[weapon-forge] Network hook installed (SA-MP)
```

or, on open.mp:

```
Successfully loaded component weapon-forge (1.0.0.0) with UID 9b6f1d2a5c3e4f70
[weapon-forge] v1.0.0 loaded (open.mp component)
```

When the gamemode starts:

```
[weapon-forge] Lag compensation: on | open.mp: no | max players: 50
```

## 4. Include (optional)

The plugin works in **every** script without any include: server-side health, deaths, damage validation, damage feed and health bar.

Add the include only to the scripts that call `WF_*` natives or implement the weapon-forge [callbacks](Callbacks):

1. Copy `include/weapon-forge.inc` into your compiler's include folder (`pawno/include`, `qawno/include`, ...).
2. Include it after `a_samp` / `open.mp`:
   ```pawn
   #include <open.mp>      // or <a_samp>
   #include <weapon-forge>
   ```

The include only contains declarations, so it adds no code to the compiled script.

## Requirements

| | Windows | Linux |
|---|---|---|
| Architecture | 32-bit server (as every SA-MP / open.mp plugin) | 32-bit server, glibc 2.17 or newer |
| Runtime | No Visual C++ redistributable needed (static runtime) | No libstdc++/libc++ needed (static) |

On 64-bit Linux, 32-bit servers need the 32-bit C library (`libc6:i386` / `glibc.i686`), which is already installed on any machine that runs a SA-MP or open.mp server.
