# Constants

All declared in `weapon-forge.inc`, with the same values as weapon-config.

## Damage types

Used by [`SetWeaponDamage`](Natives-Weapons-and-Damage#wf_setweapondamage).

| Constant | Value |
|---|---|
| `DAMAGE_TYPE_MULTIPLIER` | 0 |
| `DAMAGE_TYPE_STATIC` | 1 |
| `DAMAGE_TYPE_RANGE_MULTIPLIER` | 2 |
| `DAMAGE_TYPE_RANGE` | 3 |

## Invalid damage errors

The `error` parameter of [`OnInvalidWeaponDamage`](Callbacks#oninvalidweapondamage).

| Constant | Meaning |
|---|---|
| `WC_NO_ERROR` | - |
| `WC_NO_ISSUER` | Bullet/melee damage without a valid attacker |
| `WC_NO_DAMAGED` | Damage given to a player that isn't connected |
| `WC_INVALID_DAMAGE` | The damage amount doesn't match the weapon (modified client) |
| `WC_INVALID_DISTANCE` | Melee/vehicle damage from too far away |

## Rejected hit reasons

`hit[e_Reason]` in [`OnRejectedHit`](Callbacks#onrejectedhit). The `e_Info*` fields carry the values shown in parentheses (floats are stored as float bits).

| Constant | Meaning |
|---|---|
| `HIT_NO_DAMAGEDID` | Hit an invalid/disconnected player |
| `HIT_INVALID_WEAPON` | Weapon that can't give damage this way |
| `HIT_LAST_SHOT_INVALID` | No valid shot before the hit |
| `HIT_MULTIPLE_PLAYERS` | One bullet hit several players (count) |
| `HIT_MULTIPLE_PLAYERS_SHOTGUN` | One shotgun shot hit too many players (count) |
| `HIT_DYING_PLAYER` | Hit a player that is dying |
| `HIT_SAME_TEAM` | Hit a teammate (see `WF_SetFriendlyFire`) |
| `HIT_UNSTREAMED` | The players can't see each other (player/vehicle id) |
| `HIT_INVALID_HITTYPE` | Invalid bullet hit type (type) |
| `HIT_BEING_RESYNCED` | Kept for compatibility |
| `HIT_NOT_SPAWNED` | Shooter is dead or not spawned |
| `HIT_OUT_OF_RANGE` | Beyond the weapon's range (distance, range) |
| `HIT_TOO_FAR_FROM_SHOT` | Victim too far from the bullet's hit position (distance) |
| `SHOOTING_RATE_TOO_FAST` | Firing faster than the weapon allows (average, samples, max) |
| `SHOOTING_RATE_TOO_FAST_MULTIPLE` | Same, switching weapons (average, samples) |
| `HIT_RATE_TOO_FAST` | Hitting faster than the weapon allows (average, samples, max) |
| `HIT_RATE_TOO_FAST_MULTIPLE` | Same, switching weapons (average, samples) |
| `HIT_TOO_FAR_FROM_ORIGIN` | Shot/damage too far from the shooter's position (distance) |
| `HIT_INVALID_DAMAGE` | Damage amount doesn't match the weapon (amount) |
| `HIT_SAME_VEHICLE` | Shot a player in the same vehicle |
| `HIT_OWN_VEHICLE` | Shot the vehicle the shooter is in |
| `HIT_INVALID_VEHICLE` | Shot an invalid vehicle (vehicle id) |
| `HIT_DISCONNECTED` | Shot a disconnected player (player id) |
| `HIT_PROTECTED` | **New.** Target in god mode or spawn protection |

## E_REJECTED_HIT

```pawn
enum E_REJECTED_HIT {
           e_Time,          // gettime() timestamp
           e_Hour,
           e_Minute,
           e_Second,
    WEAPON:e_Weapon,
           e_Reason,        // HIT_*
           e_Info1,
           e_Info2,
           e_Info3,
           e_Name[WF_MAX_PLAYER_NAME] // target name, "#" if none
}
```

## Statistics

For [`WF_GetPlayerStat`](Natives-Combat-and-Statistics#wf_getplayerstat--wf_getplayerstatfloat): `WF_STAT_SHOTS_FIRED`, `WF_STAT_SHOTS_HIT`, `WF_STAT_HITS_TAKEN`, `WF_STAT_HEADSHOTS_GIVEN`, `WF_STAT_HEADSHOTS_TAKEN`, `WF_STAT_KILLS`, `WF_STAT_DEATHS`, `WF_STAT_ASSISTS`, `WF_STAT_REJECTED_HITS`.

For `WF_GetPlayerStatFloat`: `WF_STATF_DAMAGE_GIVEN`, `WF_STATF_DAMAGE_TAKEN`, `WF_STATF_HEALTH_LOST`, `WF_STATF_ARMOUR_LOST`.

## Custom weapon ids

Besides GTA's weapon ids, these appear in callbacks:

| Constant | Id | Meaning |
|---|---|---|
| `WEAPON_UNARMED` | 0 | Fist |
| `WEAPON_VEHICLE_M4` | 19 | Hunter/Predator/Rustler/Sea Sparrow gun |
| `WEAPON_VEHICLE_MINIGUN` | 20 | Hunter minigun |
| `WEAPON_VEHICLE_ROCKETLAUNCHER` | 21 | Rockets fired by vehicles (Hunter, Hydra, Rhino) |
| `WEAPON_PISTOLWHIP` | 48 | Hit with the butt of a gun |
| `WEAPON_HELIBLADES` | 50 | Helicopter blades |
| `WEAPON_EXPLOSION` | 51 | Explosion of unknown origin |
| `WEAPON_CARPARK` | 52 | Run over / pushed by a car |
| `WEAPON_UNKNOWN` | 55 | Script damage (`DamagePlayer`, `SetPlayerHealth(0)`), bleeding |

`BODY_PART_UNKNOWN` (0) is the body part of damage that doesn't hit a specific part.

## Body parts

| Id | Part |
|---|---|
| 3 | Torso |
| 4 | Groin |
| 5 | Left arm |
| 6 | Right arm |
| 7 | Left leg |
| 8 | Right leg |
| 9 | Head |
