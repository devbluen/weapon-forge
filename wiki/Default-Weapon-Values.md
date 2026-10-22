# Default Weapon Values

The defaults come from weapon-config, which takes them from GTA:SA's `weapon.dat`. All of them can be changed:

- **Damage** with [`SetWeaponDamage`](Natives-Weapons-and-Damage#wf_setweapondamage). "×1" means the damage reported by the game is used as is (`DAMAGE_TYPE_MULTIPLIER`); a number means a fixed amount per hit (`DAMAGE_TYPE_STATIC`).
- **Range** with `SetWeaponMaxRange` (bullet weapons).
- **Rate** with `SetWeaponShootRate`: the minimum interval between shots, in ms.

| Id | Weapon | Damage | Range | Rate (ms) |
|---|---|---|---|---|
| 0 | Fist | ×1 | 1.76 | 250 |
| 1 | Brass knuckles | ×1 | 1.76 | 250 |
| 2 | Golf club | ×1 | 1.76 | 250 |
| 3 | Nightstick | ×1 | 1.76 | 250 |
| 4 | Knife | ×1 | 1.76 | 250 |
| 5 | Bat | ×1 | 1.76 | 250 |
| 6 | Shovel | ×1 | 1.6 | 250 |
| 7 | Pool cue | ×1 | 1.76 | 250 |
| 8 | Katana | ×1 | 1.76 | 250 |
| 9 | Chainsaw | ×1 | 1.76 | 30 |
| 10-13 | Dildos / vibrators | ×1 | 1.76 | 250 |
| 14 | Flowers | ×1 | 1.76 | 250 |
| 15 | Cane | ×1 | 1.76 | 250 |
| 16 | Grenade | ×82.5 | 40 | - |
| 18 | Molotov | ×1 | 40 | - |
| 19 | Vehicle gun (M4) | 9.9 | 90 | 90 |
| 20 | Vehicle gun (minigun) | 46.2 | 75 | 20 |
| 21 | Vehicle rocket | ×82.5 | - | - |
| 22 | Colt 45 | 8.25 | 35 | 160 |
| 23 | Silenced pistol | 13.2 | 35 | 120 |
| 24 | Deagle | 46.2 | 35 | 120 |
| 25 | Shotgun | 3.3 per pellet | 40 | 800 |
| 26 | Sawn-off shotgun | 3.3 per pellet | 35 | 120 |
| 27 | Combat shotgun | 4.95 per pellet | 40 | 120 |
| 28 | Mac-10 (Uzi) | 6.6 | 35 | 50 |
| 29 | MP5 | 8.25 | 45 | 90 |
| 30 | AK-47 | 9.9 | 70 | 90 |
| 31 | M4 | 9.9 | 90 | 90 |
| 32 | Tec-9 | 6.6 | 35 | 50 |
| 33 | Country rifle | 24.75 | 100 | 800 |
| 34 | Sniper | 41.25 | 320 | 800 |
| 35 | Rocket launcher | ×82.5 | 55 | - |
| 36 | Heat seeking RPG | ×82.5 | 55 | - |
| 37 | Flamethrower | ×1 | 5.1 | - |
| 38 | Minigun | 46.2 | 75 | 20 |
| 39 | Satchel | ×82.5 | 40 | - |
| 41 | Spraycan | 0.33 | 6.1 | 10 |
| 42 | Fire extinguisher | 0.33 | 10.1 | 10 |
| 48 | Pistol whip | 2.64 | - | 400 |
| 49 | Vehicle | 9.9 | - | - |
| 50 | Helicopter blades | 330 | - | - |
| 51 | Explosion | ×82.5 | - | - |
| 52 | Car parking | ×1 | - | - |
| 53 | Drowning | ×1 | - | - |
| 54 | Splat (fall/collision) | ×165 | - | - |
| 55 | Unknown (script damage) | ×1 | - | - |

Notes:

- **Explosions:** the game reports up to 82.5 damage. The plugin normalises it to 0-1 and multiplies by the configured value, so the default ×82.5 keeps GTA's damage.
- **Falls:** collision damage is normalised to 0-1 and multiplied by 165. With [`SetCustomFallDamage`](Natives-Environment#wf_setcustomfalldamage), the multiplier given there is used instead.
- **Melee:** fist and melee damage depends on the fighting style and the move, so they are multipliers. The plugin validates the amounts the game can produce.
- **Armour:** by default armour absorbs everything except falls, drowning, car park and script damage. With `SetCustomArmourRules(true, ...)`, bullet weapons only hit armour on the torso; see [`SetWeaponArmourRule`](Natives-Weapons-and-Damage#wf_setcustomarmourrules--wf_setweaponarmourrule).
