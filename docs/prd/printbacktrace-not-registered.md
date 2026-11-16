# Error "PrintBacktrace is not registered": crashdetect gagal load karena weapon-forge

Status: Implemented (1.1.4) · Date: 2026-11-16

> **Implementasi 2026-11-16:** `amxhooks::Install` hanya memasang hook `amx_Register`. `amxhooks::Relayer()` (dipanggil di `AmxLoad`) memasang `amx_Exec`/`amx_FindPublic` saat script pertama dimuat. Kalau salah satunya gagal, keduanya dilepas.
> Tes di salinan `D:\SAMPSERVER\Server` (open.mp, crashdetect 4.19.4, streamer):
> - `crashdetect, streamer` + `compat.pwn` dengan `PrintBacktrace()`: crashdetect ter-load, backtrace tercetak, `[compat] DONE fails=0`, exit 0.
> - DLL lama dengan setup yang sama: `CrashDetect must be loaded before 'weapon-forge.dll'` dan `Function not registered: PrintBacktrace`.
> - `streamer` saja: `fails=0`.
> - `streamer, crashdetect`: `fails=0`. crashdetect menolak karena streamer, dan ini perilaku bawaan crashdetect, bukan dari weapon-forge.
>
> Ternyata hook `amx_Register` tidak diperiksa oleh crashdetect (open question pertama terjawab). Layering `Relayer()` masih dipertahankan dan belum dihapus.

> **Update 2026-11-16: akar masalah terkonfirmasi** dari log server pengguna (`log.txt`, open.mp 1.5.8, weapon-forge 1.1.3 sebagai component, crashdetect 4.19.4):
>
> ```
> [weapon-forge] v1.1.3 loaded (open.mp component)
> Loading plugin: crashdetect
>   CrashDetect must be loaded before 'weapon-forge.dll'
> Plugin failed to initialize.
> ...
> [Error] Function not registered: PrintBacktrace
> [Error] File or function is not found
> ```
>
> Sebelum weapon-forge dipasang, gamemode yang sama jalan normal dengan crashdetect (`CrashDetect plugin 4.19.4`, tanpa error).
> Hipotesis versi sebelumnya ("crashdetect tidak ada di config") **salah** untuk kasus ini: crashdetect ada di config, tetapi **menolak inisialisasi karena weapon-forge**.

## Problem

Pemilik server yang memakai **crashdetect** (dan gamemode yang di-compile dengan `crashdetect.inc`, misalnya lewat YSI) tidak bisa menjalankan gamemode setelah memasang weapon-forge. Error yang muncul: `Function not registered: PrintBacktrace`, dan script tidak jalan.

**Penyebab:**

1. weapon-forge memasang hook `jmp` di awal `amx_Exec`, `amx_FindPublic`, dan `amx_Register` saat `Startup()` (`src/main.cpp:131` → `amxhooks::Install`, `src/amxhooks.cpp:642`).
   - Di open.mp, `Startup()` dipanggil dari `onInit` component (`src/omp_component.cpp:76`), yang selalu jalan **sebelum** legacy plugin di-load.
2. Saat `Load()`, crashdetect memeriksa apakah `amx_Exec` sudah diawali `jmp` ke modul lain. Kalau iya, crashdetect mencetak `CrashDetect must be loaded before '<modul>'` dan mengembalikan `false`. Akibatnya native crashdetect tidak pernah terdaftar.
3. Gamemode mereferensikan `PrintBacktrace`, jadi open.mp menolak menjalankan script.

**Di open.mp, pengguna tidak bisa mengatasinya dengan mengubah urutan load**, karena component selalu di-load sebelum legacy plugin. Di SA-MP, masalah yang sama muncul kalau `weapon-forge` ditulis sebelum `crashdetect` di baris `plugins`.

## Goals

- crashdetect dan weapon-forge jalan bersamaan, di semua mode server, tanpa bergantung pada urutan load.
- Semua fitur weapon-forge tetap jalan: callback dari `amx_Exec`, callback palsu dari `amx_FindPublic`, dan override native dari `amx_Register`.
- Tidak ada regresi pada perbaikan streamer/sampgdk (`docs/prd/npc-crash-dan-konflik-streamer.md`).

## Non-goals

- Menyamarkan bentuk hook (misalnya `push/ret` sebagai pengganti `jmp`) supaya lolos dari pemeriksaan crashdetect. crashdetect perlu menjadi hook terdalam supaya bisa melaporkan error AMX dengan benar.
- Menyediakan stub `PrintBacktrace` di weapon-forge.
- Kompatibilitas dengan JIT. Itu pekerjaan terpisah.

## API

N/A: tidak ada perubahan pada native, callback, atau konstanta. Perubahan hanya ada di waktu pemasangan hook internal.

## Behaviour

**Usulan: pasang hook `amx_Exec` dan `amx_FindPublic` saat script pertama di-load, bukan saat `Startup()`.**

- `Startup()` hanya memasang hook `amx_Register`.
  - Hook ini harus dipasang lebih awal karena native server didaftarkan saat script dimuat.
  - crashdetect hanya memeriksa `amx_Exec`, jadi hook Register tidak memicu penolakan (lihat Open questions).
- `wf::AmxLoad()` (`src/main.cpp:163`) memasang hook Exec/FindPublic pada panggilan pertama, sebelum `Relayer()`. Saat itu semua plugin sudah di-load ("Plugins are all loaded before the first script"), sehingga:
  - crashdetect dan sampgdk/streamer sudah memasang `jmp`-nya. Hook kita dipasang di atasnya dan trampoline menyalin `jmp rel32` mereka (sudah ditangani di `Hook::Install`, `src/hook.cpp:232`).
  - weapon-forge otomatis menjadi hook paling luar, sehingga layering `Relayer()` / `g_exec_top` / `g_find_top` / `g_foreign` kemungkinan tidak diperlukan lagi. Kode itu sebaiknya dihapus kalau tes streamer tetap lulus. Kalau ragu, biarkan dulu.
- Kalau pemasangan tertunda gagal: tulis log `ERROR: could not hook amx_Exec/amx_FindPublic` satu kali. Plugin tidak boleh crash. Native tetap terdaftar, tetapi fitur callback mati.
- `amxhooks::Exec`/`FindPublic` (`src/amxhooks.cpp:688,702`) sudah punya jalur `!Installed()` yang langsung ke fungsi asli, jadi aman dipanggil sebelum hook terpasang.
- `gmx` dan filterscript: pemasangan hanya sekali (flag), dan `AmxLoad` berikutnya tidak memasang ulang.
- `Shutdown()`/`Remove()` tetap melepas hook dengan urutan terbalik.

## Server matrix

| | SA-MP 0.3.7 | SA-MP 0.3.DL | open.mp component | open.mp legacy plugin |
|---|---|---|---|---|
| Sekarang | Error kalau weapon-forge ditulis sebelum crashdetect | sama | **selalu error** | error kalau weapon-forge sebelum crashdetect di `legacy_plugins` |
| Setelah fix | jalan, urutan bebas | sama | jalan | jalan, urutan bebas |

## Compatibility

- Tidak ada perubahan API dan tidak perlu migrasi script.
- **Workaround sampai fix rilis** (untuk ditulis di FAQ), pilih salah satu:
  1. Pakai weapon-forge sebagai **legacy plugin**: pindahkan ke `plugins/`, lalu tulis setelah `crashdetect` di `legacy_plugins`. Fitur sync (health nametag, sync fix) mati.
  2. Lepas crashdetect, lalu compile ulang gamemode tanpa `crashdetect.inc` di path include.

## Performance

Tidak berubah. Pemasangan hook hanya terjadi sekali saat script pertama dimuat.

## Testing

- **Manual (wajib):** di server pengguna (open.mp 1.5.8, crashdetect 4.19.4, streamer 2.9.4, mysql, sscanf, gvar, pawn-memory, audio, discord-connector), dengan weapon-forge di `components/`:
  1. Log tidak lagi memuat `CrashDetect must be loaded before`, dan memuat `CrashDetect plugin 4.19.4`.
  2. Tidak ada `Function not registered`, dan gamemode jalan.
  3. Error runtime di gamemode tetap memunculkan backtrace crashdetect. Ini membuktikan hook crashdetect aktif di bawah weapon-forge.
  4. Damage, death, dan callback streamer tetap normal (regresi untuk `npc-crash-dan-konflik-streamer`).
- **SA-MP 0.3.7:** `plugins weapon-forge crashdetect streamer` (urutan "salah") harus jalan.
- **`tests/compat.pwn`:** panggil `IsCrashDetectPresent()` (via `#tryinclude <crashdetect>`), lalu pastikan bernilai true saat crashdetect di-load bersama weapon-forge.

## Docs

- `wiki/FAQ.md`: entri "`CrashDetect must be loaded before 'weapon-forge'` / `PrintBacktrace` not registered", berisi penyebab, versi yang memperbaiki, dan workaround di atas.
- `wiki/How-It-Works.md`: paragraf hook AMX, dengan catatan bahwa Exec/FindPublic dipasang saat script pertama dimuat.
- `wiki/Installation.md`: catatan kompatibilitas crashdetect.
- Changelog rilis berikutnya (versi patch).

## Open questions

- Apakah crashdetect 4.19.x/4.22 hanya memeriksa `amx_Exec`, atau juga `amx_Register`/`amx_FindPublic`? Kalau `amx_Register` ikut diperiksa, hook Register harus dicari jalan lain (misalnya menangkap native server lewat `onAmxLoad` setelah pendaftaran). Ini perlu dicek di source crashdetect atau langsung diuji.
- Apakah hook Register memang harus dipasang sebelum script pertama di open.mp? Dengan kata lain, apakah open.mp mendaftarkan native-nya sebelum atau sesudah `onAmxLoad`? Kalau sesudah, Register juga bisa ditunda dan masalahnya hilang sepenuhnya.
- Hapus layering `Relayer()` sekarang, atau di rilis berikutnya setelah fix ini terbukti di server nyata?
