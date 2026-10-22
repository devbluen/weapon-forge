# Natives: Damage Feed and Health Bar

## Damage feed

The damage feed shows the last hits given (green, left) and taken (blue, right) on the player's screen. Consecutive hits on or from the same player are merged. Spectators see the feed of the player they spectate.

### WF_SetDamageFeed / WF_SetDamageFeedForPlayer / WF_IsDamageFeedActive
*Aliases: `SetDamageFeed`, `SetDamageFeedForPlayer`, `IsDamageFeedActive`*

```pawn
native WF_SetDamageFeed(bool:toggle);
native WF_SetDamageFeedForPlayer(playerid, toggle = -1);
native WF_IsDamageFeedActive(playerid = INVALID_PLAYER_ID);
```

Enables the feed globally (default on) or per player. For `WF_SetDamageFeedForPlayer`, `toggle` is `1` to show, `0` to hide, and `-1` to follow the global setting.

### WF_SetDamageFeedHideDelay / WF_SetDamageFeedColors / WF_SetDamageFeedPosition
**New.**

```pawn
native WF_SetDamageFeedHideDelay(ms);
native WF_SetDamageFeedColors(given_color, taken_color);
native WF_SetDamageFeedPosition(Float:given_x, Float:given_y, Float:taken_x, Float:taken_y);
```

| Setting | Default |
|---|---|
| Hide delay | 3000 ms (minimum 100) |
| Colors | given `0x30FF50FF`, taken `0x33CCFFFF` |
| Position | given `200.0, 365.0`, taken `440.0, 365.0` (textdraw coordinates, centred) |

## Health bar

The client's real health is kept at 8000000+, so the HUD health bar is replaced by textdraws showing the server-side health. The bar is shown while the player is on foot or in a vehicle.

### WF_EnableHealthBarForPlayer
*Alias: `EnableHealthBarForPlayer`*

```pawn
native WF_EnableHealthBarForPlayer(playerid, bool:enable);
```

Shows or hides the health bar for one player, for example when your gamemode draws its own HUD.

### Global settings
*Aliases: `SetHealthBarPosition`, `SetHealthBarSize`, `SetHealthBarPadding`, `SetHealthBarColor` and the `Get` versions*

```pawn
native WF_SetHealthBarPosition(Float:x, Float:y);
native WF_SetHealthBarSize(Float:x, Float:y);
native WF_SetHealthBarPadding(const Float:padding[4]);          // top, right, bottom, left
native WF_SetHealthBarColor(borderColor = 0, bgColor = 0, fgColor = 0); // 0 keeps the current color
native WF_GetHealthBarPosition(&Float:x, &Float:y, playerid = INVALID_PLAYER_ID);
native WF_GetHealthBarSize(&Float:x, &Float:y, playerid = INVALID_PLAYER_ID);
native WF_GetHealthBarPadding(Float:padding[4], playerid = INVALID_PLAYER_ID);
native WF_GetHealthBarColor(&borderColor = 0, &bgColor = 0, &fgColor = 0, playerid = INVALID_PLAYER_ID);
```

| Setting | Default |
|---|---|
| Position | `546.0, 66.7` (over the GTA health bar) |
| Size | `61.7, 8.4` |
| Padding | `2.1, 1.9, 1.6, 2.0` |
| Colors | border `0x000000FF`, background `0x5A0C0EFF`, foreground `0xB4191DFF` |

The `Get` versions return a player's effective values when `playerid` is given.

### Per-player settings
*Aliases: `SetHealthBarPositionForPlayer`, `SetHealthBarSizeForPlayer`, `SetHealthBarPaddingForPlayer`, `SetHealthBarColorForPlayer`*

```pawn
native WF_SetHealthBarPosForPlayer(playerid, Float:x = Float:0x7FFFFFFF, Float:y = Float:0x7FFFFFFF);
native WF_SetHealthBarSizeForPlayer(playerid, Float:x = Float:0x7FFFFFFF, Float:y = Float:0x7FFFFFFF);
native WF_SetHealthBarPaddingForPlayer(playerid, const Float:padding[4] = {...});
native WF_SetHealthBarColorForPlayer(playerid, borderColor = 0, bgColor = 0, fgColor = 0);
```

Override the global settings for one player, for example for HUD mods or a different screen layout. Calling them without values (or `0` for colors) goes back to the global settings.

```pawn
// Move the bar for players using a custom HUD
WF_SetHealthBarPosForPlayer(playerid, 500.0, 100.0);
WF_SetHealthBarColorForPlayer(playerid, 0x000000FF, 0x0E3D5AFF, 0x1D7DB4FF); // blue bar
```
