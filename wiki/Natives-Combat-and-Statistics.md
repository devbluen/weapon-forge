# Natives: Combat and Statistics

All of these are new in weapon-forge.

## WF_SetCombatTimeout / WF_IsPlayerInCombat

```pawn
native WF_SetCombatTimeout(ms);
native bool:WF_IsPlayerInCombat(playerid);
```

A player is in combat after giving or taking damage, until `ms` milliseconds pass without damage (default 10000). Raises `OnPlayerEnterCombat` / `OnPlayerLeaveCombat`. `0` disables the combat state.

```pawn
CMD:quit(playerid)
{
    if (WF_IsPlayerInCombat(playerid)) {
        return SendClientMessage(playerid, -1, "You can't leave during combat.");
    }
    ...
}
```

## WF_GetLastAttacker / WF_GetTimeSinceLastDamage

```pawn
native WF_GetLastAttacker(playerid);
native WF_GetTimeSinceLastDamage(playerid);
```

Last player that damaged `playerid` (`INVALID_PLAYER_ID` if none), and milliseconds since the last damage given or taken (`-1` if never).

## WF_GetPlayerHitHistory

```pawn
native WF_GetPlayerHitHistory(playerid, idx, &issuerid, &WEAPON:weaponid, &Float:amount, &bodypart, &ms_ago);
```

The last 10 hits taken by a player. `idx` 0 is the most recent. Returns 0 when there is no hit at `idx`.

```pawn
new issuer, WEAPON:weapon, Float:amount, bodypart, ago;
for (new i = 0; WF_GetPlayerHitHistory(playerid, i, issuer, weapon, amount, bodypart, ago); i++) {
    printf("%d: %d hit with %d (%.1f) %d ms ago", i, issuer, _:weapon, amount, ago);
}
```

## WF_GetLastShotInfo

```pawn
native WF_GetLastShotInfo(playerid, &WEAPON:weaponid, &hittype, &hitid, &Float:hit_x, &Float:hit_y, &Float:hit_z, &Float:length);
```

The player's last accepted shot: weapon, `BULLET_HIT_TYPE_*`, hit id, hit position and bullet length. Returns the milliseconds since the shot, or `-1` if none.

## WF_SetAssistParams

```pawn
native WF_SetAssistParams(Float:min_damage = 10.0, window = 15000);
```

A player gets an assist (`OnPlayerKillAssist`, `WF_STAT_ASSISTS`) when they dealt at least `min_damage` to the victim in the last `window` milliseconds before someone else killed them. `min_damage = 0.0` disables assists.

## WF_GetPlayerStat / WF_GetPlayerStatFloat

```pawn
native WF_GetPlayerStat(playerid, stat);
native Float:WF_GetPlayerStatFloat(playerid, stat);
```

Statistics collected since the player connected (or since `WF_ResetPlayerStats`):

| `WF_GetPlayerStat` | |
|---|---|
| `WF_STAT_SHOTS_FIRED` | Accepted shots |
| `WF_STAT_SHOTS_HIT` | Hits dealt to other players |
| `WF_STAT_HITS_TAKEN` | Hits taken |
| `WF_STAT_HEADSHOTS_GIVEN` / `WF_STAT_HEADSHOTS_TAKEN` | Bullet hits on the head |
| `WF_STAT_KILLS` / `WF_STAT_DEATHS` / `WF_STAT_ASSISTS` | |
| `WF_STAT_REJECTED_HITS` | Shots/hits rejected by the validation (useful for anti-cheat) |

| `WF_GetPlayerStatFloat` | |
|---|---|
| `WF_STATF_DAMAGE_GIVEN` / `WF_STATF_DAMAGE_TAKEN` | Total damage |
| `WF_STATF_HEALTH_LOST` / `WF_STATF_ARMOUR_LOST` | How the damage taken was split |

## WF_GetPlayerAccuracy / WF_ResetPlayerStats

```pawn
native Float:WF_GetPlayerAccuracy(playerid);
native WF_ResetPlayerStats(playerid);
```

Accuracy in percent (`shots hit / shots fired`), and a reset of every statistic (e.g. at the start of a match).

```pawn
public OnPlayerDeath(playerid, killerid, WEAPON:reason)
{
    new msg[128];
    format(msg, sizeof(msg), "Accuracy %.1f%% | K %d / D %d / A %d",
        WF_GetPlayerAccuracy(playerid),
        WF_GetPlayerStat(playerid, WF_STAT_KILLS),
        WF_GetPlayerStat(playerid, WF_STAT_DEATHS),
        WF_GetPlayerStat(playerid, WF_STAT_ASSISTS));
    SendClientMessage(playerid, -1, msg);
    return 1;
}
```
