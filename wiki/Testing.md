# Testing

## Example scripts

`tests/gamemode.pwn` and `tests/filterscript.pwn` use the include the way a real script would: compatible names, extras and every callback signature.

## Runtime test

`tests/runtime.pwn` is a gamemode that tests the plugin on a real server. It connects an NPC and checks:

- default and configured weapon values, weapon classes and names;
- server-side health: `DamagePlayer`, armour absorbing damage, god mode, damage multipliers, `SetPlayerHealth`;
- `OnPlayerDamage` / `OnPlayerDamageDone` being called;
- the fake health written into the NPC's sync packets (`WF_GetPlayerRawHealth`);
- `WF_KillPlayer`: dying state, `GetPlayerState` returning `PLAYER_STATE_WASTED`, `OnPlayerPrepareDeath`, `OnPlayerDeath` with the right weapon, no damage while dying;
- respawn: `OnPlayerDeathFinished`, restored health, statistics.

It prints `[rt] PASS ...` / `[rt] FAIL ...` lines, then `[rt] DONE fails=N`, and shuts the server down.

### SA-MP

1. Compile `tests/runtime.pwn` into `gamemodes/runtime.amx` and `tests/npc_idle.pwn` into `npcmodes/npc_idle.amx`.
2. Optionally compile `tests/filterscript.pwn` into `filterscripts/` to see filterscripts receiving `OnPlayerDeath`.
3. `server.cfg`:
   ```
   gamemode0 runtime 1
   plugins weapon-forge.dll
   maxnpc 1
   ```
4. Run the server and check the log.

### open.mp

1. Compile `tests/runtime.pwn` with the open.mp includes (`TEST_OPEN_MP=1`) into `gamemodes/runtime.amx`. It uses `NPC_Create` instead of an NPC script.
2. Put `weapon-forge.dll` in `components/`.
3. `config.json`: `"max_bots": 1` and `"main_scripts": ["runtime 1"]` in the `pawn` section.
4. Run the server and check the log.

Expected result on both servers:

```
[rt] DONE fails=0 raknet=1
```

As a legacy plugin on open.mp, the two sync packet checks fail, which is expected (no network access in that mode).
