# Players

## Disc image

The launcher accepts `.iso`, `.rvz` and `.wbfs` directly. `.rvz` is read
natively through Dolphin's DiscIO; there is no RVZ-to-ISO conversion step.

## Controls

| Action | Keyboard | GameCube pad | Xbox pad | DualSense | Switch Pro |
|---|---|---|---|---|---|
| Move / menu cursor | WASD | Left stick | Left stick | Left stick | Left stick |
| Taunt 1-4 | — | L1 + D-pad | LB + D-pad | L1 + D-pad | L + D-pad |
| Menu cursor | — | D-pad | D-pad | D-pad | D-pad |
| Switch target | IJKL | C-stick | Right stick | Right stick | Right stick |
| A | Enter | A | A | Cross | B |
| B | Backspace | B | B | Circle | A |
| X | X | X | X | Square | Y |
| Y | Y | Y | Y | Triangle | X |
| Z | Left Shift | Z (R1) | RB | R1 | R |
| L / R | Q / E | L / R | LT / RT | L2 / R2 | ZL / ZR |
| Pause | Space | Start | Menu | Options | Plus |
| Emulator menu | Esc | — | — | — | — |
| Fullscreen | F11 | — | — | — | — |

The original game ignores the D-pad in menus and uses it for taunts in a
match. The default profile maps the D-pad to the menu cursor, so it navigates
menus out of the box; **L1 + D-pad** still taunts. The stock layout
(D-pad = taunt, menus = left stick) is in
`defaults/GCPadNew.dualsense.ini`.

Two players are supported (port 1 and port 2) for multiplayer modes.

## Supported controllers

The launcher detects every SDL gamepad and writes `GCPadNew.ini` with the
exact device name, so you do not need a pre-made profile. The layout presets
cover:

- Xbox Series, Xbox One, Xbox 360 and generic XInput pads
- Xbox Elite Series 1 and 2 (Microsoft `045E:02E3`, `045E:0B00`, `045E:0B05`,
  `045E:0B22`) — standard Xbox layout; P1-P4 paddles are ignored
- DualSense, DualShock 4 and DualShock 3
- DualSense Edge (Sony `054C:0DF2`) — standard DualSense layout; paddles,
  Fn and touchpad are ignored
- Switch Pro Controller, Joy-Con, Switch Online pads and Wii U Pro Controller
- Steam Deck
- 8BitDo pads in XInput mode or Switch mode
- The GameCube controller adapter
- Keyboard (WASD)

On the **CONTROLLER** page, select the active controller and choose a
**Layout**:

- **Auto-detect** is the default. It swaps A/B and X/Y for Switch, Joy-Con,
  Wii U and other Nintendo-layout names; everything else uses the standard
  Xbox/PlayStation layout.
- **Xbox / PlayStation (standard)** forces the standard layout.
- **Nintendo (A/B swap)** fixes a pad whose face buttons are swapped, such as
  an 8BitDo pad in Switch mode.
- **Keyboard (WASD)** writes the keyboard profile.

Press **Use this controller** to rewrite the profile with the selected layout.
Existing custom profiles are left alone until you do.

The runner logs one line at startup:
`controller: name="…" guid=… layout=… dpad=…`. If a pad is still wrong, send
that line; it identifies the exact model and mode.

Two more lines can appear when something is unusual:

- `controller: no gamepad enumerated (joysticks=N gamepads=0); keeping existing
  GCPadNew.ini` — no pad was visible when the game started (a wireless pad
  still waking up, for example). The existing profile is kept as it is.
- `controller: configured device "…" not found; using "…"` — the pad in the
  profile is not connected, so the first pad that is connected was bound
  instead.

## Save data

The game uses a folder-based memory card (GCI). On first launch it asks to
create System Data; accept once so Exhibition matches can start.

```
%LOCALAPPDATA%\WrestleManiaX8Recomp\GC\USA\Card A\
```

or, in portable mode, `User\GC\USA\Card A\` beside the executable.

To bring saves from an emulator, copy the GCI files from your Dolphin memory
card into that folder. Card images (`.raw`, `.mcd`) are not read directly.

The launcher's PLAY page has a MEMORY CARD card: create the folder card, back
it up to `Backups\CardA-<timestamp>`, import a `.gci`, or open the card folder.

## Save states

Press **F1** in the game window to save a state. The launcher lists, loads,
deletes and reveals states from its StateSaves folder; saving is done in the
runner because the launcher is closed while the game runs. States are not
portable between builds of the recompiled module.

## Known gaps

- 1v1 and Fatal 4 Way are tested end to end. Other match types, every
  entrance and a full match finish are not exhaustively tested.
- Audio is enabled and verified with Cubeb; not every output device was
  listened to.
- Netplay is not part of the tested v0.1.2 scope.
- No texture, music or gameplay mods are included in v0.1.2.
