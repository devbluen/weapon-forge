# Natives: Health and Death

The server's `SetPlayerHealth`, `GetPlayerHealth`, `SetPlayerArmour` and `GetPlayerArmour` are replaced by the plugin and already work with the server-side values. The natives below add what those can't do.

## WF_SetPlayerHealth / WF_GetPlayerHealth

```pawn
native WF_SetPlayerHealth(playerid, Float:health, Float:armour = -1.0);
native Float:WF_GetPlayerHealth(playerid, &Float:health = 0.0);
```

`WF_SetPlayerHealth` sets health and, optionally, armour in one call (weapon-config's 3-parameter `SetPlayerHealth`). A health of 0 or less kills the player.

`WF_GetPlayerHealth` returns the health as the function result.

```pawn
WF_SetPlayerHealth(playerid, 100.0, 100.0);
if (WF_GetPlayerHealth(playerid) < 20.0) { ... }
```

## WF_SetPlayerArmour / WF_GetPlayerArmour

```pawn
native WF_SetPlayerArmour(playerid, Float:armour);
native Float:WF_GetPlayerArmour(playerid, &Float:armour = 0.0);
```

Same as the replaced natives; `WF_GetPlayerArmour` returns the value.

## WF_SetPlayerMaxHealth / WF_SetPlayerMaxArmour
*Aliases: `SetPlayerMaxHealth`, `SetPlayerMaxArmour`, `GetPlayerMaxHealth`, `GetPlayerMaxArmour`*

```pawn
native WF_SetPlayerMaxHealth(playerid, Float:value);
native WF_SetPlayerMaxArmour(playerid, Float:value);
native Float:WF_GetPlayerMaxHealth(playerid);
native Float:WF_GetPlayerMaxArmour(playerid);
```

Maximum health/armour (default 100). Can go above 100: the health bar shows the percentage. Reset on connect.

## WF_GetLastDamageHealth / WF_GetLastDamageArmour
*Aliases: `GetLastDamageHealth`, `GetLastDamageArmour`*

```pawn
native Float:WF_GetLastDamageHealth(playerid);
native Float:WF_GetLastDamageArmour(playerid);
```

How much health and armour the last damage took. Useful in `OnPlayerDamageDone`.

## WF_IsPlayerDying
*Alias: `IsPlayerDying`*

```pawn
native WF_IsPlayerDying(playerid);
```

1 while the death animation plays (between the death and the respawn).

## WF_IsPlayerSpawned / WF_IsPlayerPaused
*Aliases: `WC_IsPlayerSpawned`, `WC_IsPlayerPaused`*

```pawn
native bool:WF_IsPlayerSpawned(playerid);
native bool:WF_IsPlayerPaused(playerid);
```

`WF_IsPlayerSpawned`: spawned, alive and not in class selection. `WF_IsPlayerPaused`: no updates for more than 2 seconds.

## WF_SetRespawnTime / WF_GetRespawnTime
*Aliases: `SetRespawnTime`, `GetRespawnTime`*

```pawn
native WF_SetRespawnTime(ms);
native WF_GetRespawnTime();
```

How long the death animation lasts before `OnPlayerDeathFinished` and the respawn. Default 3000 ms. `OnPlayerPrepareDeath` can override it per death.

## WF_KillPlayer
**New.**

```pawn
native WF_KillPlayer(playerid, killerid = INVALID_PLAYER_ID, WEAPON:weaponid = WEAPON_UNKNOWN);
```

Kills a spawned player, crediting `killerid` with `weaponid` (statistics, `OnPlayerDeath`, assists). It bypasses god mode and armour, but still calls `OnPlayerDamage`, which can cancel it. Returns 1 if the player is now dying.

```pawn
// Kill command used by an admin, credited to the admin
WF_KillPlayer(targetid, playerid, WEAPON_UNKNOWN);
```

## WF_RevivePlayer
**New.**

```pawn
native WF_RevivePlayer(playerid, Float:health = -1.0);
```

Cancels a death in progress and brings the player back where they are, with `health` (`-1` = maximum health). It clears the animation and makes the player controllable again. It works during the death animation, or after `OnPlayerDeathFinished` returned 0 (the player is still lying in the death animation).

```pawn
public OnPlayerDeathFinished(playerid, bool:cancelable)
{
    if (cancelable) {
        SetPVarInt(playerid, "injured", 1);
        return 0; // stays down
    }
    return 1;
}

CMD:revive(playerid, params[])
{
    new targetid = strval(params);
    if (GetPVarInt(targetid, "injured")) {
        WF_RevivePlayer(targetid, 30.0);
        DeletePVar(targetid, "injured");
    }
    return 1;
}
```

## WF_ResyncPlayer
*Alias: `ResyncPlayer`*

```pawn
native WF_ResyncPlayer(playerid);
```

Re-sends the health and armour to the client and clears its animations.
