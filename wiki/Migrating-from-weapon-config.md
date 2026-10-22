# Migrating from weapon-config

weapon-forge keeps weapon-config's functions, constants and callbacks. For most gamemodes, migrating is just swapping the include and the plugin.

## Steps

1. [Install](Installation) the plugin (`plugins/` on SA-MP, `components/` on open.mp).
2. Replace the include:
   ```pawn
   // #include <weapon-config>
   #include <weapon-forge>
   ```
3. Remove Pawn.RakNet / SKY if weapon-config was the only reason you had them. weapon-forge reads the sync packets by itself.
4. Remove `WC_*` defines that tuned the include (`WC_DEBUG`, `WC_FEED_HEIGHT`, ...). Use the equivalent natives listed below.
5. Recompile the gamemode **and** the filterscripts.

## What stays the same

- Callbacks: `OnPlayerDamage`, `OnPlayerDamageDone`, `OnPlayerPrepareDeath`, `OnPlayerDeathFinished`, `OnRejectedHit`, `OnInvalidWeaponDamage`, `OnPlayerUseVendingMachine`. The signatures are identical.
- Constants: `DAMAGE_TYPE_*`, `WC_*` errors, `HIT_*` reasons, `E_REJECTED_HIT`, `WEAPON_PISTOLWHIP`, `WEAPON_CARPARK`, `WEAPON_UNKNOWN`, ...
- Functions with the same names (aliases of the `WF_*` natives): `SetWeaponDamage`, `GetWeaponDamage`, `SetWeaponMaxRange`, `SetWeaponShootRate`, `SetCustomArmourRules`, `SetWeaponArmourRule`, `SetWeaponName`, `DamagePlayer`, `SetPlayerMaxHealth`, `SetPlayerMaxArmour`, `GetLastDamageHealth`, `IsPlayerDying`, `SetRespawnTime`, `ResyncPlayer`, `SetCbugAllowed`, `SetCbugDeathDelay`, `GetRejectedHit`, `AverageShootRate`, `AverageHitRate`, `SetKnifeSync`, `SetDisableSyncBugs`, `SetCustomFallDamage`, `SetCustomVendingMachines`, `SetVehiclePassengerDamage`, `SetVehicleUnoccupiedDamage`, `SetDamageSounds`, `SetDamageFeed`, `SetDamageFeedForPlayer`, `IsDamageFeedActive`, the health bar functions, `WC_IsPlayerSpawned`, `WC_IsPlayerPaused`, `WC_GetWeaponName`, `IsBulletWeapon`, `IsMeleeWeapon`, `IsHighRateWeapon`.
- Behaviour: damage validation rules, default weapon values, death animations, damage feed, health bar, vending machines and c-bug punishment are ported from weapon-config.

Define `WF_NO_COMPAT` before the include if those names clash with your own code; only the `WF_*` names are declared then.

## Differences

| weapon-config | weapon-forge |
|---|---|
| `SetPlayerHealth(playerid, health, armour)` | `SetPlayerHealth` keeps the server's 2-parameter signature. Use `WF_SetPlayerHealth(playerid, health, armour)` |
| `new Float:hp = GetPlayerHealth(playerid);` | `GetPlayerHealth` returns 1/0 like the original native. Use `WF_GetPlayerHealth(playerid)`, which returns the value |
| `ReturnWeaponName(weaponid)` | Not available; use `GetWeaponName(weaponid, name)`, which returns the weapon-forge names |
| `ResyncPlayer` respawns the player in place | Re-sends health/armour and clears animations |
| Each script has its own state | One shared state: a filterscript reading `GetPlayerHealth` gets the same server-side value as the gamemode |
| Needs the include in every script | Works in every script without the include |
| `#define WC_FEED_HEIGHT` and similar | Not configurable at compile time; see the natives below |
| Requires Pawn.RakNet or SKY | No dependency |
| `OnPlayerGiveDamage` / `OnPlayerTakeDamage` are not called in the script | Same by default; `WF_SetForwardDamageCallbacks(true)` calls them |
| Cancelling a real client death in `OnPlayerDamage` respawns the player in place | The death is hidden from the scripts, but the client still respawns normally |
| Settings survive only while the script is loaded | Settings are reset to the defaults on `gmx` (same result) |

## Compile-time defines

| weapon-config define | weapon-forge equivalent |
|---|---|
| `WC_DEBUG` / `WC_DEBUG_SILENT` | `WF_SetDebug(1)` (console only) |
| `WC_FEED_GIVEN_COLOR` / `WC_FEED_TAKEN_COLOR` | `WF_SetDamageFeedColors(given, taken)` |
| `WC_HEALTH_BAR_FG_COLOR` / `WC_HEALTH_BAR_BG_COLOR` | `SetHealthBarColor(border, bg, fg)` |
| `WC_CUSTOM_VENDING_MACHINES` | `SetCustomVendingMachines(bool)` |
| `WC_MAX_REJECTED_HITS`, `WC_FEED_HEIGHT`, `WC_MAX_DAMAGE_RANGES` | Fixed: 15 rejected hits, 5 feed lines, 8 damage ranges |

## open.mp NPCs

NPCs created with open.mp's `NPC_Create` are not respawned by `SpawnPlayer`. Respawn them yourself:

```pawn
public OnPlayerDeathFinished(playerid, bool:cancelable)
{
    if (cancelable && IsPlayerNPC(playerid)) {
        NPC_Respawn(playerid);
    }
    return 1;
}
```

SA-MP NPCs (`ConnectNPC`) are respawned by the plugin.
