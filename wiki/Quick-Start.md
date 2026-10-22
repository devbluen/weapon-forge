# Quick Start

After [installing](Installation) the plugin, players already have server-side health and armour, weapon damage validation, death animations, a damage feed and a health bar. No code is needed for that.

This page shows the most common customisations.

## Configure weapons

```pawn
#include <open.mp>
#include <weapon-forge>

public OnGameModeInit()
{
    // Fixed damage per bullet
    SetWeaponDamage(WEAPON_DEAGLE, DAMAGE_TYPE_STATIC, 50.0);

    // Damage by distance: 41 up to 50 m, 30 from 50 m, 20 from 100 m
    SetWeaponDamage(WEAPON_SNIPER, DAMAGE_TYPE_RANGE, 41.0, 50.0, 30.0, 100.0, 20.0);

    // Headshots deal 50% more damage with every weapon
    WF_SetWeaponBodypartMultiplier(-1, 9, 1.5);

    // Armour only protects the torso from bullets
    SetCustomArmourRules(true, true);
    return 1;
}
```

## Deaths and respawn

```pawn
public OnGameModeInit()
{
    SetRespawnTime(4000);          // death animation lasts 4 seconds
    WF_SetSpawnProtection(3000);   // 3 seconds of protection after each spawn
    return 1;
}

public OnPlayerDeath(playerid, killerid, WEAPON:reason)
{
    // Called by the plugin when the player dies server-side
    SendDeathMessage(killerid, playerid, reason);
    return 1;
}
```

## React to damage

```pawn
public OnPlayerDamage(&playerid, &Float:amount, &issuerid, &WEAPON:weapon, &bodypart)
{
    // No damage inside the safe zone
    if (IsPlayerInSafeZone(playerid)) {
        return 0;
    }

    // Sniper headshots kill
    if (weapon == WEAPON_SNIPER && bodypart == 9) {
        amount = 1000.0;
    }
    return 1;
}

public OnPlayerDamageDone(playerid, Float:amount, issuerid, WEAPON:weapon, bodypart)
{
    // Damage was applied: update a HUD, log, give money...
    return 1;
}
```

## Anti-cheat

```pawn
public OnGameModeInit()
{
    SetCbugAllowed(false);   // block c-bug
    return 1;
}

public OnRejectedHit(playerid, hit[E_REJECTED_HIT])
{
    new text[144];
    GetRejectedHit(playerid, 0, text);
    printf("[anti-cheat] %d: %s", playerid, text);
    return 1;
}
```

## Health is server-side

Use the regular natives; they are replaced by the plugin in every script:

```pawn
SetPlayerHealth(playerid, 100.0);
SetPlayerArmour(playerid, 50.0);

new Float:health;
GetPlayerHealth(playerid, health);   // the server-side value

SetPlayerHealth(playerid, 0.0);      // kills the player, with death animation
```

Next steps: [Callbacks](Callbacks), [Natives](Natives), [Examples](Examples).
