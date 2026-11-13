# WrestleMania X8 Recomp v0.1.0

Initial public build.

## What works

- Boots the retail USA disc (`GW3E78`) from legal/logo screens through the
  opening movie, title screen and main menu.
- Exhibition -> Single -> Normal Match -> character select -> arena select ->
  entrance -> live in-ring match.
- **59.94 fps / speed 1.00** in the tested 1v1 match with the Clang `-O3`
  static module.
- Pause menu, memory-card System Data creation and loading.
- Themed launcher (PLAY / GRAPHICS / PERFORMANCE / AUDIO / CONTROLLER) with
  SteamGridDB icon and backdrop art for WrestleMania X8 (Macho Sadness).
- Launcher parity tools: memory-card create/backup/import/reveal, savestate
  list/load/delete/reveal, host checks (shader cache, High priority,
  physical-core affinity, DSP ROMs), log diagnostics, and a 15 s close guard.
- Controller support for Xbox, PlayStation, Switch/Joy-Con, Steam Deck,
  8BitDo, the GameCube adapter and keyboard, with auto-detection and a manual
  Nintendo A/B-swap override.
- ISO and RVZ import with retail DOL verification.

## Requirements

- Windows 10/11 x64.
- Your own USA GameCube disc of WWE WrestleMania X8 (`.iso` or `.rvz`).
- Vulkan or OpenGL 4.4 capable GPU.

## Notes

- The static module is the default core. JIT is available as a fallback.
- The default controller profile uses the D-pad for menu navigation because
  the original game ignores it in menus. L1 + D-pad still taunts in a match;
  the stock layout is included in `defaults/`.
- No game data is included in this release.

## Known limitations

- Only a 1v1 Exhibition match is verified end to end. Fatal 4 Way, Battle
  Royal, entrances and a match finish are untested in this build.
- Audio is enabled but was not listened to on every output device.
- No texture, music or gameplay mods.
- Save states are build-specific.
