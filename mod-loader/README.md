# UYA Mod Loader

Shared campaign/multiplayer loader, based on `sm-cheats/module-loader`, without
its heap edits. The same optional exception-handler payload is included. `main.c` contains the
commented loading, validation, and dispatch flow. This folder is independent of
`campaign/` and `multiplayer/`.

## Enable or disable modules

The host filesystem layout keeps the editable list at the root of `uya/`:

```text
hostfs/uya/
  module.txt
  mods/
    debug/exceptionhandler.bin
    campaign-test.ntscu.bin
    campaign-test.pal.bin
    campaign-test.ntscj.bin
    multiplayer-test.ntscu.bin
    multiplayer-test.pal.bin
```

The packaged `release/hostfs/uya/` uses the same layout. Entries in the list stay
as a base name such as `campaign-test`; the loader adds the running region and
`.bin` automatically. Explicit region suffixes are also supported.

Edit **`hostfs/uya/module.txt`**:

```text
// 0 = disabled, 1 = singleplayer, 2 = multiplayer, 3 = both
hello                      3 // both
campaign-test              1 // singleplayer
multiplayer-test           2 // multiplayer
disabled-mod               0 // do not load
```

Each entry requires a path and exactly one digit from `0` through `3`.
Blank lines, `//` comments (full-line or trailing), tabs, and Windows line endings
are accepted. Full-line `#` comments are also accepted for compatibility, but old
bare-path entries must be updated to include a mode. Malformed entries are logged
and skipped. Names cannot contain whitespace. Binaries live directly in
`host0:uya/mods/`. There is one shared list, with a maximum of eight loaded
module images and 2047 bytes of list text.

```text
campaign-test           1 // automatic region: campaign-test.<region>.bin
campaign-test.ntscu     1 // NTSC-U only
campaign-test.pal.bin   1 // PAL only; .bin is optional
campaign-test.ntscj     0 // disabled
```

Use either an automatic entry or individual regional entries for a given mod.
An explicit entry for another region is skipped without opening a file. A `0`
entry does not override an enabled automatic entry. If enabled entries resolve
to the same binary, only the first successfully loaded entry applies (including
its mode flags). The binary's own region and supported-mode checks still apply.
An optional `modules/` prefix is accepted for old lists, but no directory is
created for it. Names and suffixes are lowercase; unknown suffixes are rejected.

Disabled entries do not open files or reserve runtime module slots.

The selected modes must be supported by the binary's header. A both-mode binary
can be restricted to `1` or `2`. A singleplayer-only binary set to `2` or `3` is
rejected, not promoted into a multiplayer module. The same applies in reverse.
`3` covers singleplayer (including the initial main menu) and multiplayer.

Like the SM loader, this reads the list once per boot. **Restart emulation after
changing it.** This is not live unloading: a module may have patched game code,
so stopping its callback would not undo its effects. A missing/invalid list is
retried every 300 hook calls. A missing/invalid individual module is logged and
skipped. Restart after repairing one.

## Create a module

From this directory, with Python 3.8 or newer:

```sh
python tools/build.py new my-mod
python tools/build.py new campaign-mod --library librac3
python tools/build.py new multiplayer-mod --library libuya
python tools/build.py new shared-mod --library both
```

Or use `make new-module NAME=my-mod LIBRARY=none` (`PYTHON=python` on systems
without a `python3` command).

Each command creates `modules/<name>/module.json` and commented starter C code,
and appends entries set to **`0` (disabled)** to `module.txt`, with comments
showing the appropriate enable value. Existing folders are never
overwritten. The `hello` sample is enabled by default and only prints a message.

- `none`: one `main.c`, usable in both modes through the loader API.
- `librac3`: campaign-only `main.c`, with `<librac3/...h>` headers available.
- `libuya`: multiplayer-only `main.c`, with `<libuya/...h>` headers available.
- `both`: **one folder**, with `campaign.c` and `multiplayer.c`. These become
  `<name>-campaign.<region>.bin` and `<name>-multiplayer.<region>.bin`. Set the campaign entry to `1` and the multiplayer entry to `2`.
  Each image links the correct library, avoiding collisions between identically
  named library functions and types. Do not link both archives into one image.

For a one-image module, `module.json` can specify `sources` (a list of relative
`.c` / `.S` paths), `mode` (`campaign`, `multiplayer`, or `both`), and `regions`.
For a `both` library module, use `campaign_sources` and `multiplayer_sources`
to add more files to each image. The generated multiplayer callback includes
`uyaPreUpdate()` / `uyaPostUpdate()` to maintain library input state.

`moduleInit(const ModuleApi *)` runs once, the first time its mode is active.
A negative result disables its update callback while retaining its memory.
`moduleUpdate(void)` runs on subsequent matching return hooks, including the first
one after initialization. Hooks can run more than once per video frame. Check
level/player readiness before using game APIs; do not retain pointers into a
previous level. Module BSS is zeroed at load time and persists across mode changes.
The supplied API pointer remains valid and is updated when the mode changes.
Use `api->print/open/read/close/flushCache` for mode-correct resident SDK services.

## Build

Requires Python 3.8+, GNU make, and the PS2 EE toolchain. Set `PS2SDK` and put
`ee-gcc`, `ee-nm`, and `ee-objcopy` on PATH. `EE_PREFIX` overrides `ee-`.

```sh
python tools/build.py build                  # all three regions
python tools/build.py build --region ntscu   # one region
make                                        # same as all-region build
```

Modules receive automatic, nonoverlapping, 64-byte-aligned addresses. No manual
layout file or address selection is needed. All declared modules are built;
`module.txt` selects which ones load and the modes in which they run. The layout is recorded in
`build/<region>/layout.json`. Rebuild/deploy the complete module set together:
adding or resizing a module can change later modules' linked addresses.

Library defaults match this workspace:

- `LIBRAC3_DIR`: `Metroynome/librac3`
- `LIBUYA_DIR`: `Horizon/horizon-uya-patch/libuya`

Set these environment variables for another checkout. Library builds happen in
isolated per-region copies under `build/`, leaving the library sources alone.
Unused library sections are discarded so unrelated unfinished functions do not
prevent a module from linking. Actual functions you call must still be implemented.

The existing Docker image can build this from PowerShell:

```powershell
docker run --rm `
  -v D:/Repos/Metroynome:/work/Metroynome `
  -v D:/Repos/Horizon/horizon-uya-patch/libuya:/work/Horizon/horizon-uya-patch/libuya:ro `
  -w /work/Metroynome/uya-cheats/mod-loader `
  --entrypoint sh dnawrkshp/ps2dev-libdl:latest `
  -c 'apk add --no-cache python3 && make'
```

## Use in PCSX2

After a successful build:

1. Use `release/mod-loader.<region>.pnach` as the cheat contents for your game's
   CRC (PCSX2 shows it in the log). Enable cheats. Do not enable another cheat
   that occupies the loader/module memory range or replaces the same hooks.
2. Enable PCSX2 host filesystem access and point its host root at
   `release/hostfs`, so `host0:uya/module.txt` resolves to the packaged list.
3. Edit the packaged `release/hostfs/uya/module.txt` if needed, then cold-boot.
   If you rebuild, the source list in `hostfs/uya/module.txt` replaces that copy.

The loader uses host filesystem access, as the SM loader does. It is not a disc
or memory-card module installer. Switching campaign/multiplayer selects the
matching callbacks automatically. Level IDs `1..0x26` select campaign and
`0x27` and above select multiplayer (including the lobby). Campaign ID `-1`
is the initial main menu and runs campaign and both-mode modules. ID `0` and
other negative IDs defer loading/callbacks as pre-load or unknown states.
Modules must check player/level readiness before calling gameplay functions.
The hook's resident SDK layout must agree with the level classification;
no separate campaign and multiplayer loader
patches need to be enabled.

## Optional exception handler

`hostfs/uya/mods/debug/exceptionhandler.bin` is an exact copy of the SM loader's
payload (10,356 bytes). When present, the build copies it to
`release/hostfs/uya/mods/debug/exceptionhandler.bin`. The loader attempts installation
once per boot, on its first valid menu/gameplay/lobby hook, before initializing modules.
It is separate from `module.txt` and does not consume a module slot.

To disable it, remove or rename the file in the hostfs root you use and cold-boot.
To keep it disabled across builds, remove/rename the source file too; the next
build removes any stale packaged copy. Missing files simply log a message and
normal module loading continues. Restart after adding it to enable it again.

Installation requires the whole `0x000c8000?0x000ce000` range to be empty and
checks the exact file size and FNV-1a hash before executing it. Corrupt, truncated,
oversized, or unreadable payloads are skipped; occupied memory is left untouched.
The binary patches kernel exception handlers, as in SM. Installation and crash
output in UYA have not yet been tested in a live emulator.

## Regions and validation status

| Component | NTSC-U | PAL | NTSC-J |
|---|---|---|---|
| Loader + API-only module build | Passed | Passed | Passed |
| `librac3` module link test | Passed | Passed | Passed |
| Supplied `libuya` module link test | Passed | Passed | No regional library supplied |

The scaffold restricts `libuya` and `both` modules to NTSC-U/PAL by default.
NTSC-J is **not** silently compiled with NTSC-U multiplayer addresses. To add it,
supply a genuinely ported `libuya` with `Makefile.ntscj` and add `ntscj` to the
module's region list. Library-specific address accuracy remains the library's
responsibility; defining a region macro does not port a function.

Campaign SDK addresses were checked against the supplied three retail boot-core
files and an NTSC-U campaign memory image. Multiplayer SDK addresses were checked
against all 22 supplied NTSC-U/PAL multiplayer lobby/map memory dumps, including
regional level-ID globals. Campaign level globals were checked in the NTSC-U
memory dumps and matched to PAL/NTSC-J extracted level code. **NTSC-J multiplayer
runtime addresses have not been verified; its callbacks are deferred rather
than reading an NTSC-U global.** Unknown SDK layouts are rejected by the runtime
signature checks. See [ADDRESSES.md](ADDRESSES.md).

No live emulator gameplay test has been performed. The generated patches still
need cold-boot testing in campaign and multiplayer, level transitions, and mode
switches on the intended regional discs. Do not interpret successful compilation
or host tests as an in-game test result.

## Memory and tests

No heap base/size edits or heap allocation. The optional exception handler uses
its own low-memory range only when installed:

| Range | Use |
|---|---|
| `0x000c8000?0x000ce000` | Optional exception handler, 24 KiB including workspace |
| `0x000f0000?0x000f2000` | Loader, maximum 8 KiB |
| `0x000f2000?0x000fe000` | Modules, 48 KiB including BSS |
| `0x000fe000?0x00100000` | Persistent loader state |

Headers include ABI version, region, mode mask, link address, sizes, and entry
point offsets. The loader rejects invalid ranges, oversized/truncated images,
wrong regions, overlapping modules, invalid entry points, and path traversal.
Memory is cleared before loading; data/instruction caches are synchronized before
executing new code. State is outside PNACH-injected bytes so repeated cheat writes
do not reset it. This area must remain free of other injected patches.

The test commands below use local test sources, which are excluded from version
control by this project's `.gitignore`. The manifest and address checks are shipped.

```sh
python tools/build.py check
make test                         # Python scaffolder tests + native C runtime tests
python3 tests/integration.py       # requires EE compiler and both library sources
python tools/verify_addresses.py 'D:/PS2/ISOs/RAC Levels/rac3'
```

Native tests cover partial reads, BSS clearing, malformed headers, bad paths,
overlap rejection, oversized/truncated files, list parsing, and mode/reentry
checks. Integration tests actually link `padGetButton` from each library in
separate module images, including the NTSC-J campaign library.

Library-backed modules can include `<librac3/stdio.h>` or `<libuya/stdio.h>`
and call `printf` for PCSX2 console output. Campaign `printf` uses the verified
SDK console routine at `0x00121760`; multiplayer uses `0x00128c70`.
Library-free modules can use `api->print`, which follows the active mode.
