# v0.1.4 — generic controller system

Replaces the per-device controller profiles with one canonical system, shared
with the other ModernGekko projects. Design note:
`docs/notes/controller-generic-design.md`.

## Controller

- **One static `GCPadNew.ini`.** Canonical SDL positional bindings
  (A = south, B = east, X = west, Y = north). Written once on first run and
  never regenerated or rewritten at launch, so a sleeping or late wireless pad
  can no longer be turned into a keyboard profile.
- **Two layout classes.** `positional` (default) and `nintendo`
  (A<->B and X<->Y so printed labels match). D-pad, sticks and triggers are
  never swapped, so the DualSense/Edge D-pad mirroring bug is gone by
  construction.
- **Class chosen once per pad from the SDL gamepad type** (Switch Pro, Joy-Con,
  Switch classic clones), with one explicit 8BitDo Switch-mode GUID fallback
  for clones that report `UNKNOWN`. No per-device GUID tables, no `a>b`
  heuristic, no name guessing.
- **Override wins**: `controller_class = auto | positional | nintendo` in
  `config.ini`, the CONTROLLER page, or `--controller-layout`.
- **Device resolution without name dependency**: if the configured pad is
  absent the runtime binds the first enumerated pad and logs
  `controller: configured device missing; bound "..."`. A 2 s watcher binds a
  pad that is plugged in after launch (hot-plug), with no profile write.
- Startup line kept and simplified:
  `controller: name="..." guid=... class=auto|positional|nintendo dpad=normal
  profile=static|missing`.

Removed: `DetectControllerMapping`, the DualSense Edge / Xbox Elite tables, the
blanket `a>b` rule, the D-pad button-index correction, profile generation at
launch and the layout migration logic.

## Memory card (carried over from v0.1.3)

Raw memory card in slot A (`Core/SlotA = 1`), empty slot B (`SlotB = 255`),
`GC/<region>` created before EXI init, forced in the runtime and in
`Config/Dolphin.ini` for existing installs. Save and load were verified on
v0.1.3 and are unchanged here.

## Verification

- Offline matrix (`tools/controller_layout_test.cpp`): class parsing and
  precedence, canonical profile contents, write-once behaviour, configured
  device read-back, `controller_class` round-trip, legacy `controller_layout`
  acceptance, memory-card slot keys.
- Real runs: profile byte-identical across launches (`profile_unchanged`), custom
  profile preserved, `--controller-layout nintendo` reports
  `class=nintendo`, no-pad boot keeps the keyboard profile, boot holds
  59.94 fps.

## Not verified (honest)

- No physical pad was attached during this build's verification, so per-pad
  press tests and the hot-plug checklist in the design note were not run here.
  `class=positional` for Xbox/DualSense/Elite and `class=nintendo` for Switch
  pads follow from the SDL type, which the previous release did confirm live
  (Xbox One `layout=standard dpad=normal`, DualSense same).
- Two pads with different classes on one machine: class is per pad, but the
  profile binds port 1 only.
