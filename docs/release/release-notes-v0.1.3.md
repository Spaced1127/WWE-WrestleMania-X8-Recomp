# v0.1.3 — controller hotfix

Fixes the controller regressions reported after v0.1.2. No game data is
included; the disc, extraction and module are unchanged.

## Xbox controllers stopped responding

v0.1.2 added a pad scan before the game starts so a fresh install gets a
`GCPadNew.ini` without opening the launcher. That scan also rewrote the profile
when it found **no** pad. A wireless Xbox pad that is still waking up, a pad on
a hub that enumerates late, or a pad the driver has not published yet therefore
turned the profile into a keyboard profile; the pad connected a second later
and the game never saw it.

- The profile is now only written when at least one gamepad was enumerated. With
  no pad the runner logs `controller: no gamepad enumerated (joysticks=N
  gamepads=0); keeping existing GCPadNew.ini` and leaves the file untouched.
  A fresh install with no pad still gets the keyboard profile so the game is
  playable.
- If the pad named in the profile is missing but another pad is connected, the
  runner binds the connected pad and logs the substitution instead of leaving
  dead input.

## Scan and runtime now use the same SDL driver set

The early scan initialised SDL video+gamepad and then called `SDL_Quit()`.
Windows pads reach SDL through four different drivers (XInput, Windows.Gaming
Input, DirectInput, HIDAPI) and the name SDL reports differs per driver; the
profile stores that name verbatim. The scan now applies the same hints
Dolphin's SDL backend applies (`SDL_HINT_JOYSTICK_DIRECTINPUT`,
`SDL_HINT_JOYSTICK_HIDAPI_GAMECUBE`, the Joy-Con combine hints), initialises
only joystick/haptic/gamepad, and leaves those subsystems running so the
runtime reuses the same enumeration instead of re-initialising a second time.

## Face-button layout

The v0.1.2 rule read the live SDL mapping for every pad and switched to the
Nintendo layout whenever A's button index was above B's. That is true for
8BitDo's own rows, but also for 219 of the SDL DB's 812 rows — Betop, BUFFALO,
Cideko, Acteck, Defender and the Steam virtual pad among them. The rule now
stops at the 8BitDo family (name or 8BitDo vendor GUID), where the DB is
consistent: Pro 2/SN30/FC30/M30-Bluetooth are Nintendo-ordered, Ultimate/64 are
standard. Everything else keeps the standard layout unless it is a Nintendo
name or a Nintendo/8BitDo-Switch GUID.

## D-pad left/right correction

The same rule treated "D-pad left has a higher button index than D-pad right" as
a reversed D-pad. The DualSense family binds `dpleft:b15,dpright:b13`, which is
correct, so every DualSense and DualSense Edge profile got its D-pad mirrored.
Button-index correction is now limited to 8BitDo pads; the hat-mask check
(`h0.2`/`h0.8`) still applies to everyone because it is exact.

## Memory card: raw card in slot A, empty slot B

Same defect as the other ModernGekko projects: the runtime never configured the
EXI slots, so `Core/SlotA` stayed at Dolphin's default `8`
(`MemoryCardFolder`) and `UICommon::CreateDirectories()` was never called. The
GCI folder needs `GC/<region>/Card A` to exist, and the slot-A probe can hang
at "checking memory card in slot A".

- `Runtime::Create` now mirrors DolphinQt: `SetUserDirectory` →
  `CreateDirectories()` → `Init()`, then forces `Core/SlotA = 1` (raw card) and
  `Core/SlotB = 255` (None) and saves them. `SlotB = 0` is the Dummy EXI device
  and the game rejects it.
- The launcher and the runner also write both keys into
  `Config/Dolphin.ini` (overwriting, not just inserting), so existing installs
  are corrected on the next start, launcher-only starts included.
- `recomplauncher.json` carries `Core/SlotA`/`Core/SlotB` in `ensure`, `always`
  and `new`.
- The launcher's MEMORY CARD page now shows and backs up
  `GC/MemoryCardA.USA.raw`. "Import .gci" is disabled with an explanation:
  single-file import needs the folder device, which is no longer used.

Verified on a fresh user dir: `GC/USA`, `GC/EUR`,
`GC/JAP` and the rest of the Dolphin tree are created, `GC/MemoryCardA.USA.raw`
appears at 16,777,216 bytes, `Config/Dolphin.ini` starts with
`[Core] SlotA = 1 / SlotB = 255`, and no `GC/USA/Card A` folder is created.

Save and load were then verified on the shipped build: answering YES to the
game's "Do you want to create System Data?" wrote the card
(SHA-256 `8A2C41E9…` formatted-empty to `A1A47B2E6578D70E0ACC946E8214C2464030C2A83CAAED9DAA41EA2FCB3EB4FE`),
directory entry `GW3E-78 "/w_mania2002"` with 14 blocks at block 5, block
allocation updated from 2043 free / last 4 to 2029 free / last 18. A clean
shutdown (`command=stop`, which flushes the card) left the hash unchanged, and
the next boot reported **"Load complete."** instead of asking to create data.
Savestates from the folder era re-enable the folder device and must not be used
for card testing.

## Other

- Xbox Elite `045E:0B22` added to the Elite table.
- Startup diagnostic line kept: `controller: name="…" guid=… layout=… dpad=…`.
- Offline layout matrix: Xbox 360 / One / One S / Series X / Elite S1 / Elite S2
  (USB and Bluetooth rows), DualSense, DualSense Edge, 8BitDo Pro 2 (standard
  and Switch mode), Ultimate, 64, Micro, Switch Pro, plus non-8BitDo rows that
  must stay standard. The empty-scan and missing-device rewrite rules are
  covered by the same test binary.

## Verification

- `work/ctl-test/layout_test.exe`: all layout, D-pad and rewrite cases pass.
- Live pads on the shipped build: Xbox One Controller
  (`guid=0300fa675e040000ff02000000007801`) → `layout=standard dpad=normal`,
  existing profile reused; DualSense Wireless Controller
  (`guid=030057564c050000e60c000000016800`) → `layout=standard dpad=normal`
  (the D-pad mirroring bug would have shown `dpad=swapped`).
- Run with an existing Xbox profile and no pad attached: profile untouched, the
  no-gamepad guard logged instead of a keyboard rewrite.
- Fresh user dir, no pad: keyboard profile written, module loaded,
  `booted=1 fps=59.94`.

## If a controller is still wrong

Start the game once and send us the startup line from the log
(`Logs\WrestleManiaX8.log`):

`controller: name="..." guid=... layout=... dpad=...`

That line names the exact model, mode and layout that was chosen, so a report
turns into a fix. Two extra lines may appear and are also useful:
`no gamepad enumerated (...)` and `configured device "..." not found`.

## Verification status (honest)

- Controllers: verified offline against the real SDL controller DB (Xbox 360,
  Xbox One, One S, Series X, Elite S1/S2, DualSense, DualSense Edge, 8BitDo
  Pro 2 both modes, Ultimate, 64, Micro, Switch Pro) and live on this build
  with a physical Xbox One Controller and a DualSense, both detected as
  `layout=standard dpad=normal`. Physical button presses on your own pad are
  still yours to confirm - if something is off, the startup line above is what
  we need.
- Memory card: fresh-install creation of `GC/<region>` and
  `GC/MemoryCardA.USA.raw` (16,777,216 bytes), a game-written save
  (`GW3E-78`, 14 blocks, card SHA before/after), a flush-safe clean shutdown,
  and a reload that reports "Load complete." are all verified on the shipped
  build.
- Boot: shipped artifact holds 59.94 fps and stops cleanly.

## SHA-256

See `WrestleManiaX8Recomp-v0.1.3-SHA256SUMS.txt` attached to this release.
