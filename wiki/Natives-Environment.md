# Natives: Environment

## WF_SetCustomFallDamage
*Alias: `SetCustomFallDamage`*

```pawn
native WF_SetCustomFallDamage(bool:toggle, Float:damage_multiplier = 25.0, Float:death_velocity = -0.6);
```

Replaces GTA's fall damage with damage computed from the vertical speed at landing. Falling faster than `death_velocity` kills. Below it, the damage scales from 0 at -0.2 up to `damage_multiplier`. Disabled by default.

## WF_SetCustomVendingMachines
*Alias: `SetCustomVendingMachines`*

```pawn
native WF_SetCustomVendingMachines(bool:toggle);
```

GTA's vending machines heal the client directly, which doesn't work with server-side health. When enabled (default), the plugin replaces every vending machine of the map with an object and handles their use itself: animation, sound and health (35 HP by default, see [`OnPlayerUseVendingMachine`](Callbacks#onplayerusevendingmachine)). A player needs money and less than full health to use one.

## WF_SetVehiclePassengerDamage / WF_SetVehicleUnoccupiedDamage
*Aliases: `SetVehiclePassengerDamage`, `SetVehicleUnoccupiedDamage`*

```pawn
native WF_SetVehiclePassengerDamage(bool:toggle);
native WF_SetVehicleUnoccupiedDamage(bool:toggle);
```

GTA only damages vehicles through their driver's client. These options damage vehicles that have no driver: vehicles with only passengers, and/or empty vehicles. An empty vehicle destroyed this way explodes after a few seconds and respawns 10 seconds later. Both are disabled by default.

## WF_SetDamageSounds
*Alias: `SetDamageSounds`*

```pawn
native WF_SetDamageSounds(taken, given);
```

Sound ids played to the victim (`taken`, default 1190) and to the attacker (`given`, default 17802, the "hit marker") on each hit. `0` disables a sound. The taken sound is disabled by default when lag compensation is off. Spectators hear the sounds of the player they spectate.
