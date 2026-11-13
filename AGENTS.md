# AGENTS.md — WWE WrestleMania X8 recomp

Rules and context for any agent working in this project. Read
`docs/STATE.md` first; it holds the live inventory and evidence paths.

## Hard rules

- **No game data** in Git, releases, fixtures or docs: no ISO/RVZ/GCM/WBFS,
  DOL, FST, `work/game/`, textures, audio, saves or cover art.
- **Do not edit generated C** (`build/modules/**/generated/**`). Fix the
  emitter/runtime and regenerate.
- **Do not clean build trees** unless the user explicitly asks. Module builds
  and PGO profiles are expensive.
- **No commit, tag, release or push** without explicit user approval.
- **No claim without evidence.** Boot it, load a real state, read
  `work/auto/status.txt`, inspect `work/evidence/*.png`.
- **Never interfere with the user's desktop.** No synthetic mouse or keyboard
  input, no focus stealing, and no moving/resizing windows the agent did not
  create. Use `--automation-dir`, headless `--extract`, hidden windows or
  window-only captures. The user may be working; ask before any test that
  must take over the screen.
- **Stop after three failed variations** of the same fix; change approach.

## Project facts

| Item | Value |
|---|---|
| Disc ID | `GW3E78` (USA) |
| DOL SHA-256 | `91af3136f2a48c18a6b96a5726d1f171c9e714bb8d4d504f835070fc230a303e` |
| Entry | `0x800051fc` |
| Engine | Yukes `adm` layout |
| Runtime | ModernGekko (Dolphin-derived) |
| Module | `gGW3E78_recomp.dll` |

The built tools live in the SpongeBob project checkout:
`H:\Projects\SB-RFD\build\moderngekko\` (`dolrecomp.exe`,
`moderngekko-port.exe`, `moderngekko-run.exe`). Clang is at
`C:\Program Files\LLVM\bin`, Ninja under VS Build Tools 18.

## Verified flow

Boot → legal → THQ → JAKKS → intro → title → Start → main menu → Exhibition
→ Single → Normal Match → rules OK → character select → arena select → A →
NOW LOADING → arena → entrance → in-ring match.

System Data is read from Memory Card Slot A. The created save is
`work/user-3/GC/USA/Card A/78-GW3E-__2f__w_mania2002.gci`.

## Input / automation

Use `--automation-dir work\auto`. Commands are `key=value` files in
`work\auto\commands/`. Status is `work\auto/status.txt`.

- `command=pad_frames`, `port=0`, `frames=3`, `a=1` (also `start`, `b`, `x`,
  `y`, `z`, `l`, `r`, `l_analog`, `r_analog`, `dpad_*`, `main_x/y`, `c_x/y`).
- `command=pad` / `command=clear_pad` for held input.
- `command=screenshot`, `path=<abs>` writes the guest framebuffer.
- `command=save_state` / `command=load_state`, `path=<file>`.
- `command=read_memory` / `write_memory`, `command=stop`.

Known quirks:

- The first pad command after `--load-state` is swallowed.
- On the arena screen, long holds are ignored; **short A taps every 3 s**
  start the match. Do not use the all-buttons combo: it includes B and backs
  out into the attract demo.
- Synthetic keyboard events (`keybd_event`, `SendKeys`) do not reach the game.

## Benchmarks

`tools/bench.ps1 -Label <name> -Module <dll> -Core static|jit` loads
`work/states/match.sav`, warms up 35 s, reads `status.txt`, appends to
`docs/benchmarks.csv`.

Baseline 2026-11-09: MSVC `-O2` module, in-match = **40.7 fps / speed 0.674**
with the user's browser open. Menus = 59.94 fps.

## Performance order

1. Clang `-O3` module (guide: 27.8 → 46.8 fps on SpongeBob).
2. `DOLRECOMP_C_LOOP_CYCLE_BUDGET=4096`, `DOLRECOMP_C_MAX_CALL_DEPTH=128`
   compile definitions.
3. PGO on real gameplay (`moderngekko-port pgo-run`).
4. `-march=x86-64-v3` for the shipped build, native for local testing.
5. Only then consider runtime work (idle loop, helpers).
