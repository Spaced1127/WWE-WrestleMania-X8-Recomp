# Controller profiles

These are Dolphin-format `GCPadNew.ini` profiles. The runtime reads
`<user-dir>\Config\GCPadNew.ini`.

| File | Layout | Device string |
|---|---|---|
| `GCPadNew.dualsense.ini` | PlayStation | `SDL/0/DualSense Wireless Controller` |
| `GCPadNew.dualsense-menu.ini` | PlayStation, D-pad = menus | `SDL/0/DualSense Wireless Controller` |
| `GCPadNew.dualshock4.ini` | PlayStation | `SDL/0/PS4 Controller` |
| `GCPadNew.dualshock3.ini` | PlayStation | `SDL/0/PS3 Controller` |
| `GCPadNew.xbox.ini` | Xbox | `SDL/0/Xbox Wireless Controller` |
| `GCPadNew.xbox360.ini` | Xbox | `SDL/0/Xbox 360 Controller` |
| `GCPadNew.steamdeck.ini` | Xbox | `SDL/0/Steam Deck Controller` |
| `GCPadNew.8bitdo.ini` | Xbox (XInput mode) | `SDL/0/8BitDo Pro 2` |
| `GCPadNew.switch-pro.ini` | Nintendo (A/B and X/Y swapped) | `SDL/0/Nintendo Switch Pro Controller` |
| `GCPadNew.joycon.ini` | Nintendo | `SDL/0/Joy-Con (L)` |
| `GCPadNew.wiiu-pro.ini` | Nintendo | `SDL/0/Wii U Pro Controller` |
| `GCPadNew.gamecube.ini` | Standard | `SDL/0/GameCube Controller Adapter` |
| `GCPadNew.generic.ini` | Standard | `SDL/0/Gamepad` |
| `GCPadNew.keyboard.ini` | Keyboard | `DInput/0/Keyboard Mouse` |

The launcher is the reliable path for any controller: its CONTROLLER page
lists every SDL gamepad plus **Keyboard (WASD)**, writes the exact SDL device
name into `GCPadNew.ini`, and offers a **Layout** selector:

- **Auto-detect** swaps A/B and X/Y for Switch, Joy-Con, Wii U and other
  Nintendo-layout names. Everything else uses the standard Xbox/PlayStation
  positional layout.
- **Xbox / PlayStation (standard)** forces the standard layout.
- **Nintendo (A/B swap)** forces the Nintendo layout; use it for an 8BitDo
  pad in Switch mode or any controller whose face buttons are swapped.
- **Keyboard (WASD)** writes the keyboard profile.

The files above are templates. If one does not produce input, its `Device =`
line does not match the name SDL reports; let the launcher generate the
profile instead.

The device name is whatever SDL reports; common examples are
`SDL/0/DualSense Wireless Controller`, `SDL/0/Xbox Series X Controller`,
`SDL/0/Nintendo Switch Pro Controller` and `SDL/0/Pro Controller`.

All other lines are generic: they map the emulated GameCube pad to SDL gamepad
controls (`Button A/B/X/Y`, `Pad N/S/W/E`, `Left X/Y`, `Right X/Y`,
`Trigger L/R`).

## D-pad: menus vs taunts

WrestleMania X8's menus read **only the main stick**. The D-pad is a taunt in a
match. You cannot have both on the same physical inputs, so there are two
modes:

### Normal profile

```
Main Stick/Up  = `Left Y+` ...
D-Pad/Up       = `Pad N` ...
```

Menus: left stick only. Match: D-pad = taunt 1-4.

### Menu profile (default, `GCPadNew.dualsense-menu.ini`)

```
Main Stick/Up  = `Pad N` | `Left Y+`
D-Pad/Up       = `Pad N` & `Shoulder L`
```

Menus: D-pad or left stick. Match: D-pad moves, **L1 + D-pad taunts**.
The L1 modifier is otherwise unused by the GameCube pad mapping.

This is the default for `tools\play.ps1`. Use `-StockDpad` for the normal
layout (D-pad = taunt, menus = stick only).

## Controls in a match

| Input | Action |
|---|---|
| Left stick | move |
| Right stick (C-stick) | switch target |
| D-pad | taunt 1-4 (normal) / move (menu profile) |
| A / B / X / Y | strikes and grapples |
| L / R | block / dodge |
| Start | pause |
