# Natives: Weapons and Damage

See [Default Weapon Values](Default-Weapon-Values) for the defaults of every weapon.

## WF_SetWeaponDamage
*Alias: `SetWeaponDamage`*

```pawn
native WF_SetWeaponDamage(WEAPON:weaponid, damage_type, Float:amount, Float:...);
```

Sets how much damage a weapon deals.

| `damage_type` | Meaning |
|---|---|
| `DAMAGE_TYPE_MULTIPLIER` | The damage reported by the client is multiplied by `amount` |
| `DAMAGE_TYPE_STATIC` | Every hit deals `amount` (per pellet for shotguns) |
| `DAMAGE_TYPE_RANGE` | Static damage by distance (bullet weapons only) |
| `DAMAGE_TYPE_RANGE_MULTIPLIER` | Multiplier by distance (bullet weapons only) |

For the range types, `amount` applies from 0 m. After it come pairs of `distance, value`: each value applies from its distance on. Up to 8 ranges.

```pawn
SetWeaponDamage(WEAPON_DEAGLE, DAMAGE_TYPE_STATIC, 50.0);
SetWeaponDamage(WEAPON_FIST, DAMAGE_TYPE_MULTIPLIER, 2.0);                        // punches deal double
SetWeaponDamage(WEAPON_SNIPER, DAMAGE_TYPE_RANGE, 41.0, 50.0, 30.0, 100.0, 20.0); // 41 / 30 from 50 m / 20 from 100 m
SetWeaponDamage(WEAPON_M4, DAMAGE_TYPE_RANGE_MULTIPLIER, 1.0, 40.0, 0.75);        // 75% beyond 40 m
```

Returns 1 on success, 0 for an invalid weapon or type.

## WF_GetWeaponDamage / WF_GetWeaponDamageType
*Alias: `GetWeaponDamage`*

```pawn
native Float:WF_GetWeaponDamage(WEAPON:weaponid);
native WF_GetWeaponDamageType(WEAPON:weaponid);
```

Configured damage value and `DAMAGE_TYPE_*` (`-1` for an invalid weapon).

## WF_SetWeaponMaxRange / WF_GetWeaponMaxRange
*Aliases: `SetWeaponMaxRange`, `GetWeaponMaxRange`*

```pawn
native WF_SetWeaponMaxRange(WEAPON:weaponid, Float:range);
native Float:WF_GetWeaponMaxRange(WEAPON:weaponid);
```

Maximum distance of a bullet weapon. Shots and hits beyond it are rejected (`HIT_OUT_OF_RANGE`). Bullet weapons only.

## WF_SetWeaponShootRate / WF_GetWeaponShootRate
*Aliases: `SetWeaponShootRate`, `GetWeaponShootRate`*

```pawn
native WF_SetWeaponShootRate(WEAPON:weaponid, max_rate);
native WF_GetWeaponShootRate(WEAPON:weaponid);
```

Fastest possible interval between two shots, in milliseconds. When the average of the last shots/hits is more than 20 ms faster than this, they are rejected (`SHOOTING_RATE_TOO_FAST`, `HIT_RATE_TOO_FAST`).

## WF_SetCustomArmourRules / WF_SetWeaponArmourRule
*Aliases: `SetCustomArmourRules`, `SetWeaponArmourRule`*

```pawn
native WF_SetCustomArmourRules(bool:armour_rules, bool:torso_rules = false);
native WF_SetWeaponArmourRule(WEAPON:weaponid, bool:affects_armour, bool:torso_only = false);
```

By default armour absorbs all damage except falls, drowning, car park and `WEAPON_UNKNOWN`. With `armour_rules` enabled, each weapon's rule decides:

- `affects_armour = false`: the weapon's damage goes straight to health.
- `torso_only = true` (when `torso_rules` is enabled): armour only absorbs hits on the torso (body part 3).

By default only bullet weapons have `affects_armour = true, torso_only = true`, so `SetCustomArmourRules(true, true)` makes armour protect the torso from bullets only.

## WF_SetWeaponBodypartMultiplier / WF_GetWeaponBodypartMultiplier
**New.**

```pawn
native WF_SetWeaponBodypartMultiplier(weaponid, bodypart, Float:multiplier);
native Float:WF_GetWeaponBodypartMultiplier(WEAPON:weaponid, bodypart);
```

Multiplies the damage of a weapon on a body part (3 torso, 4 groin, 5/6 arms, 7/8 legs, 9 head). `weaponid = -1` applies to every weapon. Default 1.0.

```pawn
WF_SetWeaponBodypartMultiplier(-1, 9, 1.5);            // +50% on headshots, every weapon
WF_SetWeaponBodypartMultiplier(_:WEAPON_SNIPER, 9, 3.0);
WF_SetWeaponBodypartMultiplier(-1, 7, 0.75);           // legs take less damage
WF_SetWeaponBodypartMultiplier(-1, 8, 0.75);
```

## WF_SetWeaponName / WF_GetWeaponName
*Aliases: `SetWeaponName`, `WC_GetWeaponName` (the server's `GetWeaponName` also uses these names)*

```pawn
native WF_SetWeaponName(WEAPON:weaponid, const name[]);
native WF_GetWeaponName(WEAPON:weaponid, name[], len = sizeof(name));
```

Names shown in the damage feed and returned by `GetWeaponName`. Custom ids have names too (`"Pistol whip"`, `"Car parking"`, `"Unknown"`, ...).

## WF_IsBulletWeapon / WF_IsMeleeWeapon / WF_IsHighRateWeapon
*Aliases: `IsBulletWeapon`, `IsMeleeWeapon`, `IsHighRateWeapon`*

```pawn
native bool:WF_IsBulletWeapon(WEAPON:weaponid);   // 22-34, 38
native bool:WF_IsMeleeWeapon(WEAPON:weaponid);    // 0-15, pistol whip
native bool:WF_IsHighRateWeapon(WEAPON:weaponid); // flamethrower, spraycan, extinguisher, car park, drowning
```

## WF_DamagePlayer
*Alias: `DamagePlayer`*

```pawn
native WF_DamagePlayer(playerid, Float:amount, issuerid = INVALID_PLAYER_ID, WEAPON:weaponid = WEAPON_UNKNOWN, bodypart = BODY_PART_UNKNOWN, bool:ignore_armour = false);
```

Damages a player through the full pipeline: multipliers, `OnPlayerDamage`, armour, `OnPlayerDamageDone`, feed, statistics and death. Players in god mode or spawn protection are not damaged. Returns 0 if the player isn't connected or `amount < 0`.

```pawn
// Explosion caused by a player's bomb
DamagePlayer(targetid, 60.0, ownerid, WEAPON_EXPLOSION, 3);
```
