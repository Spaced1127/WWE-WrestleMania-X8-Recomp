# WWE WrestleMania X8 — GameCube static recomp

Native recompilation of the retail GameCube release of *WWE WrestleMania X8*
(`GW3E78`, USA) using the
[GameCube Static Recomp Guide](https://github.com/Spaced1127/GameCube-Static-Recomp-Guide)
pipeline: DolRecomp C backend + ModernGekko runtime.

This repository contains **no game data**. Supply your own legally dumped disc.
Disc images, extracted files, generated code, save states and builds are
git-ignored.

## Current status

| Step | State |
|---|---|
| RVZ / ISO input | native, byte-identical extraction |
| Disc / DOL / FST inventory | done |
| DOL hash verified | done |
| Static C module (MSVC) | done, boots |
| Boot → title → menu → match | done |
| In-ring gameplay | done, The Rock vs Steve Austin |
| Pause menu | done |
| Performance | **59.94 fps / speed 1.00 in-match (Clang `-O3`)** |
| PGO | optional headroom, not started |
| Branded launcher / runner | done |
| v0.1.2 release zip | done, no game data |

See [`docs/STATE.md`](docs/STATE.md) for the full inventory, evidence paths,
save-state checkpoints and open issues.

## v0.1.2 release

```powershell
tools\build_launcher.bat        # WrestleManiaX8Recomp.exe + ...Runner.exe
tools\finalize_release.ps1 -Module <gGW3E78_recomp.dll>   # folder + zip + hash
```

Artifact: `dist\WrestleManiaX8Recomp-v0.1.2-Windows-x64.zip`
SHA-256: `E22F48D2808471CAB4ED0E22C85DAF31D187264BCEA61A90FE87ADCF1D0064FB`
GitHub release: https://github.com/Spaced1127/WWE-WrestleMania-X8-Recomp/releases/tag/v0.1.2

The launcher and runner are WMX8-branded, the static Clang module is the
default core, and the generated controller profile uses the D-pad for menu
navigation (L1 + D-pad taunts). The CONTROLLER page covers Xbox, PlayStation,
Switch/Joy-Con, Steam Deck, 8BitDo, the GameCube adapter and keyboard, with
auto-detection and a manual Nintendo A/B-swap override. No game data is
included.

## Key facts

| Item | Value |
|---|---|
| Disc ID | `GW3E78` |
| DOL SHA-256 | `91af3136f2a48c18a6b96a5726d1f171c9e714bb8d4d504f835070fc230a303e` |
| Entry point | `0x800051fc` |
| Module | `gGW3E78_recomp.dll` |
| Engine | Yukes `adm` (`menu/`, `bg/`, `model/`, `motion/`, `sound/`) |

## Layout

```
tools/            disc/DOL helpers, FST check, benchmark script
work/             disc, extracted game, states, evidence (git-ignored)
build/            generated + compiled modules (git-ignored)
dist/             release folders and zips (git-ignored)
lib/              ModernGekko-Template checkout (git-ignored)
overlay/          local ModernGekko source changes (tracked)
docs/STATE.md     live project state and evidence
docs/reference/   local copy of the guide (git-ignored)
art/              SteamGridDB source art (git-ignored)
resources/        blurred launcher backdrop used by the build
```

## Build

`lib/ModernGekko-Template` is upstream runtime source and is not committed
(hundreds of MB). Clone it once:

```powershell
git clone https://github.com/ExpansionPak/ModernGekko-Template.git lib/ModernGekko-Template
cd lib/ModernGekko-Template
git submodule update --init --recursive
cd ..\..
Copy-Item overlay\ModernGekko\* lib\ModernGekko-Template\lib\ModernGekko\ -Recurse -Force
```

The overlay carries this port's launcher theme/controller changes and the
runtime `InspectGame` fix; see `overlay/README.md`.

The guide is also a local checkout:

```powershell
git clone https://github.com/Spaced1127/GameCube-Static-Recomp-Guide.git docs/reference/GameCube-Static-Recomp-Guide
```

Fetch the third-party launcher art (not committed) and build:

```powershell
python tools\fetch_steamgrid_art.py
python tools\make_steamgrid_assets.py
tools\build_launcher.bat
```

The launcher and runner names, disc ID and DOL hash are pinned in
`tools\build_launcher.bat`.

## Play

```powershell
tools\play.ps1                                   # auto-detect controller; D-pad navigates menus
tools\play.ps1 -StockDpad                        # D-pad = taunt, menus = stick only
tools\play.ps1 -Layout keyboard                  # keyboard fallback
tools\play.ps1 -State work\states\match.sav      # jump into a match
```

`defaults/` ships 14 profiles: DualSense, DualSense menu, DualShock 4/3,
Xbox, Xbox 360, Steam Deck, 8BitDo, Switch Pro, Joy-Con, Wii U Pro, GameCube
adapter, generic and keyboard. The launcher's CONTROLLER page detects any SDL
pad, writes the exact device name, and has an Auto / Xbox-PlayStation /
Nintendo / Keyboard layout selector. DualSense Edge and Xbox Elite Series 1/2
are GUID-detected as standard PlayStation/Xbox pads; their paddles, Fn and
touchpad buttons are intentionally ignored. The D-pad is **taunt** in WMX8; menus use
the left stick and the right stick switches targets. Only the `Device =` line
is controller-specific; see `defaults/README.md`.

## Quick start

```powershell
# Extract directly from an ISO or RVZ with the launcher (no conversion)
tools\extract_game.ps1 -Image "WWE WrestleMania X8 (USA).rvz" -Force
# or: tools\extract_game.ps1 -Image "WWE WrestleMania X8 (USA).iso" -Force

# Build the module (MSVC first, then Clang)
& <moderngekko>\moderngekko-port.exe build work\game --backend c --toolchain msvc --output build\modules
& <moderngekko>\moderngekko-port.exe build work\game --backend c --toolchain clang --opt-level 3 --output build\modules

# Run from the match checkpoint
& <moderngekko>\moderngekko-run.exe --game work\game --module <module>\gGW3E78_recomp.dll `
  --user-dir work\user-3 --graphics Vulkan --audio Null `
  --automation-dir work\auto --load-state work\states\match.sav
```

`.iso`, `.rvz` and `.wbfs` are first-class launcher inputs. The file picker
lists all three, and `extract_game.ps1` drives the launcher's headless
`--extract` path, which reads RVZ through Dolphin DiscIO and writes the same
`sys/` + `files/` tree as an ISO. `tools\rvz_extract.py` is an optional
fallback for ISO-only diagnostics, not part of this flow.

`tools\verify_dol.py`, `tools\check_fst.py` and `tools\gcdisc.py` accept the
extracted `work\game` folder directly, so RVZ users do not need a raw ISO for
the diagnostics either.

Automation commands go in `work/auto/commands/*.txt`:

```
command=pad_frames
port=0
frames=3
a=1
```

`work/auto/status.txt` reports `fps`, `vps`, `speed` and `frame_count`.

## Benchmark

```powershell
tools\bench.ps1 -Label msvc-static -Module <msvc dll> -Core static
tools\bench.ps1 -Label jit          -Module <any dll>  -Core jit
```

Results append to `docs/benchmarks.csv`.

## Licence

Project code: GPL-3.0 (see `LICENSE`). DolRecomp and ModernGekko are GPL-3.0;
Dolphin is GPL-2.0-or-later. The combined work is GPL-3.0. WWE and all related
marks are trademarks of their respective owners. This is an unofficial fan
project with no affiliation. No game data is included.
