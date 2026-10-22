# Callbacks

The plugin calls these publics in every script that implements them: filterscripts first, the gamemode last. Declare them with the signatures below; the include has the matching `forward`s.

Unless stated otherwise, the return value is ignored.

## OnPlayerDamage

```pawn
public OnPlayerDamage(&playerid, &Float:amount, &issuerid, &WEAPON:weapon, &bodypart)
```

Called before damage is applied, after it was validated.

| Parameter | Description |
|---|---|
| `playerid` | Player taking the damage |
| `amount` | Damage that will be applied. `0.0` means "kill" (e.g. `SetPlayerHealth(playerid, 0.0)`) |
| `issuerid` | Player causing the damage, or `INVALID_PLAYER_ID` |
| `weapon` | Weapon id, including the custom ids (`WEAPON_PISTOLWHIP`, `WEAPON_CARPARK`, `WEAPON_UNKNOWN`, ...) |
| `bodypart` | 3 torso, 4 groin, 5 left arm, 6 right arm, 7 left leg, 8 right leg, 9 head |

**Return** `0` to cancel the damage. Every argument is passed by reference and can be changed. The next script receives the changed values.

```pawn
public OnPlayerDamage(&playerid, &Float:amount, &issuerid, &WEAPON:weapon, &bodypart)
{
    if (GetPlayerTeam(playerid) == GetPlayerTeam(issuerid)) {
        return 0; // no team damage
    }
    if (bodypart == 9) {
        amount *= 2.0;
    }
    return 1;
}
```

## OnPlayerDamageDone

```pawn
public OnPlayerDamageDone(playerid, Float:amount, issuerid, WEAPON:weapon, bodypart)
```

Called after damage was applied. `amount` is the health + armour actually removed (capped at what the player had). Use `GetLastDamageHealth` / `GetLastDamageArmour` for the split.

## OnPlayerPrepareDeath

```pawn
public OnPlayerPrepareDeath(playerid, animlib[32], animname[32], &anim_lock, &respawn_time)
```

Called when the player's health reaches 0, before the death animation.

| Parameter | Description |
|---|---|
| `animlib`, `animname` | Animation chosen by the plugin (e.g. `"PED"`, `"KO_shot_front"`). Change it, or clear it (`animlib[0] = 0;`) to play none |
| `anim_lock` | `1` locks the player in place during the animation |
| `respawn_time` | Milliseconds until `OnPlayerDeathFinished`. `-1` (default) uses `SetRespawnTime` |

## OnPlayerDeathFinished

```pawn
public OnPlayerDeathFinished(playerid, bool:cancelable)
```

Called when the death animation ends, just before the player is respawned. The server-side health is already restored to the maximum; the weapons are reset after the callback (unless you cancel).

**Return** `0` with `cancelable == true` to keep the player alive where they are instead of respawning, for example in an "injured" system. `cancelable` is `false` when the client really died or went to class selection.

## OnPlayerDeath

The regular callback. The plugin calls it in every script when a player dies server-side, with the correct killer and weapon. For real client deaths (e.g. a vehicle exploding with the player inside), the plugin fixes the `killerid`/`reason` the server reports before the scripts see them.

## OnRejectedHit

```pawn
public OnRejectedHit(playerid, hit[E_REJECTED_HIT])
```

Called when a shot or damage from `playerid` is rejected by the validation. `hit[e_Reason]` is one of the [`HIT_*` reasons](Constants#rejected-hit-reasons). `GetRejectedHit(playerid, 0, text)` gives a readable description of the last one.

| Field | Description |
|---|---|
| `e_Time`, `e_Hour`, `e_Minute`, `e_Second` | When it happened |
| `e_Weapon` | Weapon used |
| `e_Reason` | `HIT_*` reason |
| `e_Info1..3` | Reason-specific values (distance, rate, ...) |
| `e_Name` | Name of the target player (`#` if none) |

## OnInvalidWeaponDamage

```pawn
public OnInvalidWeaponDamage(playerid, damagedid, Float:amount, WEAPON:weaponid, bodypart, error, bool:given)
```

Called when the damage itself is invalid. `error` is one of `WC_NO_ISSUER`, `WC_NO_DAMAGED`, `WC_INVALID_DAMAGE` and `WC_INVALID_DISTANCE`. `given` tells whether it came from `OnPlayerGiveDamage` (`true`) or `OnPlayerTakeDamage` (`false`).

## OnPlayerUseVendingMachine

```pawn
public OnPlayerUseVendingMachine(playerid, &Float:health_given)
```

Called when a player uses a vending machine (with `SetCustomVendingMachines(true)`, the default). Change `health_given` (35.0 by default). **Return** `0` to prevent it.

## OnPlayerHeadshot

```pawn
public OnPlayerHeadshot(playerid, issuerid, WEAPON:weaponid, Float:amount)
```

Called after a bullet hit the head (body part 9) and the damage was applied.

## OnPlayerKillAssist

```pawn
public OnPlayerKillAssist(playerid, victimid, killerid, Float:damage)
```

Called for each player (`playerid`) that dealt at least the minimum damage to `victimid` within the assist window before `killerid` killed them. See `WF_SetAssistParams` (default: 10 damage within 15 seconds).

## OnPlayerEnterCombat / OnPlayerLeaveCombat

```pawn
public OnPlayerEnterCombat(playerid)
public OnPlayerLeaveCombat(playerid)
```

A player enters combat when they give or take damage, and leaves it after the combat timeout without damage (`WF_SetCombatTimeout`, default 10 seconds).

## OnPlayerCbug

```pawn
public OnPlayerCbug(playerid, WEAPON:weaponid, interval)
```

Called when a c-bug is detected while c-bug is disallowed (`SetCbugAllowed(false)`). `interval` is the time in ms between the shot and the crouch.

**Return** `0` to skip the punishment. By default the player is frozen briefly, and the damage they dealt in the last 1.2 s is undone.

## OnPlayerGiveDamage / OnPlayerTakeDamage

The server callbacks are consumed by the plugin and **not** called in the scripts, as in weapon-config. Use `OnPlayerDamage` / `OnPlayerDamageDone` instead, or enable them with `WF_SetForwardDamageCallbacks(true)`.
