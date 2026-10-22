# Natives: Anti-Cheat and Sync

Every hit goes through weapon-config's validation: weapon, damage values, range, shot origin and hit position, fire and hit rate, streaming, teams, players in the same vehicle, and dying or unspawned players. Rejected hits raise [`OnRejectedHit`](Callbacks#onrejectedhit) and are kept in a per-player history.

## WF_SetCbugAllowed / WF_GetCbugAllowed
*Aliases: `SetCbugAllowed`, `GetCbugAllowed`*

```pawn
native WF_SetCbugAllowed(bool:enabled, playerid = INVALID_PLAYER_ID);
native bool:WF_GetCbugAllowed(playerid = INVALID_PLAYER_ID);
```

Allows or forbids c-bug (shooting and crouching to skip the reload animation), globally or for one player. Allowed by default.

When forbidden, crouching within 1.2 s of a shot with a c-bug weapon:

1. raises [`OnPlayerCbug`](Callbacks#onplayercbug), which can return 0 to skip the punishment;
2. freezes the player for 0.6 s in place;
3. undoes the damage they dealt in the last 1.2 s, including deaths that weren't reported yet (see below).

## WF_SetCbugDeathDelay
*Alias: `SetCbugDeathDelay`*

```pawn
native WF_SetCbugDeathDelay(bool:toggle);
```

With c-bug forbidden, `OnPlayerDeath` is delayed by 1.2 s so that kills made with c-bug can be undone. Enabled by default.

## WF_GetRejectedHit
*Alias: `GetRejectedHit`*

```pawn
native WF_GetRejectedHit(playerid, idx, output[], maxlength = sizeof(output));
```

Readable description of one of the last 15 rejected hits of `playerid` (`idx` 0 = most recent). Returns 0 when there is none.

```
[14:02:11] (Deagle -> Player) Hit out of range (62.1034 > 35.0000)
```

## WF_AverageShootRate / WF_AverageHitRate
*Aliases: `AverageShootRate`, `AverageHitRate`*

```pawn
native WF_AverageShootRate(playerid, shots, &multiple_weapons = 0);
native WF_AverageHitRate(playerid, hits, &multiple_weapons = 0);
```

Average interval in ms of the last `shots`/`hits` (2-10). `multiple_weapons` is set when they were made with different weapons. Returns `-1` if there aren't enough samples.

## WF_SetKnifeSync
*Alias: `SetKnifeSync`*

```pawn
native WF_SetKnifeSync(toggle);
```

Whether the knife "stealth kill" works. With lag compensation it is disabled by default (the plugin removes the aim key while holding a knife). When enabled, a knife kill plays the proper animations for both players and kills instantly.

## WF_SetDisableSyncBugs
*Alias: `SetDisableSyncBugs`*

```pawn
native WF_SetDisableSyncBugs(toggle);
```

Fixes for client sync bugs, enabled by default: shooting while running or in melee animations ("ghost shooting"), the detonator crash and grenade/spray misuse. Requires network access (always available, except when running as a legacy plugin on open.mp).

## WF_SetLagCompMode / WF_GetLagCompMode

```pawn
native WF_SetLagCompMode(mode);
native WF_GetLagCompMode();
```

Lag compensation mode used by the validation. It is read from the server when the gamemode starts (`lagcompmode` on SA-MP, `game.lag_compensation_mode` on open.mp), so it rarely needs to be changed.

## WF_SetForwardDamageCallbacks

```pawn
native WF_SetForwardDamageCallbacks(bool:toggle);
```

By default, `OnPlayerGiveDamage` and `OnPlayerTakeDamage` are consumed by the plugin and not called in the scripts, as in weapon-config. Enable this to still receive them (with the raw, unvalidated values).
