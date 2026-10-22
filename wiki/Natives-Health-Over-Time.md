# Natives: Health Over Time

All of these are new in weapon-forge.

## WF_SetHealthRegen

```pawn
native WF_SetHealthRegen(Float:health_per_second, delay_after_damage = 5000);
```

Regenerates every player's health by `health_per_second` up to the maximum health. Regeneration starts `delay_after_damage` milliseconds after the player last gave or took damage. `0.0` (default) disables it.

```pawn
WF_SetHealthRegen(2.0, 8000); // 2 HP/s, 8 s after the last fight
```

## WF_SetPlayerHealthRegen

```pawn
native WF_SetPlayerHealthRegen(playerid, Float:health_per_second = -1.0, delay_after_damage = -1);
```

Per-player regeneration. `-1` uses the global value set with `WF_SetHealthRegen`.

## WF_SetPlayerBleeding / WF_StopPlayerBleeding / WF_IsPlayerBleeding

```pawn
native WF_SetPlayerBleeding(playerid, Float:damage_per_second, duration, issuerid = INVALID_PLAYER_ID, WEAPON:weaponid = WEAPON_UNKNOWN);
native WF_StopPlayerBleeding(playerid);
native bool:WF_IsPlayerBleeding(playerid);
```

Damages the player by `damage_per_second` every second for `duration` milliseconds. The damage ignores armour and goes through the normal pipeline (`OnPlayerDamage`, feed, statistics). A bleeding death is credited to `issuerid` with `weaponid`. Bleeding stops on death and respawn. Calling it again replaces the current bleeding; `damage_per_second <= 0` or `duration <= 0` stops it.

```pawn
public OnPlayerDamageDone(playerid, Float:amount, issuerid, WEAPON:weapon, bodypart)
{
    // Knife wounds bleed for 5 seconds
    if (weapon == WEAPON_KNIFE) {
        WF_SetPlayerBleeding(playerid, 2.0, 5000, issuerid, WEAPON_KNIFE);
    }
    return 1;
}
```
