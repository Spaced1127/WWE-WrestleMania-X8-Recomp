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
| RVZ → ISO | done |
| Disc / DOL / FST inventory | done |
| DOL hash verified | done |
| Static C module (MSVC) | done, boots |
| Boot → title → menu → match | done |
| In-ring gameplay | done, The Rock vs Steve Austin |
| Pause menu | done |
| Performance | **59.94 fps / speed 1.00 in-match (Clang `-O3`)** |
| PGO | optional headroom, not started |
| Branded launcher / runner | done |
| v0.1.0 release zip | done, no game data |

See [`docs/STATE.md`](docs/STATE.md) for the full inventory, evidence paths,
save-state checkpoints and open issues.

## v0.1.0 release

```powershell
tools\build_launcher.bat        # WrestleManiaX8Recomp.exe + ...Runner.exe
tools\finalize_release.ps1 -Module <gGW3E78_recomp.dll>   # folder + zip + hash
```

Artifact: `dist\WrestleManiaX8Recomp-v0.1.0-Windows-x64.zip`
SHA-256: `21EBA6A10B3D84D0E05AC31B16AE1D1C39D8BA89767102AFA1C696E20A6EB1AD`
GitHub release: https://github.com/Spaced1127/WWE-WrestleMania-X8-Recomp/releases/tag/v0.1.0

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
Nintendo / Keyboard layout selector. The D-pad is **taunt** in WMX8; menus use
the left stick and the right stick switches targets. Only the `Device =` line
is controller-specific; see `defaults/README.md`.

## Quick start

```powershell
# Decompress your own RVZ
python tools\rvz_extract.py "WWE WrestleMania X8 (USA).rvz" work\disc\game.iso

# Verify and extract
python tools\gcdisc.py info work\disc\game.iso
& <moderngekko>\dolrecomp.exe extract work\disc\game.iso work\game
python tools\verify_dol.py work\disc\game.iso work\game\sys\main.dol

# Build the module (MSVC first, then Clang)
& <moderngekko>\moderngekko-port.exe build work\game --backend c --toolchain msvc --output build\modules
& <moderngekko>\moderngekko-port.exe build work\game --backend c --toolchain clang --opt-level 3 --output build\modules

# Run from the match checkpoint
& <moderngekko>\moderngekko-run.exe --game work\game --module <module>\gGW3E78_recomp.dll `
  --user-dir work\user-3 --graphics Vulkan --audio Null `
  --automation-dir work\auto --load-state work\states\match.sav
```

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
