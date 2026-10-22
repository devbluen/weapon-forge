# Natives

Declared in `weapon-forge.inc`. Names in *italics* are the weapon-config compatible aliases, which are not declared when `WF_NO_COMPAT` is defined.

## Replaced server natives

The plugin replaces these natives in **every** script. They keep their names and signatures but work with the server-side state:

| Native | Behaviour with weapon-forge |
|---|---|
| `SetPlayerHealth` / `GetPlayerHealth` | Server-side health. `SetPlayerHealth(playerid, 0.0)` kills with the death animation. `GetPlayerHealth` returns 1/0 like the original |
| `SetPlayerArmour` / `GetPlayerArmour` | Server-side armour |
| `GetPlayerState` | Returns `PLAYER_STATE_WASTED` while the death animation plays |
| `SpawnPlayer` | Refused while dying; restores full health if it was 0 |
| `SetPlayerTeam` / `GetPlayerTeam` | Team used by the friendly fire rules |
| `SendDeathMessage` | Maps custom weapon ids (pistol whip, car park, ...) to ones the client knows |
| `GetWeaponName` | Weapon names from weapon-forge (customisable with `SetWeaponName`) |
| `ApplyAnimation`, `ClearAnimations`, `TogglePlayerControllable`, `SetPlayerPos`, `SetPlayerPosFindZ`, `SetPlayerVelocity` | Ignored while dying (so the death animation isn't broken) |
| `SetPlayerVirtualWorld` / `GetPlayerVirtualWorld` | Handle the temporary world used during the respawn |
| `TogglePlayerSpectating`, `PlayerSpectatePlayer` | Cancel the death / track spectators (the feed is shown to them) |
| `DestroyVehicle` | Resets the vehicle's damage state |
| `IsPlayerInCheckpoint`, `IsPlayerInRaceCheckpoint`, `SetPlayerSpecialAction` | Return 0 / refused while the player isn't spawned |

## Categories

| Page | Natives |
|---|---|
| [Weapons and Damage](Natives-Weapons-and-Damage) | `WF_SetWeaponDamage`, `WF_GetWeaponDamage`, `WF_GetWeaponDamageType`, `WF_SetWeaponMaxRange`, `WF_GetWeaponMaxRange`, `WF_SetWeaponShootRate`, `WF_GetWeaponShootRate`, `WF_SetWeaponArmourRule`, `WF_SetCustomArmourRules`, `WF_SetWeaponBodypartMultiplier`, `WF_GetWeaponBodypartMultiplier`, `WF_SetWeaponName`, `WF_GetWeaponName`, `WF_IsBulletWeapon`, `WF_IsMeleeWeapon`, `WF_IsHighRateWeapon`, `WF_DamagePlayer` |
| [Health and Death](Natives-Health-and-Death) | `WF_SetPlayerHealth`, `WF_GetPlayerHealth`, `WF_SetPlayerArmour`, `WF_GetPlayerArmour`, `WF_SetPlayerMaxHealth`, `WF_SetPlayerMaxArmour`, `WF_GetPlayerMaxHealth`, `WF_GetPlayerMaxArmour`, `WF_GetLastDamageHealth`, `WF_GetLastDamageArmour`, `WF_IsPlayerDying`, `WF_IsPlayerSpawned`, `WF_IsPlayerPaused`, `WF_SetRespawnTime`, `WF_GetRespawnTime`, `WF_KillPlayer`, `WF_RevivePlayer`, `WF_ResyncPlayer` |
| [Protection and Modifiers](Natives-Protection-and-Modifiers) | `WF_SetPlayerGodMode`, `WF_IsPlayerGodMode`, `WF_SetSpawnProtection`, `WF_GetSpawnProtection`, `WF_SetPlayerProtection`, `WF_GetPlayerProtectionTime`, `WF_SetPlayerDamageMultiplier`, `WF_GetPlayerDamageMultiplier`, `WF_SetFriendlyFire`, `WF_GetFriendlyFire` |
| [Combat and Statistics](Natives-Combat-and-Statistics) | `WF_SetCombatTimeout`, `WF_IsPlayerInCombat`, `WF_GetLastAttacker`, `WF_GetTimeSinceLastDamage`, `WF_GetPlayerHitHistory`, `WF_GetLastShotInfo`, `WF_SetAssistParams`, `WF_GetPlayerStat`, `WF_GetPlayerStatFloat`, `WF_ResetPlayerStats`, `WF_GetPlayerAccuracy` |
| [Health Over Time](Natives-Health-Over-Time) | `WF_SetHealthRegen`, `WF_SetPlayerHealthRegen`, `WF_SetPlayerBleeding`, `WF_StopPlayerBleeding`, `WF_IsPlayerBleeding` |
| [Anti-Cheat and Sync](Natives-Anti-Cheat-and-Sync) | `WF_SetCbugAllowed`, `WF_GetCbugAllowed`, `WF_SetCbugDeathDelay`, `WF_GetRejectedHit`, `WF_AverageShootRate`, `WF_AverageHitRate`, `WF_SetKnifeSync`, `WF_SetDisableSyncBugs`, `WF_SetLagCompMode`, `WF_GetLagCompMode`, `WF_SetForwardDamageCallbacks` |
| [Environment](Natives-Environment) | `WF_SetCustomFallDamage`, `WF_SetCustomVendingMachines`, `WF_SetVehiclePassengerDamage`, `WF_SetVehicleUnoccupiedDamage`, `WF_SetDamageSounds` |
| [Damage Feed and Health Bar](Natives-Damage-Feed-and-Health-Bar) | `WF_SetDamageFeed`, `WF_SetDamageFeedForPlayer`, `WF_IsDamageFeedActive`, `WF_SetDamageFeedHideDelay`, `WF_SetDamageFeedColors`, `WF_SetDamageFeedPosition`, `WF_EnableHealthBarForPlayer`, health bar position/size/padding/color |
| [Miscellaneous](Natives-Miscellaneous) | `WF_SetDebug`, `WF_IsOpenMP`, `WF_IsRakNetActive`, `WF_GetPlayerRawHealth`, `WF_GetVersion` |

## Settings and gmx

Every setting changed with these natives is global and kept by the plugin until the gamemode exits. On `gmx` everything is reset to the defaults, so configure the plugin in `OnGameModeInit`.
