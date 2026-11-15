# Crash saat NPC (FCNPC) spawn & konflik dengan streamer

Status: Draft · Date: 2026-11-15

> **Update 2026-11-15 — akar masalah terkonfirmasi & diperbaiki** (tes di salinan `D:\SAMPSERVER\Server`, open.mp 1.6.7, streamer 2.9.6).
> Crash juga terjadi pada pemain asli, bukan hanya NPC. Minidump: `0xC0000005` di `streamer.dll+0x3f3ed` dengan `edi = 0x7F000005` (indeks palsu weapon-forge untuk `OnPlayerRequestClass`). sampgdk di dalam streamer membaca tabel public `publics + index*8` tanpa cek batas untuk setiap indeks `> -10000`.
> Perbaikan di `src/amxhooks.cpp`:
> 1. `kFakeIndex` sekarang `-0x20000000`. sampgdk memperlakukan indeks `<= -10000` dengan aman.
> 2. Kalau ada plugin yang meng-hook `amx_Exec`/`amx_FindPublic` di atas weapon-forge, `Relayer()` (dipanggil saat script pertama dimuat) memasang lapisan kedua di atasnya. Indeks palsu milik sampgdk dipetakan ke callback lewat nama (`g_foreign`), jadi streamer tetap menerima callback dan tidak ada lagi spam `Index mismatch`.
>
> Hasil: build lama crash (exit 139), build baru keluar normal (exit 0) dengan gamemode yang sama + NPC, tanpa peringatan sampgdk. H1 (NPC/FCNPC) tidak terbukti sebagai penyebab crash. Sisanya tetap jadi pekerjaan terpisah.

## Problem

Pemilik server melaporkan dua masalah di **open.mp (weapon-forge sebagai component)**, dengan **streamer** dan **FCNPC** terpasang sebagai legacy plugin (urutan di config: streamer lalu weapon-forge):

1. **Server crash saat NPC FCNPC spawn.** NPC dipakai sebagai stand-in di dalam game.
2. **"Bentrok" dengan streamer.** Gejala persisnya belum diketahui (lihat Open questions).

Belum ada crash log, jadi penyebab pastinya **belum terkonfirmasi**. Bacaan kode memberi hipotesis berikut, diurutkan dari yang paling mungkin:

### H1: weapon-forge memperlakukan NPC seperti client asli (crash saat spawn)

`p.npc` sudah diisi (`src/events.cpp:308`), tetapi hanya dipakai di jalur damage/death. Saat NPC connect atau spawn, plugin tetap mengirim aksi yang hanya berguna untuk client sungguhan:

| Kapan | Panggilan | Lokasi |
|---|---|---|
| Connect | `RemoveBuildingForPlayer` ×N (vending machine) | `src/events.cpp:323` → `src/vending.cpp:144,150` |
| Spawn | `SetPlayerHealth(8000000 + x)` | `src/events.cpp:416` → `src/hud.cpp:214` |
| Spawn | `CreatePlayerTextDraw` / `PlayerTextDrawShow` (health bar, damage feed) | `src/hud.cpp:38,49,237,351` |
| Spawn pertama | `ApplyAnimation("VENDING", "null")` | `src/events.cpp:429` |
| Class selection | `RemoveBuildingForPlayer` ×3 | `src/events.cpp:458-460` |
| Mati | `SpawnPlayer` (respawn otomatis NPC) | `src/death.cpp:433-436` |

FCNPC dibuat untuk internal server SA-MP. Di open.mp, NPC FCNPC kemungkinan **tidak punya peer jaringan yang valid**, jadi RPC per-player seperti textdraw, remove building, atau animasi yang dikirim ke NPC itu adalah titik crash yang paling mungkin. Waktunya juga cocok: semua panggilan di atas terjadi tepat saat `OnPlayerSpawn`. Selain itu, `SpawnPlayer` bukan cara yang benar untuk me-respawn NPC FCNPC (seharusnya `FCNPC_Respawn`). Kasusnya sama dengan `NPC_Create` di open.mp yang sudah dicatat di `wiki/Migrating-from-weapon-config.md#openmp-npcs`.

### H2: dua plugin meng-hook fungsi AMX yang sama (konflik streamer)

weapon-forge memasang inline hook x86 buatan sendiri (`src/hook.cpp`) pada `amx_Register`, `amx_FindPublic` dan `amx_Exec` (`src/amxhooks.cpp`). Streamer 2.9.x membawa sampgdk, yang (perlu diverifikasi) juga meng-hook `amx_FindPublic`/`amx_Exec` untuk menerima callback seperti `OnPlayerConnect`. Titik rawannya:

- **Urutan hook di open.mp tidak bisa diatur dari config.** Component (`onInit`, `src/omp_component.cpp`) selalu memasang hook sebelum legacy plugin dimuat, jadi weapon-forge selalu berada di lapisan dalam. Urutan "streamer lalu weapon-forge" di config tidak berpengaruh.
- **Indeks public palsu.** `FindDetour` mengembalikan `kFakeIndex + cb` (`src/amxhooks.cpp:29,542`) untuk callback yang tidak ada di gamemode. Hook sampgdk di luar yang mencoba `amx_GetPublic(index)` akan gagal, sehingga streamer bisa **tidak menerima** callback itu (misalnya `OnPlayerConnect`/`OnPlayerDisconnect` kalau gamemode tidak mengimplementasikannya). Akibatnya objek atau pickup tidak muncul, atau data player di streamer basi saat id dipakai ulang oleh NPC.
- **`Hook::Remove` menimpa hook plugin lain.** `Unpatch()` (`src/hook.cpp:272`) menulis ulang byte asli yang disalin saat install, tanpa mengecek apakah prolog masih berisi jump milik kita. Kalau plugin lain sudah meng-hook di atasnya, saat shutdown hook mereka terhapus atau mengarah ke modul yang sudah di-unload, dan server crash saat ditutup.
- **Mode fallback "toggle"** (`src/hook.cpp:218`, `Hook::Unpatched` di `:296`) tidak re-entrant. Selama callback berjalan, panggilan `amx_Exec` bersarang (dari streamer, atau dari `CallLocalFunction`) melewati hook, dan `g_internal_exec` bisa tertinggal. Mode ini aktif kalau log menampilkan `WARN: using fallback hook mode`.
- **Callback yang di-Skip tidak terlihat oleh hook di dalam kita.** Contohnya `OnPlayerGiveDamage`/`OnPlayerTakeDamage` saat `forward_damage_callbacks` mati (`src/amxhooks.cpp:376,387`). Di open.mp ini tidak berdampak ke streamer karena weapon-forge ada di dalam, tetapi di SA-MP bisa berdampak tergantung urutan load.

## Goals

- NPC FCNPC bisa spawn, menerima damage, mati dan respawn di open.mp + weapon-forge component + streamer **tanpa crash** (dites minimal 100 siklus spawn→mati→respawn).
- weapon-forge **tidak mengirim aksi khusus client** (textdraw, remove building, animasi vending, suara) ke player yang `IsPlayerNPC` bernilai true.
- Dengan streamer terpasang, callback streamer (`OnPlayerPickUpDynamicPickup`, `OnPlayerEnterDynamicCP`, streaming objek untuk player baru) tetap berjalan, baik gamemode mengimplementasikan `OnPlayerConnect` dan kawan-kawan atau tidak.
- Shutdown server dan `gmx` dengan streamer terpasang tidak crash.
- Akar masalah H1 dan H2 dikonfirmasi atau ditolak dengan log, bukan tebakan.

## Non-goals

- Mendukung FCNPC secara penuh: sinkronisasi health internal FCNPC, `FCNPC_OnTakeDamage`, dan sejenisnya. weapon-forge hanya berhenti merusak NPC.
- Mengganti mesin hook AMX dengan event handler native open.mp (`PlayerSpawnEventHandler` dll.). Itu redesign besar, dicatat sebagai opsi di Open questions.
- Menambah library hook eksternal (subhook dan sejenisnya).

## API

Perbaikan minimal **tidak menambah native atau callback**. Perilaku NPC berubah secara internal.

Opsional, hanya jika jawaban Open question #3 membutuhkannya (`WF_*` saja, tidak ada padanan di weapon-config):

```pawn
// Default: true. SA-MP ConnectNPC di-respawn plugin (perilaku sekarang).
// false: plugin tidak memanggil SpawnPlayer untuk NPC; script me-respawn
// sendiri di OnPlayerDeathFinished (FCNPC_Respawn / NPC_Respawn).
native WF_SetNPCAutoRespawn(bool:toggle);
```

## Behaviour

- **NPC (`p.npc == true`), dilewati:** pembuatan, update dan show health bar (`hud.cpp`), damage feed, `RemoveDefaultVendingMachines`, `ApplyAnimation("VENDING", ...)`, `RemoveBuildingForPlayer` di class selection, `PlayerPlaySound` yang ditujukan ke NPC. Cukup satu guard di tiap fungsi entry, bukan di setiap pemanggil.
- **NPC, tetap jalan:** state health/armour internal, damage, death, `OnPlayerDeath`/`OnPlayerDeathFinished`, statistik. `SetPlayerHealth(8000000 + x)` perlu dicek ulang untuk NPC (Open question #2).
- **Respawn NPC:** tetap `SpawnPlayer` secara default (perilaku sekarang). Untuk FCNPC, dokumentasikan agar memanggil `FCNPC_Respawn` di `OnPlayerDeathFinished`, sama seperti contoh `NPC_Respawn`.
- **`p.npc` harus benar** sebelum aksi pertama, termasuk saat plugin dimuat di tengah jalan (`ScriptInit`, `src/events.cpp:169`) dan saat FCNPC memanggil `OnPlayerConnect`.
- **Hook AMX:**
  - `Hook::Remove` hanya memulihkan byte asli kalau 5 byte pertama target masih berupa jump milik kita. Kalau tidak, biarkan dan tulis log.
  - `FindDetour` tidak lagi memalsukan indeks untuk callback yang juga dicegat plugin lain. Cara pastinya menunggu hasil verifikasi H2; kandidatnya: pastikan public palsu tetap bisa di-resolve namanya, atau dokumentasikan bahwa gamemode wajib mengimplementasikan callback tersebut.
- **Disconnect, `gmx`, filterscript:** tidak berubah. Guard NPC berlaku sama untuk gamemode dan filterscript karena logikanya ada di C++.
- **Id tidak valid:** tetap ditolak oleh `ValidPlayer` (`src/state.hpp:344`).

## Server matrix

| | SA-MP 0.3.7 | SA-MP 0.3.DL | open.mp component | open.mp legacy plugin |
|---|---|---|---|---|
| Guard NPC (tanpa HUD/RPC client) | Ya | Ya | Ya | Ya |
| ConnectNPC (npcmodes) | Jalan, respawn otomatis | Jalan, respawn otomatis | N/A | N/A |
| FCNPC | Jalan (FCNPC dibuat untuk ini) | Perlu dicek | **Kasus pelapor**. Dukungan FCNPC di open.mp sendiri belum pasti | Sama |
| `NPC_Create` open.mp | N/A | N/A | Jalan, respawn via `NPC_Respawn` | Jalan, tanpa fitur sync |
| Urutan hook vs streamer | Ikut urutan `plugins` | Ikut urutan `plugins` | weapon-forge selalu di dalam | Ikut urutan `legacy_plugins` |
| Fix `Hook::Remove` | Ya | Ya | Ya | Ya |

## Compatibility

- Tidak ada perubahan API. Migrasi dari weapon-config tidak terpengaruh.
- Perubahan yang terlihat: NPC tidak lagi punya textdraw health bar/damage feed. Itu memang tidak pernah terlihat siapa pun, jadi tidak breaking.
- Kalau nanti `WF_SetNPCAutoRespawn` ditambahkan, default `true` menjaga perilaku sekarang.

## Performance

Guard NPC berupa satu cek `bool` per panggilan HUD/vending, nol biaya per paket sync. Bahkan sedikit lebih hemat karena NPC tidak lagi membuat textdraw. Tidak ada timer baru.

## Testing

1. **Reproduksi dulu (wajib sebelum fix H2):** server open.mp + weapon-forge component + streamer + FCNPC, gamemode minimal yang membuat 1 FCNPC lalu `FCNPC_Spawn`. Kumpulkan log server lengkap dan crash dump (minidump/`crashinfo`). Ulangi tanpa streamer, lalu tanpa weapon-forge, untuk mengisolasi pihak yang crash. Ulangi dengan `NPC_Create` sebagai pengganti FCNPC.
2. `tests/runtime.pwn`: setelah NPC spawn, cek `WF_IsDamageFeedActive(g_Npc) == 0` dan health bar NPC tidak dibuat. Jalankan suite di SA-MP dan open.mp **dengan streamer ikut dimuat**.
3. Tes stres manual: 100× siklus spawn → `WF_KillPlayer` → respawn untuk NPC, lalu `gmx`, lalu shutdown. Hasilnya harus tanpa crash dan tanpa `WARN: using fallback hook mode`.
4. Streamer: `CreateDynamicPickup`/`CreateDynamicCP` dengan gamemode yang **tidak** punya `OnPlayerConnect`. Pickup harus muncul dan callback dynamic harus terpanggil.

## Docs

- `wiki/FAQ.md`: entri "Server crash saat NPC FCNPC spawn" dan "Dipakai bersama streamer".
- `wiki/Migrating-from-weapon-config.md#openmp-npcs`: tambah contoh `FCNPC_Respawn`.
- `wiki/How-It-Works.md`: jelaskan urutan hook AMX vs plugin lain dan bahwa NPC tidak mendapat HUD.
- `wiki/Natives.md` + `wiki/Natives-Miscellaneous.md`: hanya kalau `WF_SetNPCAutoRespawn` jadi ditambahkan.
- `README.md` dan `README.pt-BR.md`: baris NPC di bagian catatan (pt-BR baris 68) ditambah FCNPC.

## Open questions

1. **Crash log:** mohon kirim `log.txt` open.mp lengkap dan crash dump / output crashdetect saat NPC spawn. Tanpa ini, H1 vs H2 vs "FCNPC sendiri tidak kompatibel dengan open.mp" belum bisa dibedakan.
2. **Versi FCNPC & open.mp:** FCNPC mana yang dipakai di open.mp (rilis resmi atau fork)? Apakah server tetap crash kalau weapon-forge dilepas tapi FCNPC tetap dipasang? Kalau ya, masalahnya ada di FCNPC dan solusinya pindah ke `NPC_Create` open.mp.
3. **Respawn NPC:** apakah NPC stand-in di-respawn oleh script sendiri (`FCNPC_Respawn`)? Kalau ya, perlu `WF_SetNPCAutoRespawn(false)` agar plugin tidak ikut memanggil `SpawnPlayer`.
4. **Gejala "bentrok streamer":** apa yang terjadi persisnya? Crash (kapan), objek/pickup tidak muncul, callback dynamic tidak terpanggil, atau error di log?
5. **Versi streamer:** dibutuhkan untuk memastikan apakah build tersebut memakai sampgdk (dasar H2).
6. **Jangka panjang:** untuk open.mp component, apakah layak memindahkan pemrosesan callback dari hook AMX ke event handler native open.mp? Itu akan menghapus seluruh kelas konflik hook, tetapi skalanya besar dan perlu PRD terpisah.
