# WWE WrestleMania X8 Recomp v0.1.2

Controller hotfix and native RVZ input. The game reads the live SDL mapping
instead of guessing the button family from the product name, and the launcher
reads `.rvz` directly through Dolphin DiscIO with no conversion step.

This package contains **no game data**. You need your own legally dumped USA
disc (`.iso` or `.rvz`).

## What changed

- **Native RVZ input.** `.rvz` is read directly by the launcher and
  `tools/extract_game.ps1` through Dolphin DiscIO; there is no RVZ-to-ISO
  conversion step. The extracted tree is byte-identical to the ISO path.
- **8BitDo A/B and X/Y normalization.** The runner and launcher read the SDL
  mapping string (`a`, `b`, `x`, `y`) for the connected pad. Pads that map
  `a:b1,b:b0` or `a:b3,b:b2` get the Nintendo swap; pads that map
  `a:b0,b:b1` stay standard.
  - Pro 2, SN30, SN30 Pro/Pro+, SF30 Pro, N30 Pro 2, FC30 Pro, NES30, Zero,
    Zero 2 and Micro Nintendo/Switch modes now match their printed labels.
  - Ultimate Wired/Wireless/2C, the 64 controller and XInput modes keep the
    standard layout. A blanket "8BitDo means Nintendo" rule would have broken
    these, so detection is per mapping/GUID.
- **DualSense Edge and Xbox Elite.** GUID-level detection for Sony
  `054C:0DF2` and Microsoft `045E:02E3` / `045E:0B00` / `045E:0B05` maps them
  to the standard DualSense/Xbox layout. Paddles, Fn and touchpad are
  intentionally not bound; use the standard face buttons, sticks, D-pad and
  triggers.
- **D-pad left/right correction.** If a mapping reports `dpleft`/`dpright` in
  the opposite order (hat, button or axis form), the generated profile swaps
  the D-pad left/right lines. The standard 8BitDo entries in the shipped SDL
  DB are already correct and stay unchanged.
- **Existing profiles migrate.** A default 8BitDo profile that was generated
  with the old standard layout is rewritten with the detected Nintendo layout.
  Hand-edited profiles are left alone; the manual **Nintendo (A/B swap)**
  override still works.
- **Diagnostics.** Startup now logs one line with the SDL name, GUID, chosen
  layout and D-pad state, for example:

  ```text
  controller: name="SDL/0/8BitDo Pro 2" guid=03000000c82d00000660000011010000 layout=nintendo dpad=normal
  ```

## Verification

- Offline layout tests pass for:
  - 8BitDo Pro 2 `a:b1,b:b0` → Nintendo A/B and X/Y swap.
  - 8BitDo Ultimate `a:b0,b:b1` → standard layout.
  - 8BitDo Pro 2 Switch-mode `a:b3,b:b2` → Nintendo swap.
  - 8BitDo Micro synthetic reversed D-pad mapping → `D-Pad/Left = Pad E`,
    `D-Pad/Right = Pad W`.
  - Default-profile migration and custom-profile preservation.
- GUID tests pass for DualSense Edge USB/Bluetooth (`054C:0DF2`), Xbox Elite
  Series 1/2 (`045E:02E3`, `045E:0B00`, `045E:0B05`) and the standard
  DualSense/Xbox pads; Edge/Elite GUIDs override a Nintendo-looking name.
- Generated Edge/Elite profiles contain no paddle, Fn or touchpad bindings.
- Build and headless keyboard-fallback smoke test pass.
- RVZ and ISO extracted with the same launcher build are byte-identical:
  5,271 files, 1,414,424,163 bytes, manifest
  `b019760f7429ecfa9543d055a88bf841b9dd02749dd79ed6732af9959e59d725`;
  DOL `91af3136…a303e`; FST `c7f82d55…e1d4bb`.
- The RVZ-extracted tree boots the v0.1.2 runner at 59.93 fps / 1.003 in the
  Fatal 4 Way on a hidden desktop.

## Please verify

Physical hardware was not available for the controller work; the mappings were
verified against the SDL gamepad DB and offline tests. Please check:

- 8BitDo Pro 2/SN30 in Nintendo/Switch mode: A/B and X/Y match the printed
  labels; D-pad left/right is correct.
- DualSense Edge: standard DualSense face buttons, sticks, D-pad and triggers.
  Paddles, Fn and touchpad are intentionally ignored.
- Xbox Elite Series 1/2: standard Xbox face buttons, sticks, D-pad and
  triggers. P1-P4 paddles are intentionally ignored.

If anything is off, send the startup line:
`controller: name="…" guid=… layout=… dpad=…`.

## Requirements

- Windows 10/11 x64.
- Your own USA GameCube disc of WWE WrestleMania X8 (`.iso` or `.rvz`).
- Vulkan or OpenGL 4.4 capable GPU.
- About 1.4 GB free for the extracted game.

## Known limitations

- 1v1 and Fatal 4 Way are verified end to end. Other match types, every
  entrance and a full match finish are not exhaustively tested.
- Netplay is not part of the tested v0.1.2 scope.
- Save states are build-specific.
- No texture, music or gameplay mods.

## Credits

Built on ModernGekko, DolRecomp and Dolphin. Launcher art from SteamGridDB:
Macho Sadness (icon, hero, logo) and Just.Seani (grid). See `THIRD_PARTY.md`
in this package and `docs/CREDITS.md` in the repository.

Project code is GPL-3.0. WWE and all related marks are trademarks of their
respective owners. This is an unofficial fan project with no affiliation.
