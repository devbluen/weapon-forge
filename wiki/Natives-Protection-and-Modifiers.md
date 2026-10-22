# Natives: Protection and Modifiers

All of these are new in weapon-forge.

## WF_SetPlayerGodMode / WF_IsPlayerGodMode

```pawn
native WF_SetPlayerGodMode(playerid, bool:toggle);
native bool:WF_IsPlayerGodMode(playerid);
```

A player in god mode takes no damage: weapons, falls, explosions, `DamagePlayer` and bleeding. `SetPlayerHealth(playerid, 0.0)` and `WF_KillPlayer` still kill. Hits on a protected player are reported as `HIT_PROTECTED` rejected hits (except fire/spray, which would flood).

## WF_SetSpawnProtection / WF_GetSpawnProtection

```pawn
native WF_SetSpawnProtection(ms);
native WF_GetSpawnProtection();
```

Protection applied automatically after every spawn, in milliseconds. `0` (default) disables it. It works like god mode.

```pawn
WF_SetSpawnProtection(3000); // 3 seconds after each spawn
```

## WF_SetPlayerProtection / WF_GetPlayerProtectionTime

```pawn
native WF_SetPlayerProtection(playerid, ms);
native WF_GetPlayerProtectionTime(playerid);
```

Protects one player for `ms` milliseconds (`0` removes the protection). `WF_GetPlayerProtectionTime` returns the remaining time (0 when unprotected).

## WF_SetPlayerDamageMultiplier / WF_GetPlayerDamageMultiplier

```pawn
native WF_SetPlayerDamageMultiplier(playerid, Float:given = 1.0, Float:taken = 1.0);
native WF_GetPlayerDamageMultiplier(playerid, &Float:given, &Float:taken);
```

Multiplies the damage a player deals (`given`) and receives (`taken`). Both apply to a hit, together with the body part multiplier, before `OnPlayerDamage`. Reset on connect.

```pawn
// VIP takes 20% less damage
WF_SetPlayerDamageMultiplier(playerid, 1.0, 0.8);

// "Berserk" power-up: double damage for both sides
WF_SetPlayerDamageMultiplier(playerid, 2.0, 2.0);
```

## WF_SetFriendlyFire / WF_GetFriendlyFire

```pawn
native WF_SetFriendlyFire(bool:toggle);
native bool:WF_GetFriendlyFire();
```

By default, as in weapon-config, players in the same team (`SetPlayerTeam`, not `NO_TEAM`) can't damage each other (`HIT_SAME_TEAM`). `WF_SetFriendlyFire(true)` allows it.
