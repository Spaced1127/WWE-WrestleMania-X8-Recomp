WWE WrestleMania X8
Standalone PC port (static recompilation) - Windows x64 - v0.1.2
================================================================

This package contains NO game data. There is no disc image, no ROM, no
textures, no audio and no level data in these files. The recompiled module is
program code only; the art, sound and levels are read at runtime from the game
files you supply.


WHAT YOU NEED
-------------
An original GameCube copy of "WWE WrestleMania X8" (USA, disc ID GW3E78),
either as:

  * a .iso image (1.36 GiB, clean 1:1 dump), or
  * a .rvz image (Dolphin compressed format).

Both are imported directly by this app. RVZ is read natively through
Dolphin's DiscIO; there is no RVZ-to-ISO conversion step. Other regions and
other games are rejected.


QUICK START
-----------
1. Put this folder anywhere you have write access (for example
   C:\Games\WrestleManiaX8Recomp). Do not put it in Program Files: the
   extracted game is written into a "Game" folder next to the executable.

2. Run WrestleManiaX8Recomp.exe.

3. Click "Browse for ISO / WBFS / RVZ", pick your disc image, then click
   "Extract and Play". The disc is extracted once into a "Game" folder beside
   the executable and remembered in default-game.txt.

   If you already extracted the disc elsewhere, click "Browse for extracted
   game folder" and pick the folder that contains "sys" and "files".

4. Click Play. The game window opens. Start pauses in a match; Esc opens the
   emulator menu.

The extracted folder needs about 1.4 GB. Re-importing the same disc reuses the
existing extraction.


SETTINGS
--------
All settings are stored in config.ini in
%LOCALAPPDATA%\WrestleManiaX8Recomp\ (or beside the executable in portable
mode) and persist between sessions. The launcher reads and writes them; you do
not need to edit the file by hand.

The launcher has five pages: PLAY, GRAPHICS, PERFORMANCE, AUDIO and
CONTROLLER. Everything is saved automatically.

THEME / ARTWORK
---------------
The launcher ships with the WrestleMania X8 theme in theme.ini: title, gold
accent and a blurred backdrop in resources\background.png. The application
icon and backdrop are community artwork from SteamGridDB (Macho Sadness); see
THIRD_PARTY.md. The game itself still comes from your disc.

Launcher tools:

  Memory Card   Create the folder card, back it up, import a .gci save, or
                open the card folder.
  Savestates    List and load a state, delete a state, or open the state
                folder. Press F1 in game to save a state.
  Host checks   Creates Cache\Shaders, sets High priority and physical-core
                affinity, and reports the DSP ROM status and audio mute state.
  Diagnostics   Open the log folder, copy the log tail, and show the runner,
                module and last [video] line.

  Graphics backend        Vulkan (recommended) or OpenGL
  Internal resolution     640x528 (1x) up to 7680x4320 (12x)
  Anti-aliasing (MSAA)    Off / 2x / 4x / 8x, plus SSAA
  Anisotropic filtering   1x / 2x / 4x / 8x / 16x
  VSync                   on / off
  Aspect ratio            Auto (game native) / 16:9 / 4:3 / Stretch
  Widescreen hack         stretches 3D to the chosen aspect
  Frame rate              Fixed 59.94 Hz (console timing)
  Audio                   Mute all audio, master volume 0-100%
  2D compatibility        EFB access, ON by default
  Show FPS in title       on / off
  Fullscreen, controller profile, netplay host/join


CPU CORE
--------
PERFORMANCE page.

  * Static recomp (default) runs gGW3E78_recomp.dll, the Clang -O3 build.
    It holds 59.94 fps / speed 1.00 in the tested 1v1 match.
  * JIT fallback uses Dolphin's JIT core and ignores the static module. Use it
    only if a scene misbehaves in static mode.

Both modes read the same extracted game files. The static module ships in this
package and can be selected at any time.


CONTROLS (default)
------------------
  Left stick              move / menu cursor
  D-pad                   menu cursor (L1 + D-pad = taunt 1-4)
  Right stick (C-stick)   switch target
  A / B / X / Y           strikes and grapples
  L / R                   block / dodge
  Start                   pause
  F11                     toggle fullscreen
  Esc                     open/close the emulator menu

Controllers are auto-detected: Xbox, DualSense/DualShock, Switch Pro/Joy-Con,
Steam Deck, 8BitDo, the GameCube adapter and most SDL gamepads. The CONTROLLER
page has a Layout selector (Auto, Xbox/PlayStation, Nintendo, Keyboard) and
writes the exact SDL device name. Keyboard (WASD) is always listed.

The default profile maps the D-pad to the menu cursor because the original
game ignores the D-pad in menus. In a match, L1 + D-pad still triggers taunts
1-4. The stock layout (D-pad = taunt only, menus = left stick) is
`defaults/GCPadNew.dualsense.ini`; copy it over your user profile if you
prefer it.


2D COMPATIBILITY (EFB ACCESS)
-----------------------------
Leave this ON. The legal notice, the title screen, the menus and the match HUD
use the GameCube's embedded framebuffer and render black without it.


FULLSCREEN / SHUTDOWN
---------------------
Fullscreen is borderless. The window is created windowed and switches to the
monitor after the graphics backend is ready, so Alt+F4 closes cleanly. The
runner waits up to 3 seconds for the game's power-button handler and then
exits even if the game does not handle it.


FRAME RATE
----------
The game runs at the console's 59.94 Hz timing. Speed is clamped to 1.0; there
are no 150% / 200% / unlocked options.


FILES
-----
  WrestleManiaX8Recomp.exe          launcher, importer and settings
  WrestleManiaX8RecompRunner.exe    game process
  gGW3E78_recomp.dll                static-recomp core (no assets)
  Sys\                              graphics shaders and runtime tables
  LICENSES\                         third-party licenses
  config.ini                        created on first run
  Game\                             created when you import your disc


SAVES
-----
The game uses a folder memory card (GCI). On first launch the game offers to
create System Data; accept it once so Exhibition matches can start. The card
lives in:

  %LOCALAPPDATA%\WrestleManiaX8Recomp\GC\USA\Card A\

or in User\GC\USA\Card A\ in portable mode. Back up that folder.


TROUBLESHOOTING
---------------
"expected disc ID GW3E78; selected ..."
    The image is a different game or a different region.

"game root is not a readable directory"
    The selected extracted folder is wrong. Pick the folder that directly
    contains "sys" and "files".

"main DOL does not match the pinned release"
    The extracted program file is not the retail build this port was compiled
    for. Re-extract from a clean, unmodified disc image.

The launcher window is black.
    Update your graphics driver. The launcher uses a software renderer and does
    not need a GPU; the game window needs Vulkan or OpenGL 4.4.

Low frame rate
    Lower the internal resolution, turn off MSAA/SSAA, and check the GPU
    driver. The default settings target 59.94 fps at 1080p.

No controller input
    Open the launcher's CONTROLLER page, pick the active controller and press
    Use this controller. If the face buttons are swapped, choose Nintendo
    (A/B swap). Keyboard (WASD) is always available. Click the game window
    once; background input is off by default.

D-pad does nothing in menus
    That is the original game: menus read only the left stick. Use the menu
    D-pad profile, or move the left stick.


KNOWN LIMITATIONS (v0.1.2)
--------------------------
  * 1v1 and Fatal 4 Way are tested end to end. Battle Royal, every entrance
    and a full match to a finish are not exhaustively tested.
  * Audio is enabled and uses Cubeb, but was not listened to on every output
    device.
  * No texture, music or gameplay mods are included.
  * Save states are build-specific and are not portable between module builds.
