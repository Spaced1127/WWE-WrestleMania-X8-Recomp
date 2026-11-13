# WWE WrestleMania X8 Recomp v0.1.0

First public build of the native Windows static recompilation of **WWE
WrestleMania X8** (GameCube, USA, `GW3E78`).

This package contains **no game data**. You need your own legally dumped USA
disc (`.iso` or `.rvz`).

## Highlights

- Boots the retail disc through the legal/logo screens, opening movie, title
  screen and main menu.
- Exhibition -> Single -> Normal Match -> character select -> arena select ->
  entrance -> live in-ring match.
- **Full speed**: 59.91 fps / speed 0.995 in the 1v1 test; 59.93 fps / 1.004 in
  the four-wrestler Fatal 4 Way with Cubeb audio. The release runner measured
  60.00 fps / 1.006 after a 40 s warm-up.
- Static Clang `-O3` module is the default core; Dolphin JIT is selectable as a
  fallback.
- Themed launcher with SteamGridDB icon/backdrop art (Macho Sadness), parity
  tools, and controller layouts for Xbox, PlayStation, Switch/Joy-Con, Steam
  Deck, 8BitDo, the GameCube adapter and keyboard.
- ISO/RVZ import with retail DOL verification and headless `--extract`.

## Install

1. Extract the zip anywhere writable (not Program Files).
2. Run `WrestleManiaX8Recomp.exe`.
3. Browse for your `.iso`/`.rvz`, then **Extract and Play**. The disc is
   extracted once into `Game\` beside the launcher (about 1.4 GB).
4. Press Play.

Settings, controller profiles, memory cards and save states live in
`%LOCALAPPDATA%\WrestleManiaX8Recomp\`.

## Controls

The launcher writes `Config\GCPadNew.ini` with the exact SDL device name. On
the **CONTROLLER** page, pick the pad and choose a layout:

- **Auto-detect** swaps A/B and X/Y for Switch, Joy-Con, Wii U and other
  Nintendo-layout names.
- **Xbox / PlayStation** forces the standard layout.
- **Nintendo (A/B swap)** fixes a pad with swapped face buttons.
- **Keyboard (WASD)** writes the keyboard profile.

`defaults/` includes 14 profiles. The D-pad is a taunt in WMX8; the generated
profile folds it into the menu cursor, with **L1 + D-pad** for taunts. The stock
layout is in `defaults/GCPadNew.dualsense.ini`.

## Requirements

- Windows 10/11 x64.
- Your own USA GameCube disc of WWE WrestleMania X8 (`.iso` or `.rvz`).
- Vulkan or OpenGL 4.4 capable GPU.
- About 1.4 GB free for the extracted game.

## Notes

- No game data is included. The recompiled module is program code only.
- `Sys/GameSettings/GW3E78.ini` sets `StaticRecompIdlePC = 0x80179AAC`; this
  removes the frame-limiter spin and is what keeps the 4-way at full speed.
- Press **F1** in the game window to save a state. The launcher can list, load,
  delete and reveal states; saving is in the runner because the launcher is
  closed while the game runs.
- Update checks are delegated to RecompLauncher.

## Known limitations

- 1v1 and Fatal 4 Way are verified end to end. Other match types, every
  entrance and a full match finish are not exhaustively tested.
- Audio is enabled and verified with Cubeb; not every output device was
  listened to.
- Netplay is not part of the tested v0.1.0 scope.
- Save states are build-specific.
- No texture, music or gameplay mods.

## Credits

Built on ModernGekko, DolRecomp and Dolphin. Launcher art from SteamGridDB:
Macho Sadness (icon, hero, logo) and Just.Seani (grid). See `THIRD_PARTY.md`
in this package and `docs/CREDITS.md` in the repository.

Project code is GPL-3.0. WWE and all related marks are trademarks of their
respective owners. This is an unofficial fan project with no affiliation.
