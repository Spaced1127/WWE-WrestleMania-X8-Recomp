# Canonical controller system (reference implementation)

Status: implemented in WWE WrestleMania X8 Recomp v0.1.4. Port this note and the
patched files verbatim to the other ModernGekko projects.

## Why

The per-device approach carried a profile generator, per-device GUID tables
(DualSense Edge, Xbox Elite), a name heuristic and a live-mapping `a>b` rule.
Every new pad needed a new table entry, and the heuristics misfired:

- the `a>b` rule flipped 219 of the SDL DB's 812 rows (third-party Xbox-shape
  pads, Betop, BUFFALO, Cideko, Steam virtual pad);
- the button-index D-pad rule mirrored every DualSense/DualSense Edge
  (`dpleft:b15,dpright:b13` is correct, not reversed);
- rewriting `GCPadNew.ini` from a scan that enumerated no pad turned a sleeping
  wireless pad into a keyboard profile and dead input.

## Design

1. **ONE static `GCPadNew.ini`.** Canonical SDL positional bindings
   (`Buttons/A = Button A`, ...). Written only when missing (first run).
   Never regenerated or rewritten at launch.
2. **Two layout classes.** `positional` (default) and `nintendo`
   (A<->B and X<->Y so printed labels match). D-pad, sticks and triggers are
   never swapped.
3. **Class chosen once per pad at input init**, strongest signal first:
   a. `SDL_GetGamepadType` == `SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO` /
      `JOYCON_LEFT` / `JOYCON_RIGHT` / `JOYCON_PAIR` (authoritative);
   b. one explicit GUID fallback for 8BitDo Switch-mode clones that report
      `UNKNOWN` (`05ac:0004` GUID words) - the only GUID knowledge left;
   c. user override wins over everything: `controller_class =
      auto|positional|nintendo` in `config.ini`, or `--controller-layout`.
4. **Device resolution without name dependency.** The profile stores a device
   name for the picker, but at input init the runtime checks
   `EmulatedController::IsDefaultDeviceConnected()`; if the configured pad is
   absent it calls `SetDefaultDevice(first enumerated pad)` +
   `UpdateReferences()` and logs the substitution. A watcher thread repeats
   that check every 2 s, so a pad that appears after launch is bound (hot-plug)
   with no profile write and no restart.
5. **No per-device tables, no generation from scans.** The only per-device
   knowledge left is the 8BitDo Switch-mode GUID pair in (3b).
6. **Apple/Google-style fallback is unnecessary:** anything that reports a
   standard type and is not a Switch pad is positional by construction.

## Files (the exact patch)

Dolphin input layer (vendor/dolphin/Source/Core/InputCommon/ControllerInterface/SDL):

| File | Change |
|---|---|
| `LayoutClass.h` (new) | `enum class LayoutClass { Positional, Nintendo }`; header-only override slot (`SetLayoutClassOverride`, `GetLayoutClassOverride`, `LayoutClassName`). No SDL types, so the runtime can include it without linking the input library. |
| `SDLGamepad.h` | includes `LayoutClass.h`; declares `ClassifyGamepad(SDL_Gamepad*)` and `ResolveLayoutButton(SDL_Gamepad*, SDL_GamepadButton)`. |
| `SDLGamepad.cpp` | `Gamepad::Button::GetState()` reads through `ResolveLayoutButton`, which swaps SOUTH<->EAST and WEST<->NORTH when the pad's class is Nintendo. `ClassifyGamepad` = override, then SDL type, then the 8BitDo Switch-mode GUID pair. |

Runtime (`src/runtime/dolphin_runtime.cpp`, `include/moderngekko/runtime.hpp`):

| File | Change |
|---|---|
| `runtime.hpp` | `RuntimeConfig::controller_class = "auto"` (+ docs). |
| `dolphin_runtime.cpp` | applies the class override right after `UICommon::InitControllers(wsi)`; `EnsurePadBound()` binds the first pad when the configured one is absent (logs `controller: configured device missing; bound "..."`); a 2 s `std::jthread` watcher repeats it for hot-plug and is joined on shutdown. |

Host tools (`tools/`):

| File | Change |
|---|---|
| `frontend_config.hpp/.cpp` | `ControllerClass {Auto, Positional, Nintendo}`, `ParseControllerClass`, `ControllerClassName`, `CanonicalPadProfile`, `EnsureCanonicalPadProfile` (write-once), `ReadConfiguredControllers`, `EnsureMemoryCardSlots`. All layout detection, GUID tables, D-pad swap and profile-rewrite logic removed. |
| `moderngekko_run.cpp` | `EnsureControllerSetup`: hints, pad scan for the log + first-pad choice, write-once profile, `RuntimeConfig::controller_class`, startup line `controller: name="..." guid=... class=... dpad=normal profile=static|missing`. No rewrite, no `SDL_Quit`. |
| `moderngekko_launcher.cpp` | three-entry class picker (Auto / Positional / Nintendo) writing `controller_class`; "Use this controller" is the only path that (re)writes the canonical profile. |
| `netplay_session.cpp` | lobby controller toggles use `EnsureCanonicalPadProfile`. |

Tests (`tools/controller_layout_test.cpp`): class parsing and precedence,
canonical profile contents (positional A/B/X/Y, canonical D-pad, no shoulder
gating), write-once behaviour, configured-device read-back, `controller_class`
round-trip and legacy `controller_layout` acceptance, memory-card slot keys.

## Verification checklist (real runs, visible windows)

Per pad: boot, read the startup line, confirm `class=positional` for Xbox
360/One/Series/Elite, DualSense, Edge, DS4/DS3, generic DInput; `class=nintendo`
for Switch Pro, JoyCon, 8BitDo in Switch mode; `class=positional` for 8BitDo in
XInput mode. Then press A/B/X/Y and confirm the on-screen action matches the
printed label for the class.

Also check: pad absent at boot (keyboard profile kept, no rewrite), pad
plugged in after launch (log line, input works), `controller_class=nintendo`
forces the swap on a positional pad, and the profile file is byte-identical
across launches.

## Known limits

- Pads that expose A/B/X/Y as raw buttons without a recognised type are
  positional by definition; the override covers them.
- Two pads with different classes on the same machine are supported because the
  class is per pad, but the profile binds port 1 only.
