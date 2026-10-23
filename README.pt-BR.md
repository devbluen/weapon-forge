# weapon-forge

[English](README.md) | **Português**

Plugin de **vida/colete server-side, dano, morte e configuração de armas** para **SA-MP 0.3.7 / 0.3.DL** e **open.mp**. É um substituto do [weapon-config](https://github.com/oscar-broman/samp-weapon-config) com **toda** a lógica dentro do plugin.

- **Tudo no plugin.** Ele intercepta sozinho os callbacks e as natives do servidor. O include só **declara** natives, constantes e callbacks: não tem hooks, publics, macros de substituição nem código.
- **Um binário por sistema operacional para os dois servidores.** O mesmo `weapon-forge.dll`/`.so` é plugin do SA-MP e *component* do open.mp.
- **Sem dependências.** Não precisa de Pawn.RakNet nem de outro plugin: o acesso aos pacotes de sync é nativo nos dois servidores.
- **API compatível com o weapon-config.** As mesmas funções, constantes e callbacks (`OnPlayerDamage`, `OnPlayerDamageDone`, `OnPlayerPrepareDeath`, `OnPlayerDeathFinished`, `OnRejectedHit`, `SetWeaponDamage`, `SetCbugAllowed`, `DamagePlayer`...).
- **Funções novas.** God mode, proteção de spawn, multiplicadores por jogador e por parte do corpo, fogo amigo configurável, estado de combate, histórico de hits, assistências, sangramento, regeneração, estatísticas, `WF_KillPlayer`/`WF_RevivePlayer` e mais.

📖 **Documentação completa (em inglês): veja a [wiki](../../wiki)** (fontes em [`wiki/`](wiki)).

## Por que é mais rápido

O weapon-config roda tudo em Pawn. Ele processa **todo pacote de sync** (onfoot, veículo, passageiro, mira) via Pawn.RakNet, com dezenas de chamadas `BS_*` por pacote e por jogador, além de hooks ALS, `format` e dezenas de `SetTimerEx`.

| | weapon-config | weapon-forge |
|---|---|---|
| Pacotes de sync | Lidos e reescritos em Pawn | Bytes alterados em C++ antes do servidor ler o pacote. **Nenhum código Pawn por pacote** |
| Callbacks (dano, tiros, update...) | Hooks ALS em Pawn em cada script | Processados no plugin antes de qualquer script |
| Validação de dano/tiros, feed, barra de vida | Pawn | C++ |
| Timers (morte, c-bug, feed, faca...) | `SetTimerEx` | Fila de timers nativa |
| Estado (vida, colete, morte) | Arrays por script, perdidos no `gmx` | No plugin, o mesmo para gamemode e filterscripts |

## Compatibilidade

| Servidor | Windows | Linux | Como carregar |
|---|---|---|---|
| SA-MP 0.3.7 / 0.3.DL | `plugins/weapon-forge.dll` | `plugins/weapon-forge.so` | `plugins weapon-forge` no `server.cfg` |
| open.mp | `components/weapon-forge.dll` | `components/weapon-forge.so` | automático (pasta `components/`) |

O arquivo de `plugins/` e o de `components/` são **o mesmo binário**: a release já traz as duas pastas. No open.mp também funciona como *legacy plugin* (`legacy_plugins`), mas aí o open.mp não dá acesso à rede, então a vida no nametag e as correções de sync ficam desligadas (o plugin avisa no log).

Testado em servidor real: SA-MP 0.3.7-R2 e open.mp 1.6.7 (veja [Testes](#testes)).

> Não existe um arquivo único que rode em Windows **e** Linux: são formatos de executável diferentes. O que existe é um binário por SO, o mesmo para SA-MP e open.mp.

## Instalação

1. Baixe a release (`weapon-forge-X.Y.Z-windows.zip` ou `-linux.tar.gz`) ou compile (veja [Compilando](#compilando)).
2. **SA-MP:** copie `plugins/weapon-forge.dll` (ou `.so`) para `plugins/` e adicione `weapon-forge` à linha `plugins` do `server.cfg`.
   **open.mp:** copie `components/weapon-forge.dll` (ou `.so`) para `components/`. Não precisa configurar nada.
3. Para usar as funções e callbacks do weapon-forge, copie `include/weapon-forge.inc` para a pasta de includes e inclua-o:
   ```pawn
   #include <open.mp>      // ou <a_samp>
   #include <weapon-forge>
   ```

O include é opcional. Sem ele o plugin funciona igual em **todos** os scripts: vida server-side, mortes, validação de dano, feed e barra de vida. O include só é necessário para chamar as natives `WF_*` ou implementar os callbacks (`OnPlayerDamage` etc.).

### Migrando do weapon-config

1. Troque `#include <weapon-config>` por `#include <weapon-forge>`.
2. Remova o Pawn.RakNet/SKY se só eram usados pelo weapon-config.
3. Recompile o gamemode e os filterscripts.

Diferenças em relação ao weapon-config:

- `SetPlayerHealth` mantém a assinatura original do servidor (sem o 3º parâmetro de colete do weapon-config). Use `WF_SetPlayerHealth(playerid, health, armour)`.
- `GetPlayerHealth`/`GetPlayerArmour` retornam 1/0 como a native original. Use `WF_GetPlayerHealth` para receber o valor como retorno.
- `ReturnWeaponName` não existe. Use `GetWeaponName` (já retorna os nomes do weapon-forge).
- `ResyncPlayer` reenvia vida/colete e limpa animações (não faz "spawn no lugar").
- Cancelar uma **morte real** do cliente (ex.: veículo explodindo com o jogador dentro) em `OnPlayerDamage` esconde a morte dos scripts, mas o cliente ainda passa pelo respawn normal.
- `OnPlayerGiveDamage`/`OnPlayerTakeDamage` não chegam aos scripts, igual ao weapon-config. Use `WF_SetForwardDamageCallbacks(true)` se precisar deles.
- No `gmx`, todas as configurações voltam ao padrão (como no weapon-config, onde cada carregamento começa do zero).
- NPCs nativos do open.mp (`NPC_Create`) precisam de `NPC_Respawn` em `OnPlayerDeathFinished`. NPCs do SA-MP são respawnados pelo plugin.

## Como funciona

O plugin instala hooks (detours x86) nas funções da máquina virtual Pawn que o próprio servidor usa:

- **`amx_Register`:** quando o servidor registra suas natives, o plugin guarda os ponteiros de todas (para chamá-las internamente) e troca, em todos os scripts, `SetPlayerHealth`, `GetPlayerHealth`, `SetPlayerArmour`, `GetPlayerArmour`, `GetPlayerState`, `SpawnPlayer`, `SetPlayerTeam`, `GetPlayerTeam`, `SendDeathMessage`, `GetWeaponName`, `ApplyAnimation`, `ClearAnimations`, `TogglePlayerSpectating`, `TogglePlayerControllable`, `SetPlayerPos`, `SetPlayerPosFindZ`, `SetPlayerVelocity`, `SetPlayerVirtualWorld`, `GetPlayerVirtualWorld`, `PlayerSpectatePlayer`, `DestroyVehicle`, `IsPlayerInCheckpoint`, `IsPlayerInRaceCheckpoint` e `SetPlayerSpecialAction` pelas versões server-side.
- **`amx_Exec`:** cada callback do servidor (`OnPlayerGiveDamage`, `OnPlayerWeaponShot`, `OnPlayerUpdate`, `OnPlayerDeath`, `OnPlayerSpawn`...) é processado **uma vez**, antes do primeiro script. O plugin aplica a mesma decisão em todos: deixa rodar, bloqueia, ou corrige o assassino/arma de uma morte real.
- **`amx_FindPublic`:** se nenhum script implementa um callback de que o plugin precisa, ele é informado como existente no gamemode, para o servidor sempre gerar o evento.
- **Rede:** no SA-MP, o plugin envolve o `RakServer::Receive` (obtido pelo `ppData` do servidor). No open.mp (component), usa os eventos de rede do próprio open.mp. Os pacotes onfoot/veículo/passageiro/mira recebem a vida/colete server-side, o congelamento durante a morte e as correções de sync antes do servidor processá-los.

A vida real do cliente fica em `8000000 + vida`, então o cliente nunca morre sozinho. A barra de vida do HUD é desenhada com textdraws, e os outros jogadores veem o valor certo no nametag porque os pacotes de sync carregam a vida server-side.

## Callbacks

```pawn
// Antes do dano ser aplicado. Retorne 0 para cancelar; todos os argumentos podem ser alterados.
public OnPlayerDamage(&playerid, &Float:amount, &issuerid, &WEAPON:weapon, &bodypart)

// Depois do dano ser aplicado
public OnPlayerDamageDone(playerid, Float:amount, issuerid, WEAPON:weapon, bodypart)

// Antes da animação de morte. Pode trocar a animação, travar o jogador e mudar o tempo de respawn.
public OnPlayerPrepareDeath(playerid, animlib[32], animname[32], &anim_lock, &respawn_time)

// Fim da animação de morte. Com cancelable = true, retorne 0 para manter o jogador vivo (ex.: sistema de "ferido").
public OnPlayerDeathFinished(playerid, bool:cancelable)

// Um tiro/dano foi rejeitado (anti-cheat). Veja HIT_* e GetRejectedHit.
public OnRejectedHit(playerid, hit[E_REJECTED_HIT])

// Dano inválido (WC_NO_ISSUER, WC_NO_DAMAGED, WC_INVALID_DAMAGE, WC_INVALID_DISTANCE)
public OnInvalidWeaponDamage(playerid, damagedid, Float:amount, WEAPON:weaponid, bodypart, error, bool:given)

// Jogador vai usar uma máquina de venda. Retorne 0 para impedir; altere health_given.
public OnPlayerUseVendingMachine(playerid, &Float:health_given)

// --- novos ---

// Bala acertou a cabeça (bodypart 9)
public OnPlayerHeadshot(playerid, issuerid, WEAPON:weaponid, Float:amount)

// Para cada jogador que deu dano suficiente na vítima antes de outro matá-la
public OnPlayerKillAssist(playerid, victimid, killerid, Float:damage)

// Jogador deu/levou dano estando fora de combate / o tempo de combate expirou
public OnPlayerEnterCombat(playerid)
public OnPlayerLeaveCombat(playerid)

// C-bug detectado (com SetCbugAllowed(false)). Retorne 0 para não punir.
public OnPlayerCbug(playerid, WEAPON:weaponid, interval)
```

Os callbacks são chamados em todos os scripts que os implementam (filterscripts primeiro, gamemode por último). `OnPlayerDeath` continua funcionando normalmente: o plugin o chama quando o jogador morre server-side, com o assassino e a arma corretos.

## Funções

Os nomes entre parênteses são os nomes do weapon-config, declarados no include como aliases (desligue com `#define WF_NO_COMPAT`).

### Armas e dano

| Função | Descrição |
|---|---|
| `WF_SetWeaponDamage(weaponid, damage_type, Float:amount, Float:...)` (`SetWeaponDamage`) | Dano da arma. Tipos: `DAMAGE_TYPE_MULTIPLIER`, `DAMAGE_TYPE_STATIC`, `DAMAGE_TYPE_RANGE`, `DAMAGE_TYPE_RANGE_MULTIPLIER` (pares `distância, valor`: `SetWeaponDamage(WEAPON_SNIPER, DAMAGE_TYPE_RANGE, 41.0, 50.0, 30.0, 100.0, 20.0)`) |
| `Float:WF_GetWeaponDamage(weaponid)` (`GetWeaponDamage`) / `WF_GetWeaponDamageType` | Dano e tipo configurados |
| `WF_SetWeaponMaxRange` / `Float:WF_GetWeaponMaxRange` (`Set/GetWeaponMaxRange`) | Alcance máximo (armas de bala) |
| `WF_SetWeaponShootRate` / `WF_GetWeaponShootRate` (`Set/GetWeaponShootRate`) | Intervalo mínimo entre tiros (ms) |
| `WF_SetWeaponArmourRule(weaponid, bool:affects_armour, bool:torso_only = false)` | Regra de colete por arma |
| `WF_SetCustomArmourRules(bool:armour_rules, bool:torso_rules = false)` | Ativa as regras de colete |
| `WF_SetWeaponBodypartMultiplier(weaponid, bodypart, Float:multiplier)` | **Novo.** Multiplicador por parte do corpo (`weaponid = -1` = todas). Ex.: headshot `WF_SetWeaponBodypartMultiplier(-1, 9, 2.0)` |
| `WF_SetWeaponName` / `WF_GetWeaponName` (`SetWeaponName`, `GetWeaponName`) | Nome da arma (feed e `GetWeaponName`) |
| `WF_IsBulletWeapon` / `WF_IsMeleeWeapon` / `WF_IsHighRateWeapon` | Classificação |
| `WF_DamagePlayer(playerid, Float:amount, issuerid = INVALID_PLAYER_ID, weaponid = WEAPON_UNKNOWN, bodypart = 0, bool:ignore_armour = false)` (`DamagePlayer`) | Causa dano pelo pipeline completo |

### Vida, colete e morte

| Função | Descrição |
|---|---|
| `SetPlayerHealth` / `GetPlayerHealth` / `SetPlayerArmour` / `GetPlayerArmour` | Valores server-side em **todos** os scripts. `SetPlayerHealth(playerid, 0.0)` mata com animação |
| `WF_SetPlayerHealth(playerid, Float:health, Float:armour = -1.0)` / `Float:WF_GetPlayerHealth` | Versões com colete / retorno em float |
| `WF_SetPlayerMaxHealth` / `WF_SetPlayerMaxArmour` (+ `Get`) | Vida/colete máximos (podem passar de 100) |
| `Float:WF_GetLastDamageHealth(playerid)` / `WF_GetLastDamageArmour` | Quanto o último dano tirou de vida/colete |
| `WF_IsPlayerDying(playerid)` (`IsPlayerDying`) | Está na animação de morte |
| `WF_IsPlayerSpawned(playerid)` (`WC_IsPlayerSpawned`) / `WF_IsPlayerPaused` | Spawnado e vivo / sem sync há mais de 2 s |
| `WF_SetRespawnTime(ms)` / `WF_GetRespawnTime()` | Duração da animação de morte |
| `WF_KillPlayer(playerid, killerid = INVALID_PLAYER_ID, weaponid = WEAPON_UNKNOWN)` | **Novo.** Mata atribuindo assassino e arma |
| `WF_RevivePlayer(playerid, Float:health = -1.0)` | **Novo.** Cancela a morte em andamento e revive |
| `WF_ResyncPlayer(playerid)` (`ResyncPlayer`) | Reenvia vida/colete e limpa animações |
| `Float:WF_GetPlayerRawHealth(playerid, &Float:health = 0.0)` | Valor guardado pelo próprio servidor (o que os outros clientes veem) |

### Proteção e modificadores (novos)

| Função | Descrição |
|---|---|
| `WF_SetPlayerGodMode(playerid, bool:toggle)` / `WF_IsPlayerGodMode` | Ignora todo dano (exceto `SetPlayerHealth(0)`) |
| `WF_SetSpawnProtection(ms)` / `WF_GetSpawnProtection()` | Proteção automática após cada spawn |
| `WF_SetPlayerProtection(playerid, ms)` / `WF_GetPlayerProtectionTime(playerid)` | Proteção manual / tempo restante |
| `WF_SetPlayerDamageMultiplier(playerid, Float:given = 1.0, Float:taken = 1.0)` / `WF_GetPlayerDamageMultiplier` | Multiplicadores de dano causado/recebido |
| `WF_SetFriendlyFire(bool:toggle)` / `WF_GetFriendlyFire()` | Permite dano entre jogadores do mesmo time |

### Combate, histórico e estatísticas (novos)

| Função | Descrição |
|---|---|
| `WF_SetCombatTimeout(ms)` / `WF_IsPlayerInCombat(playerid)` | Estado de combate (padrão 10 s) |
| `WF_GetLastAttacker(playerid)` / `WF_GetTimeSinceLastDamage(playerid)` | Último atacante / ms desde o último dano |
| `WF_GetPlayerHitHistory(playerid, idx, &issuerid, &weaponid, &Float:amount, &bodypart, &ms_ago)` | Últimos 10 hits recebidos (`idx 0` = mais recente) |
| `WF_GetLastShotInfo(playerid, &weaponid, &hittype, &hitid, &Float:x, &Float:y, &Float:z, &Float:length)` | Último tiro válido (retorna ms desde o tiro) |
| `WF_SetAssistParams(Float:min_damage = 10.0, window = 15000)` | Regras de assistência |
| `WF_GetPlayerStat(playerid, stat)` | `WF_STAT_SHOTS_FIRED`, `WF_STAT_SHOTS_HIT`, `WF_STAT_HITS_TAKEN`, `WF_STAT_HEADSHOTS_GIVEN`, `WF_STAT_HEADSHOTS_TAKEN`, `WF_STAT_KILLS`, `WF_STAT_DEATHS`, `WF_STAT_ASSISTS`, `WF_STAT_REJECTED_HITS` |
| `Float:WF_GetPlayerStatFloat(playerid, stat)` | `WF_STATF_DAMAGE_GIVEN`, `WF_STATF_DAMAGE_TAKEN`, `WF_STATF_HEALTH_LOST`, `WF_STATF_ARMOUR_LOST` |
| `Float:WF_GetPlayerAccuracy(playerid)` / `WF_ResetPlayerStats(playerid)` | Precisão em % / zera as estatísticas |

### Vida ao longo do tempo (novos)

| Função | Descrição |
|---|---|
| `WF_SetHealthRegen(Float:health_per_second, delay_after_damage = 5000)` | Regeneração global |
| `WF_SetPlayerHealthRegen(playerid, Float:health_per_second = -1.0, delay_after_damage = -1)` | Por jogador (`-1` = usa o global) |
| `WF_SetPlayerBleeding(playerid, Float:damage_per_second, duration, issuerid = INVALID_PLAYER_ID, weaponid = WEAPON_UNKNOWN)` | Sangramento (o dano conta para o assassino) |
| `WF_StopPlayerBleeding(playerid)` / `WF_IsPlayerBleeding(playerid)` | |

### Anti-cheat e sync

| Função | Descrição |
|---|---|
| `WF_SetCbugAllowed(bool:enabled, playerid = INVALID_PLAYER_ID)` / `WF_GetCbugAllowed` | Permite/proíbe c-bug (global ou por jogador) |
| `WF_SetCbugDeathDelay(bool:toggle)` | Atrasa o `OnPlayerDeath` 1,2 s para desfazer kills de c-bug |
| `WF_GetRejectedHit(playerid, idx, output[], maxlength)` (`GetRejectedHit`) | Texto de um hit rejeitado (últimos 15) |
| `WF_AverageShootRate(playerid, shots, &multiple_weapons = 0)` / `WF_AverageHitRate` | Cadência média |
| `WF_SetKnifeSync(toggle)` | Faca instantânea (padrão: desligada com lag compensation) |
| `WF_SetDisableSyncBugs(toggle)` | Correções de sync ("ghost shooting", detonador, óculos, mira) |
| `WF_SetLagCompMode(mode)` / `WF_GetLagCompMode()` | Lido do servidor automaticamente |
| `WF_SetForwardDamageCallbacks(bool:toggle)` | Ainda chama `OnPlayerGiveDamage`/`OnPlayerTakeDamage` nos scripts |

### Ambiente

| Função | Descrição |
|---|---|
| `WF_SetCustomFallDamage(bool:toggle, Float:damage_multiplier = 25.0, Float:death_velocity = -0.6)` | Dano de queda pela velocidade |
| `WF_SetCustomVendingMachines(bool:toggle)` | Máquinas de venda server-side (padrão: ligado) |
| `WF_SetVehiclePassengerDamage(bool:toggle)` / `WF_SetVehicleUnoccupiedDamage(bool:toggle)` | Dano em veículos sem motorista |
| `WF_SetDamageSounds(taken, given)` | Sons de dano (0 desliga) |

### Feed de dano e barra de vida

| Função | Descrição |
|---|---|
| `WF_SetDamageFeed(bool:toggle)` / `WF_SetDamageFeedForPlayer(playerid, toggle = -1)` / `WF_IsDamageFeedActive` | Liga/desliga o feed |
| `WF_SetDamageFeedHideDelay(ms)` / `WF_SetDamageFeedColors(given, taken)` / `WF_SetDamageFeedPosition(Float:gx, Float:gy, Float:tx, Float:ty)` | **Novos.** Personalização do feed |
| `WF_EnableHealthBarForPlayer(playerid, bool:enable)` | Mostra/esconde a barra |
| `WF_SetHealthBarPosition` / `Size` / `Padding` / `Color` | Global |
| `WF_SetHealthBarPosForPlayer` (`SetHealthBarPositionForPlayer`) / `SizeForPlayer` / `PaddingForPlayer` / `ColorForPlayer` | Por jogador |
| `WF_GetHealthBarPosition` / `Size` / `Padding` / `Color` | Leitura |

### Diversos

`WF_SetDebug(level)`, `WF_IsOpenMP()`, `WF_IsRakNetActive()` (pacotes de sync sendo processados), `WF_GetVersion(version[], len)`.

## Exemplo

```pawn
#include <open.mp>
#include <weapon-forge>

public OnGameModeInit()
{
    SetWeaponDamage(WEAPON_DEAGLE, DAMAGE_TYPE_STATIC, 50.0);
    WF_SetWeaponBodypartMultiplier(-1, 9, 1.5);   // headshot +50%
    SetCbugAllowed(false);
    SetRespawnTime(4000);
    WF_SetSpawnProtection(3000);
    WF_SetHealthRegen(1.0, 10000);
    return 1;
}

public OnPlayerDamage(&playerid, &Float:amount, &issuerid, &WEAPON:weapon, &bodypart)
{
    if (weapon == WEAPON_SNIPER && bodypart == 9) {
        amount = 1000.0; // sniper na cabeça mata
    }
    return 1;
}

public OnPlayerDeathFinished(playerid, bool:cancelable)
{
    // Sistema de "ferido": não respawna, o jogador fica caído até ser socorrido
    if (cancelable && IsPlayerInHospitalZone(playerid)) {
        return 0;
    }
    return 1;
}

public OnPlayerKillAssist(playerid, victimid, killerid, Float:damage)
{
    GivePlayerMoney(playerid, 50);
    return 1;
}
```

## Compilando

O `build.ps1` compila **as duas plataformas** a partir do Windows: o DLL do Windows com Visual Studio, e o `.so` do Linux por compilação cruzada com o [Zig](https://ziglang.org). O script baixa o Zig sozinho, então não precisa de WSL, Docker nem máquina Linux.

Requisitos: Visual Studio 2019/2022 (ou Build Tools) com "Desenvolvimento para desktop com C++", CMake (o do Visual Studio serve) e git (para baixar o SDK do open.mp uma vez).

```powershell
.uild.ps1                          # Windows + Linux -> dist.uild.ps1 -Platform Linux          # só o weapon-forge.so
.uild.ps1 -Platform Windows
.uild.ps1 -Clean -Package          # build limpo + .zip/.tar.gz em dist.uild.ps1 -Version 1.2.0 -Package
```

A saída vai para `dist/plugins/` (SA-MP) e `dist/components/` (open.mp), mais `dist/include/weapon-forge.inc`. Se o PowerShell bloquear o script: `powershell -ExecutionPolicy Bypass -File .uild.ps1`.

O `.so` é 32 bits, exige glibc 2.17+ e tem o runtime C++ embutido; o `tools/elfcheck.py` confere isso a cada build. O `build.ps1 -Platform Linux` também roda no Linux com PowerShell (`pwsh`). Como alternativa, o `build.sh` compila com o g++ e o CMake do sistema (`sudo apt install cmake git g++-multilib`).

## CI e releases (GitHub Actions)

O workflow `.github/workflows/build.yml` roda **apenas** em pull requests e quando você o executa manualmente:

- compila Windows e Linux com o `build.ps1` e confere os binários (exports, formato ELF, versão da glibc);
- compila os scripts de teste contra os includes do SA-MP e do open.mp (qualquer warning falha o build);
- publica os binários como artifacts (o download já traz `plugins/`, `components/` e `include/`, sem zip dentro).

Para criar uma release: **Actions → Build → Run workflow**, marque **"Create a GitHub release with the binaries"** e informe a versão (vazio = arquivo `VERSION`). O workflow cria a tag `vX.Y.Z` e anexa `weapon-forge-X.Y.Z-windows.zip`, `weapon-forge-X.Y.Z-linux.tar.gz` e `weapon-forge.inc`.

A wiki fica em [`wiki/`](wiki) e é publicada na wiki do GitHub por **Actions → Wiki → Run workflow** (veja [`wiki/README.md`](wiki/README.md)).

## Testes

- `tests/gamemode.pwn` e `tests/filterscript.pwn`: testes de compilação (usados no CI).
- `tests/runtime.pwn`: teste em servidor real. Conecta um NPC e executa o pipeline: dano, colete, god mode, multiplicadores, `SetPlayerHealth`, vida falsa nos pacotes de sync, morte, `OnPlayerDeath` emitido, respawn e estatísticas. Imprime `[rt] PASS/FAIL` e desliga o servidor. No SA-MP precisa de `maxnpc 1` e de `tests/npc_idle.pwn` compilado em `npcmodes/`; no open.mp, de `"max_bots": 1`.

## Estrutura

```
include/weapon-forge.inc   declarações (natives, constantes, callbacks)
src/main.cpp               entrada como plugin SA-MP + inicialização comum
src/omp_component.cpp      entrada como component do open.mp
src/amxhooks.cpp           hooks de amx_Exec/amx_FindPublic/amx_Register e despacho de eventos
src/hook.cpp               detour x86 (Windows/Linux)
src/net_samp.cpp           pacotes de sync no SA-MP (RakServer)
src/sync.cpp               alteração dos pacotes de sync
src/combat.cpp             validação e aplicação de dano, tiros, hits rejeitados
src/death.cpp              morte, animações, OnPlayerDeath, respawn
src/events.cpp             eventos de jogador/veículo, timers, regen/sangramento
src/hud.cpp                barra de vida e feed de dano (textdraws)
src/vending.cpp            máquinas de venda
src/natives.cpp            natives do Pawn e substituições das natives do servidor
src/scripts.cpp            chamadas de callbacks Pawn
src/server.cpp             chamadas das natives do servidor
src/sdk/                   definições mínimas do SDK de plugins SA-MP
tools/elfcheck.py          validação do .so do Linux
wiki/                      páginas da wiki do GitHub
```

## Créditos e licenças

- Baseado no [weapon-config](https://github.com/oscar-broman/samp-weapon-config) de Oscar Broman e colaboradores (Apache License 2.0). As regras de validação, valores padrão das armas e dados das máquinas de venda vêm dele.
- Os índices de métodos do RakServer do SA-MP e o layout de `Packet` seguem o [Pawn.RakNet](https://github.com/katursis/Pawn.RakNet) (MIT).
- O [SDK do open.mp](https://github.com/openmultiplayer/open.mp-sdk) (MPL-2.0) é baixado no build.

Veja `NOTICE` e `third_party/`.
