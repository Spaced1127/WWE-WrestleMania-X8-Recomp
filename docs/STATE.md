# Project state — WWE WrestleMania X8 (GameCube)

Updated: 2026-11-09

## Goal

Native Windows port of the retail GameCube release of *WWE WrestleMania X8*
by static recompilation, following the
[GameCube Static Recomp Guide](https://github.com/Spaced1127/GameCube-Static-Recomp-Guide)
(DolRecomp C backend + ModernGekko runtime).

No game data is committed. The disc image, extracted files, generated code and
build output stay out of version control.

## Disc / DOL inventory

| Item | Value |
|---|---|
| Disc ID | `GW3E78` |
| Title (header) | `WRESTLE MANIA X8` |
| Region | USA (country byte 0x00) |
| ISO size | 1,459,978,240 bytes |
| ISO SHA-256 | *(not yet recorded)* |
| DOL size | 2,145,824 bytes |
| DOL SHA-256 | `91af3136f2a48c18a6b96a5726d1f171c9e714bb8d4d504f835070fc230a303e` |
| DOL SHA-1 | `91571ac05777f4847abfb41320bb0990da9ce3ba` |
| Entry point | `0x800051fc` |
| BSS | `0x80206b60` + `0x99f90` |
| FST | 5,773 entries, `0x1e7dc` bytes at ISO `0x229900` |
| DOL sections | 2 text, 8 data |

Source image: `WWE WrestleMania X8 (USA).rvz` (908,764,000 bytes), read
natively by the launcher and `tools/extract_game.ps1` through Dolphin DiscIO.
The `work/disc/game.iso` copy is only an inventory reference; the play/build
path does not convert RVZ to ISO.

`python tools/verify_dol.py work/game work/game/sys/main.dol` reports size,
SHA-256 and SHA-1 all OK, zero byte differences. The same check passes for the
RVZ-extracted folder.

## RVZ native input (2026-11-15)

The launcher `--extract` path and `tools/extract_game.ps1` read `.rvz` directly.
The project RVZ and its ISO were extracted with the same launcher build into
separate roots:

| Check | Result |
|---|---|
| Files | 5,271 both |
| Bytes | 1,414,424,163 both |
| Tree manifest SHA-256 | `b019760f7429ecfa9543d055a88bf841b9dd02749dd79ed6732af9959e59d725` both |
| DOL SHA-256 | `91af3136f2a48c18a6b96a5726d1f171c9e714bb8d4d504f835070fc230a303e` both |
| FST SHA-256 | `c7f82d550d7ddee726f1e52646acfff8384b8f4836d790dc4d772a8013e1d4bb` both |
| boot / bi2 / apploader | identical |

No RVZ-to-ISO conversion is used in the play/build path. The v0.1.2 runner
boots from the RVZ-extracted tree and holds **59.93 fps / 1.003** in the Fatal
4 Way on a hidden desktop.

## Asset profile (FST)

5,266 files extracted, 1,412,032,323 bytes. 5,773 FST entries.

| Extension | Count | Notes |
|---|---|---|
| `.yad` | 1,981 | Yukes animation data (`adm/sheet/...`) |
| `.tex` | 1,011 | textures |
| `.ymg` | 732 | Yukes model/geometry group |
| `.tpl` | 623 | GameCube textures |
| `.pac` | 296 | archives |
| `.ycg` | 163 | |
| `.ama` | 154 | |
| `.dat` | 147 | |
| `.ymf` | 93 | |
| `.lad` | 28 | |
| `.mpc` | 6 | `static.mpc`, movies |

Engine is the Yukes `adm` layout: `adm/sheet/<brand>/`, `adm/telop/`,
`adm/titan/`, plus `bg/`, `model/`, `motion/`, `menu/`, `sound/`, `movie/`,
`edit/`, `help/`.

## Toolchain

Reusing the known-good build from the SpongeBob project (`H:\Projects\SB-RFD`)
so no cold DolRecomp/ModernGekko build is needed:

- `moderngekko-port.exe` — `H:\Projects\SB-RFD\build\moderngekko`
- `dolrecomp.exe` — same directory
- `moderngekko-run.exe` — same directory (runner)
- Clang 22.1.8 at `C:\Program Files\LLVM\bin`
- Ninja from VS Build Tools
- MSVC x64 from VS Build Tools 18

## Status

| Step | State |
|---|---|
| RVZ / ISO native input | done, byte-identical |
| Disc / DOL / FST inventoried | done |
| DOL hash verified against ISO and RVZ | done |
| Game root extracted (`sys/` + `files/`) | done |
| Static C module generated + compiled | done |
| First boot in ModernGekko | done |
| Legal / logo screens render | done |
| Title screen / main menu | done |
| Character / arena select | done |
| In-ring match gameplay | done |
| Clang `-O3` module | done |
| Optimized Clang module (v3 + budget 4096 + depth 128) | done, active candidate |
| Fatal 4 Way performance | **59.93 fps / 1.004 with Cubeb** |
| 1v1 performance | 59.91 fps / 0.995 with Cubeb |
| PGO | collected, not needed (idle-loop dominated) |
| Branded launcher / runner | done, parity patched |
| v0.1.2 release zip | done, no game data |

## Module

| Item | Value |
|---|---|
| Generated C | 102 chunks |
| Decoded instructions | 415,760 (0 unknown) |
| Cross-chunk calls | 102 chunks |
| SMC patch candidates | 103 ranges |
| Build work dir | `build/modules/GW3E78/91af3136…a303e-a71f4a5f346fbe4a` |
| Module DLL | `…/gGW3E78_recomp.dll` (118.59 MB) |
| Backend / toolchain | C / MSVC 19.51 (release) |

## First boot evidence

Command:

```powershell
moderngekko-run.exe --game work\game \
  --module <module>\gGW3E78_recomp.dll \
  --user-dir work\user-3 --graphics Vulkan --audio Null \
  --title WMX8 --automation-dir work\auto
```

- Log: `[staticrecomp] module loaded … entry=0x800051FC`
- Clean shutdown stats: `native=9,907,912 fallback=0 native_exc=2701
  hook_fb=59 smc_failed=0 verifications=33 cycles=2,616,664,899`
- Automation status at the system-data prompt:
  `booted=1 fps=59.9402 vps=59.9401 speed=1.00054`
- Framebuffer captures: `work/evidence/fb-65s.png` (legal screen),
  `fb-75s.png` (THQ), `fb-85s.png` (JAKKS Pacific).
- Screenshots are taken through the automation protocol
  (`command=screenshot`), not the desktop, so they are unaffected by other
  windows.

Input: `--automation-dir` accepts `commands/*.txt` with `command=pad_frames`,
`port=0`, `frames=N`, `a=1`. This is the reliable input path; synthetic
keyboard events via `keybd_event` did not reach the game even with a keyboard
`GCPadNew.ini`.

Boot takes ~45 s wall time before the system-data prompt; most of that is the
data load, not emulation speed. The game itself reports 59.94 fps at the
prompt.

## Gameplay reached (2026-11-09)

Full flow verified through the automation protocol:

1. Boot → legal → THQ → JAKKS → opening movie → title screen.
2. Start → main menu → EXHIBITION → SINGLE → NORMAL MATCH → match rules OK.
3. Character select → The Rock (1P) → Steve Austin (COM).
4. Arena select (stage carousel, L/R, "Press A").
5. A → `NOW LOADING` → WrestleMania X8 arena loads.
6. Entrance → in-ring match with full HUD, crowd, ropes and AI.

Evidence: `work/evidence/fb-tap2.png` and `fb-tap3.png` (in-ring, health bars,
infinite timer, arena), `fb-t3.png` (arena during load), `fb-e1..4`
(memory-card system-data load), `fb-q*`, `fb-h*`, `fb-gp*`.

Automation status during the live match: `fps=41.2 vps=41.27 speed=0.70` while
the tap script, screenshots and the user's browser were running. Menus hold
`59.94 fps / speed=1.00`. A clean match baseline is still to be measured.

### Memory card / System Data

The match start reads System Data from Memory Card Slot A. With an empty card
the game prompts to create it; answering YES creates:

`work/user-3/GC/USA/Card A/78-GW3E-__2f__w_mania2002.gci` (114,752 bytes)

After that, the card load reports `Load complete` and the match proceeds.

### Input findings

- `--automation-dir` is the reliable input path (`command=pad_frames`,
  `port=0`, `frames=N`, `a=1`, `start=1`, `main_x=-1`, ...).
- Synthetic `keybd_event`/`SendKeys` did not reach the game.
- The first pad command after `--load-state` is swallowed; the arena screen
  also ignores long holds. Repeated **short A taps** (3 frames every 3 s)
  started the match reliably.
- Avoid the all-buttons combo: it includes B and backs out into the attract
  demo.

## Repeatable checkpoints

| State | Path | Contents |
|---|---|---|
| Title screen | `work/states/menu.sav` | "PRESS START", 33.5 MB |
| Main menu | `work/states/mainmenu.sav` | EXHIBITION highlighted, 22.0 MB |
| Character select | `work/states/charsel.sav` | empty grid, 27.7 MB |
| Arena select | `work/states/matchup.sav` | Rock vs Austin, "Press A", 31.6 MB |
| Final grid | `work/states/finalgrid.sav` | both picked, 32.0 MB |

## Clang result (2026-11-10)

The module built with `--toolchain clang --opt-level 3` is now the active
module (`build/modules/GW3E78/active-module.txt`):

```
[staticrecomp] module loaded: …-8f78e41483de3f25\gGW3E78_recomp.dll entry=0x800051FC
```

Same `match.sav`, same settings:

| Module | FPS | Speed |
|---|---|---|
| MSVC `/O2` | 40.7 | 0.674 |
| Clang `-O3` | **59.94** | **1.0017** |

Menus and in-match both hold the 59.94 Hz target. Clang emitted only one
warning (a `/*` inside a comment in the runtime, harmless).

## Play verification (2026-11-10)

- Clang module, `match.sav`: **59.93 fps / speed 1.0027** while pad input was
  being sent.
- In-ring controls respond: The Rock punches Austin with A; the AI fights back.
- Start opens the **PAUSE** menu (Return to Game, Quit Game, Camera Angles,
  Player Indicators, Help) — `work/evidence/fb-pz2.png`.
- The first Start after a state load is swallowed; the second opens pause.

## How to play

```powershell
$tools\play.ps1                      # auto-detect a controller
$tools\play.ps1 -Keyboard            # keyboard profile
$tools\play.ps1 -State work\states\match.sav   # jump into the match
```

`work/user-play` is seeded with `defaults/config.play.ini` (audio on, 1080p
internal resolution) and the memory card from `work/user-3`.

Keyboard mapping (`defaults/GCPadNew.keyboard.ini`): Enter = A, Backspace = B,
X/Y = X/Y, Left Shift = Z, Space = Start, WASD = main stick, IJKL = C-stick,
Q/E = L/R.

## Controller profiles

`defaults/` holds generic Dolphin `GCPadNew.ini` profiles:

| Layout | Files | Notes |
|---|---|---|
| PlayStation | `GCPadNew.dualsense.ini`, `dualsense-menu.ini`, `dualshock4.ini`, `dualshock3.ini` | Cross=A, Circle=B, Square=X, Triangle=Y |
| Xbox | `GCPadNew.xbox.ini`, `xbox360.ini`, `steamdeck.ini`, `8bitdo.ini` | standard Xbox layout |
| Nintendo | `GCPadNew.switch-pro.ini`, `joycon.ini`, `wiiu-pro.ini` | A/B and X/Y swapped to Nintendo layout |
| Standard / generic | `GCPadNew.gamecube.ini`, `generic.ini` | GameCube adapter and unknown SDL pads |
| Keyboard | `GCPadNew.keyboard.ini` | Enter=A, Backspace=B, Space=Start |

`tools\play.ps1 -Layout dualsense|xbox|switch|keyboard` installs one of the
four original templates. `-Device "SDL/0/<name>"` overrides the device line.
Only `Device =` is controller-specific; every other line uses Dolphin's
generic SDL names (`Button A`, `Pad N`, `Left Y+`, `Trigger L`, ...).

The launcher is the primary path: its CONTROLLER page lists every SDL gamepad
plus **Keyboard (WASD)**, writes the exact SDL device name, and has a
**Layout** selector (Auto / Xbox-PlayStation / Nintendo / Keyboard).
Auto-detection swaps A/B and X/Y for names containing Switch, Joy-Con,
Nintendo, Wii U or Pro Controller; everything else uses the standard
positional layout. The manual Nintendo option covers 8BitDo pads in Switch
mode and any pad whose face buttons are swapped.

GUID-level families:
- DualSense Edge (Sony `054C:0DF2`, USB and Bluetooth) -> standard DualSense
  layout. Paddles, Fn and touchpad are intentionally not bound.
- Xbox Elite Series 1/2 (Microsoft `045E:02E3`, `045E:0B00`, `045E:0B05`,
  `045E:0B22`) -> standard Xbox layout. P1-P4 paddles are intentionally not
  bound.

Layout heuristic scope (fixed in v0.1.3): the live-mapping A/B rule applies to
the 8BitDo family only (name or `c82d`/`2dc8` vendor, or `05AC:0004` Switch
mode). Applying it to every pad flipped 219 of the SDL DB's 812 rows. The
button-index D-pad correction is likewise 8BitDo-only, because the DualSense
family binds `dpleft:b15,dpright:b13` without being reversed. Nintendo names
and Nintendo/8BitDo-Switch GUIDs still select the Nintendo layout.

Profile scan (fixed in v0.1.3): the runner applies Dolphin's SDL hints
(`JOYSTICK_DIRECTINPUT`, `HIDAPI_GAMECUBE`, Joy-Con combine hints), initialises
joystick/haptic/gamepad only, and leaves them running for the runtime. It never
rewrites `GCPadNew.ini` when no gamepad enumerated, and falls back to the first
connected pad when the configured one is missing.

**D-pad in WMX8 is taunt**, not movement. Verified with automation on the
main menu: D-pad Down leaves the highlight on EXHIBITION, so the game's menus
read only the main stick. In a match the D-pad triggers taunts 1-4 and the
right stick switches targets.

For D-pad menu navigation, `tools\play.ps1` (default) folds the D-pad into the
main stick and gates taunts behind L1. `-StockDpad` restores the normal
layout:

```
Main Stick/Up = `Pad N` | `Left Y+`
D-Pad/Up      = `Pad N` & `Shoulder L`
```

Menus: D-pad or left stick. Match: D-pad moves, L1 + D-pad taunts. A standalone
copy is `defaults/GCPadNew.dualsense-menu.ini`.

## Launcher and v0.1.2 release

The launcher and runner are built from the local copy at
`lib\ModernGekko-Template\lib\ModernGekko` (the SpongeBob launcher source plus
the local runtime fixes), configured with:

```
MODERNGEKKO_FRONTEND_NAME          = "WWE WrestleMania X8 Recomp"
MODERNGEKKO_LAUNCHER_OUTPUT_NAME   = WrestleManiaX8Recomp
MODERNGEKKO_RUNNER_OUTPUT_NAME     = WrestleManiaX8RecompRunner
MODERNGEKKO_USER_DIRECTORY_NAME    = WrestleManiaX8Recomp
MODERNGEKKO_DEFAULT_WINDOW_TITLE   = "WWE WrestleMania X8"
MODERNGEKKO_REQUIRED_DISC_ID       = GW3E78
MODERNGEKKO_REQUIRED_DOL_SHA256    = 91af3136…a303e
MODERNGEKKO_PORTABLE_DEFAULT_GAME  = ON
MODERNGEKKO_GAMECUBE_CONTROLLERS   = ON
```

Build with `tools\build_launcher.bat`; package with
`tools\make_release.ps1 -Force`.

Themed pieces:

- `tools/make_steamgrid_assets.py` builds the launcher art from the
  SteamGridDB files in `art/steamgriddb/`: multi-size ICOs and 1024 px PNGs at
  `lib/.../resources/icons/`, plus a blurred 2560x827 backdrop at
  `resources/background.png`. Artists: Macho Sadness (icon, hero, logo) and
  Just.Seani (grid); see `docs/CREDITS.md`.
- `defaults/theme.ini` supplies product/subtitle/disc ID/version/accent and
  `background=resources/background.png`. The launcher loads the PNG at runtime
  through Dolphin's `Common::LoadPNG` and falls back to the procedural arena
  (gold spotlights, crowd bokeh, red ring ropes) when the file is absent.
- Artwork note: the icon and backdrop are SteamGridDB promotional art, not
  data extracted from the disc, and are used for the themed launcher at the
  user's request. No disc textures, audio or levels are shipped. See
  `docs/CREDITS.md`.
- Header: "STATIC RECOMPILATION" / "WWE WrestleMania X8". Palette is charcoal
  + gold + red.
- The missing-runner message uses the configured runner name instead of the
  SpongeBob name.
- The generated controller profile defaults to D-pad menu navigation
  (`Main Stick/* = Pad N/S/W/E | stick`, `D-Pad/* = Pad N/S/W/E & Shoulder L`).
- The CPU core defaults to **static**; the PERFORMANCE page labels JIT as the
  fallback.

Two runtime fixes were needed for this title:

- `InspectGame` no longer hashes the whole `files/` tree. The release pins the
  DOL only; hashing 1.4 GB on every boot was slow, and the scan crashed
  (`0xC0000409`) on the disc's Japanese "コピー 〜" developer artifacts.
- The launcher's `--extract` now completes: extraction, verification and
  `default-game.txt` all succeed (exit 0).

Release artifact:

```
dist\WrestleManiaX8Recomp-v0.1.2-Windows-x64.zip   19,580,869 bytes
SHA-256 E22F48D2808471CAB4ED0E22C85DAF31D187264BCEA61A90FE87ADCF1D0064FB
Release https://github.com/Spaced1127/WWE-WrestleMania-X8-Recomp/releases/tag/v0.1.2
```

Contents: launcher, runner, `gGW3E78_recomp.dll`, pruned `Sys/` (GC,
Shaders, OSD font, `GW3E78.ini`), `resources/background.png`, `LICENSES/`,
`defaults/` (14 controller profiles), README, PLAYERS, release notes,
`recomplauncher.json`. No game data. Verified: `--extract` exit 0; `--play`
with `--core static` loads `gGW3E78_recomp.dll` and reaches 59.9 fps; the
zip's runner loads the module and the `match.sav` state (`game_id=GW3E78`).

Note: the extraction still creates the disc's five non-ASCII "copy" entries
(one directory tree). They are not referenced by normal gameplay and the
loader ignores them; they are not shipped.

Release verification (2026-11-15):

- [x] `WrestleManiaX8Recomp.exe` and `WrestleManiaX8RecompRunner.exe` build
      with the WMX8 icon resources.
- [x] `--extract` on the retail ISO and the project RVZ both exit 0 and write
      byte-identical `Game` trees (5,271 files; manifest
      `b019760f…`).
- [x] `--play` starts the runner with `--core static`; the runner loads
      `gGW3E78_recomp.dll` from the release folder and reaches 59.9 fps.
- [x] The zip's runner loads the module and the `match.sav` state
      (`game_id=GW3E78`).
- [x] The zip contains no ISO/RVZ/DOL/FST/GCI/`Game` data.
- [x] `recomplauncher.json` validates against the SDK schema (0 errors).
- [x] Icon extraction from both exes shows the SteamGridDB Austin icon.
- [x] The zip's launcher loads `resources/background.png` and the WMX8 theme.
- [x] The v0.1.2 zip's runner holds **59.86 fps / 1.001** in the Fatal 4 Way
      with Null audio on a hidden desktop, high priority and affinity `0xFF`.
- [x] RecompLauncher scenario: a fresh user dir with no custom launcher, the
      runner auto-detects the pad and writes `Config\GCPadNew.ini` itself.
- [x] `controller_layout=nintendo` in `config.ini` regenerates the profile
      with A/B and X/Y swapped; Auto preserves an existing custom profile.
- [x] The keyboard layout writes `DInput/0/Keyboard Mouse` with
      Enter/Backspace/WASD bindings, and `--controller-layout` works for
      scripted launches.
- [x] 8BitDo mapping tests: Pro 2 `a:b1,b:b0` -> Nintendo; Ultimate
      `a:b0,b:b1` -> standard; Pro 2 Switch-mode `a:b3,b:b2` -> Nintendo;
      Micro reversed D-pad -> `D-Pad/Left = Pad E`, `Right = Pad W`.
- [x] The name-only fallback still returns Standard for 8BitDo Pro 2; the
      mapping-aware path returns Nintendo (root-cause regression test).
- [x] Existing default 8BitDo profiles migrate to the detected Nintendo
      layout; custom profiles are preserved.
- [x] DualSense Edge (Sony `054C:0DF2`, USB/Bluetooth) and Xbox Elite
      (`045E:02E3`, `045E:0B00`, `045E:0B05`) GUID tests -> standard layout;
      Edge/Elite GUIDs override a Nintendo-looking name; no paddle/Fn/touchpad
      bindings; standard DualSense/Xbox unchanged.
- [x] Startup diagnostics log the SDL name, GUID, chosen layout and D-pad
      state.
- [x] Controller layout tests pass for DualSense, DualSense Edge, DualShock 4,
      Xbox 360, Xbox Elite 1/2, Steam Deck, 8BitDo
      (Pro 2/Ultimate/Switch-mode/Micro), Switch Pro, Joy-Con, Wii U Pro and
      keyboard, including Nintendo A/B and X/Y swapping, D-pad correction and
      keyboard bindings.
- [x] The zip's `defaults/` contains 14 profiles: DualSense, DualSense menu,
      DualShock 4/3, Xbox, Xbox 360, Steam Deck, 8BitDo, Switch Pro, Joy-Con,
      Wii U Pro, GameCube adapter, generic and keyboard.
- [x] The zip's runner and module are byte-identical to the release folder
      (`79E69136…`, `B4264EEE…`).
- [x] Theme audit: no other-title branding or disc IDs in shipped
      launcher/config/docs files; Dolphin's generic `Sys/Shaders` collection is
      unmodified.
- [ ] Upload/download-back hash check (no public release yet).

## Controller hotfix v0.1.3 (2026-11-16)

Reported after v0.1.2: Xbox pads not responding at all; DualSense Edge and
8BitDo behaviour in doubt.

Root cause (Xbox): the v0.1.2 pre-boot scan rewrote `Config\GCPadNew.ini` even
when it enumerated no pad. A wireless Xbox pad still waking up therefore got a
keyboard profile, and the pad connected a second later with no bindings. The
scan also ran with a different SDL driver set than the runtime (video+gamepad
init, then `SDL_Quit()`, no Dolphin hints), so the stored `Device =` name could
differ from the runtime's enumeration.

Also fixed: the blanket `a>b` mapping rule (219/812 SDL DB rows affected) is
now 8BitDo-only; the button-index D-pad correction no longer mirrors DualSense
and DualSense Edge (`dpleft:b15,dpright:b13`); Xbox Elite `045E:0B22` added.

Evidence:

- `work/ctl-test/layout_test.exe` — 22-row SDL DB matrix plus rewrite rules:
  all pass.
- Hidden desktop, existing Xbox profile, no pad attached (joysticks=0):
  `controller: no gamepad enumerated (joysticks=0 gamepads=0); keeping existing
  GCPadNew.ini`; device line unchanged; `work/hotfix/logs/A.log`.
- Hidden desktop, fresh user dir, no pad: keyboard profile written,
  `booted=1 fps=59.94`; `work/hotfix/logs/C.log`.
- Shipped zip runner, existing Xbox profile, no pad: guard line, module loaded,
  `booted=1 fps=59.94`; `work/hotfix/logs/D.log`.
- Missing-device fallback is covered by the rewrite test only: no second
  physical pad was attached during verification.

Artifact (not published):

```
dist\WrestleManiaX8Recomp-v0.1.3-Windows-x64.zip   19,590,673 bytes
SHA-256 6A9BD66A5E05CD2FCC850A3C3E0CB99EB966909FA123C94D973396E5C9F0D646
Release https://github.com/Spaced1127/WWE-WrestleMania-X8-Recomp/releases/tag/v0.1.3
```

### Memory card (shared pattern from Aggressive Inline)

Applied: `Core/SlotA = 1` (raw), `Core/SlotB = 255` (None), forced in the
runtime after `UICommon::CreateDirectories()` and re-forced from the launcher
and runner into `Config/Dolphin.ini`; manifest carries both keys in
`ensure`/`always`/`new`.

Evidence (fresh user dir `work/mem/M1`, hidden desktop):
`GC/USA`, `GC/EUR`, `GC/JAP` created; `GC/MemoryCardA.USA.raw` = 16,777,216
bytes; `Config/Dolphin.ini` begins `[Core] SlotA = 1 / SlotB = 255`; no
`GC/USA/Card A`. Card SHA-256 before/after five boots incl. an in-ring match:
`8A2C41E9B230B61B98541049E3E01F2B2230CFEBCAEA413005AA05ADDEC0F98D` unchanged —
**no game-written save was triggered** (this title writes System Data only from
its own save flow; automation reached title → menu → match but never the save
prompt). Save/load through the card is therefore still unproven.
Launcher migration verified on a real install: it wrote both keys into the
existing `%LOCALAPPDATA%\WrestleManiaX8Recomp\Config\Dolphin.ini`.

Release notes: `docs/release/release-notes-v0.1.3.md`.

## Launcher parity (docs/15)

`Launcher: shell ModernGekko-Template + WMX8 parity patch, theme WWE
WrestleMania X8, parity 2026-11-13`

| Area | Result |
|---|---|
| Setup | pass — ISO/RVZ import, ID/DOL/Redump check, progress, headless `--extract` |
| Settings | pass — resolution/MSAA/SSAA/AF/VSync/fullscreen/audio/core/widescreen |
| Input | pass — every SDL pad + keyboard listed, exact device name, Auto/Xbox-PlayStation/Nintendo/Keyboard layout selector, menu-D-pad default |
| States | list/load/delete/reveal pass; save is F1 in the runner (exception) |
| Cards | pass — create, back up, import `.gci`, reveal |
| Host checks | pass — `Cache\Shaders`, High priority, physical-core affinity, DSP ROM/coef, mute shown |
| Diagnostics | pass — open logs, copy log tail, runner/module names, `[video]` line |
| Close | pass — 15 s wait then kill tree; no modal on close |
| Updates | version shown; update check delegated to RecompLauncher (exception) |
| Theme | pass — `theme.ini` supplies product/subtitle/disc ID/version/accent/background; icon is the SteamGridDB Austin mark |

Recorded exceptions:

- **Save state:** the launcher lists, loads, deletes and reveals states; saving
  is F1 in the runner because the launcher blocks while the game runs. Owner:
  runner/launcher shell.
- **Update check:** RecompLauncher owns updates; the custom launcher shows the
  version and points to RecompLauncher. Owner: release tooling.
- **Backdrop style:** `theme.ini` can point at a PNG backdrop and the launcher
  covers the window with it; other layout styles are not implemented. Owner:
  launcher shell.

## Open issues / limitations

- Audio was not listened to; Cubeb initializes but needs a human check.
- Only the 1v1 Exhibition path is verified end to end. Fatal 4 Way / Battle
  Royal / entrances are not benchmarked.
- The pause menu is confirmed; a full match to a finish is not.
- 410 FST files with Japanese "コピー 〜" names (developer copy artifacts and a
  `.lnk`) are not on disk. They are not referenced by normal gameplay; the
  game runs without them.
- The runner and build tools still come from `H:\Projects\SB-RFD`. A branded,
  self-contained build and RecompLauncher packaging are not started.
- The first pad input after `--load-state` is swallowed.

## Next actions

1. Publish v0.1.3 once the hub/user approves (no commit or push yet).
2. Re-test with a physical Xbox pad, DualSense Edge and 8BitDo Pro 2 on this
   machine: the pad-present branch (device found, profile reused) has no live
   evidence yet; only the no-pad and unit-test paths do.
3. Test heavier scenes (Fatal 4 Way / Battle Royal, entrances) for dips. Only
   then consider `-march`, loop budget/call depth and PGO.
4. Playtest with the user's own controller via `tools\play.ps1`.
5. Build a self-contained, WMX8-branded runner from a local template copy.
