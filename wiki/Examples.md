# Examples

All examples assume `#include <weapon-forge>` after `a_samp` / `open.mp`.

## Injured system

Players go down instead of dying and can be revived by others.

```pawn
new bool:gInjured[MAX_PLAYERS];

public OnPlayerPrepareDeath(playerid, animlib[32], animname[32], &anim_lock, &respawn_time)
{
    respawn_time = 2000; // lying animation for 2 s, then OnPlayerDeathFinished
    return 1;
}

public OnPlayerDeathFinished(playerid, bool:cancelable)
{
    if (cancelable && !gInjured[playerid]) {
        gInjured[playerid] = true;
        SendClientMessage(playerid, -1, "You are injured. Wait for a medic or /respawn.");
        return 0; // stay down
    }
    gInjured[playerid] = false;
    return 1;
}

// A medic revives the player
Revive(targetid)
{
    if (gInjured[targetid]) {
        gInjured[targetid] = false;
        WF_RevivePlayer(targetid, 25.0);
    }
}

public OnPlayerDamage(&playerid, &Float:amount, &issuerid, &WEAPON:weapon, &bodypart)
{
    return !gInjured[playerid]; // injured players take no more damage
}
```

## Team deathmatch with friendly fire off and headshots

```pawn
public OnGameModeInit()
{
    WF_SetFriendlyFire(false);                    // default, shown for clarity
    WF_SetWeaponBodypartMultiplier(-1, 9, 2.0);   // headshots deal double
    SetCbugAllowed(false);
    WF_SetSpawnProtection(3000);
    return 1;
}

public OnPlayerSpawn(playerid)
{
    SetPlayerTeam(playerid, gPlayerTeam[playerid]); // used by the friendly fire rules
    return 1;
}

public OnPlayerHeadshot(playerid, issuerid, WEAPON:weaponid, Float:amount)
{
    GameTextForPlayer(issuerid, "~r~HEADSHOT", 1000, 3);
    return 1;
}
```

## Kill rewards with assists

```pawn
public OnPlayerDeath(playerid, killerid, WEAPON:reason)
{
    if (killerid != INVALID_PLAYER_ID) {
        GivePlayerMoney(killerid, 500);
    }
    return 1;
}

public OnPlayerKillAssist(playerid, victimid, killerid, Float:damage)
{
    GivePlayerMoney(playerid, floatround(damage) * 2); // 2$ per damage point
    return 1;
}
```

## Anti-combat-logging

```pawn
public OnPlayerDisconnect(playerid, reason)
{
    // The plugin clears the player after this callback, so the state is still there
    if (reason == 1 && WF_IsPlayerInCombat(playerid)) {
        new attacker = WF_GetLastAttacker(playerid);
        printf("%d quit during combat (last attacker %d)", playerid, attacker);
        // punish, give the kill to the attacker, ...
    }
    return 1;
}
```

## Arena with custom damage per player

```pawn
JoinArena(playerid, bool:vip)
{
    WF_ResetPlayerStats(playerid);
    WF_SetPlayerDamageMultiplier(playerid, 1.0, vip ? 0.85 : 1.0); // VIPs take 15% less
    WF_SetPlayerHealthRegen(playerid, 0.0);                          // no regen in the arena
}

LeaveArena(playerid)
{
    new msg[96];
    format(msg, sizeof(msg), "Accuracy: %.1f%%  Kills: %d  Headshots: %d",
        WF_GetPlayerAccuracy(playerid),
        WF_GetPlayerStat(playerid, WF_STAT_KILLS),
        WF_GetPlayerStat(playerid, WF_STAT_HEADSHOTS_GIVEN));
    SendClientMessage(playerid, -1, msg);

    WF_SetPlayerDamageMultiplier(playerid);    // back to 1.0 / 1.0
    WF_SetPlayerHealthRegen(playerid);         // back to the global regen
}
```

## Bleeding from knife wounds

```pawn
public OnPlayerDamageDone(playerid, Float:amount, issuerid, WEAPON:weapon, bodypart)
{
    if (weapon == WEAPON_KNIFE && !WF_IsPlayerBleeding(playerid)) {
        WF_SetPlayerBleeding(playerid, 1.5, 6000, issuerid, WEAPON_KNIFE);
        SendClientMessage(playerid, 0xFF0000FF, "You are bleeding!");
    }
    return 1;
}
```

## Logging suspicious players

```pawn
public OnRejectedHit(playerid, hit[E_REJECTED_HIT])
{
    if (WF_GetPlayerStat(playerid, WF_STAT_REJECTED_HITS) % 20 == 0) {
        new text[144];
        GetRejectedHit(playerid, 0, text);
        printf("[anti-cheat] player %d has %d rejected hits, last: %s",
            playerid, WF_GetPlayerStat(playerid, WF_STAT_REJECTED_HITS), text);
    }
    return 1;
}
```

## Hiding the health bar for a custom HUD

```pawn
public OnPlayerConnect(playerid)
{
    WF_EnableHealthBarForPlayer(playerid, false); // the gamemode draws its own
    return 1;
}

UpdateMyHud(playerid)
{
    new Float:health, Float:armour;
    GetPlayerHealth(playerid, health);   // server-side values
    GetPlayerArmour(playerid, armour);
    // ...
}
```
