# Natives: Miscellaneous

## WF_SetDebug

```pawn
native WF_SetDebug(level);
```

`1` prints damage, shots and rejected hits to the server console. `0` (default) turns it off. Useful while tuning weapons; don't leave it on in production.

## WF_IsOpenMP

```pawn
native bool:WF_IsOpenMP();
```

`true` when running on open.mp.

## WF_IsRakNetActive

```pawn
native bool:WF_IsRakNetActive();
```

`true` when the plugin processes the sync packets (nametag health, sync fixes). It is `false` only on open.mp when the plugin is loaded as a legacy plugin instead of a component, or on SA-MP before the network starts.

## WF_GetPlayerRawHealth

```pawn
native Float:WF_GetPlayerRawHealth(playerid, &Float:health = 0.0);
```

The health stored by the server itself, as opposed to the server-side health `GetPlayerHealth` returns. With sync packet processing this is the value other players see in the nametag. Without it, it's the client's 8000000+ value. Mostly useful for debugging.

## WF_GetVersion

```pawn
native WF_GetVersion(version[] = "", len = sizeof(version));
```

Copies the version string (e.g. `"1.0.0"`) into `version` and returns it as a number (`MAJOR * 10000 + MINOR * 100 + PATCH`, e.g. `10000`).

```pawn
if (WF_GetVersion() < 10100) {
    print("This gamemode needs weapon-forge 1.1.0 or newer.");
}
```
