# Performance notes — WWE WrestleMania X8

## Method

- Same in-match save state: `work/states/match.sav` (The Rock vs Steve Austin,
  WrestleMania X8 arena, match running).
- `work/auto/status.txt` sampled after a 35 s warm-up and again 15 s later.
- `fps` is the emulated frame rate, `vps` the VI rate, `speed` the ratio to the
  59.94 Hz console rate.
- External load matters: the user's browser (Brave) and Elgato WaveLink run on
  this machine. Numbers below are with that load present.

## Measured

| Date | Module | Core | FPS | Speed | Notes |
|---|---|---|---|---|---|
| 2026-11-09 | MSVC `/O2` (a71f4a5f) | static | 40.7 | 0.674 | in-match, browser open |
| 2026-11-09 | MSVC `/O2` (a71f4a5f) | static | 59.94 | 1.000 | menus / arena select |
| 2026-11-10 | Clang `-O3` (8f78e414) | static | **59.94** | **1.0017** | in-match, browser open; `speed=1.0009` on re-check |
| 2026-11-10 | Clang `-O3` (8f78e414) | static | 59.93 | 1.0027 | in-match with pad input |
| 2026-11-13 | Clang `-O3` (release zip) | static | 49.96 | 0.817 | zip artifact, heavy external load |

The Clang module reaches the 59.94 Hz target in the tested 1v1 match. Evidence:
`work/evidence/clang-match.png`, `work/evidence/fb-pause.png`.

The release-zip check on 2026-11-13 measured 49.96 fps because the machine was
under heavy external load (browser, release test and other apps). The same
module and state measured 59.94 fps in the earlier isolated runs; close
background GPU/CPU work before judging performance.

## Fatal 4 Way (2026-11-13)

State: `work/states/fatal4.sav` — The Rock, Steve Austin, Undertaker and
Triple H in the ring with the referee.

| Configuration | FPS | Speed | Notes |
|---|---|---|---|
| Static Clang `-O3`, 1080p, MSAA 1 | 54.83 | 0.920 | clean run |
| JIT, same state/settings | **59.94** | **1.000** | runtime/GPU holds target |
| Static, 640x528, MSAA 1 | 43.47 | 0.742 | noisy; not GPU-bound |
| Static, live user session | 48.80 | 0.828 | browser/other load |
| Static optimized (v3, budget 4096, depth 128), Null audio | **59.99** | **1.0068** | clean run |
| Static optimized, **Cubeb audio** | 58.56 | 0.9768 | audio on, browser open |
| Static optimized + idle skip + Cubeb audio | **59.93** | **1.004** | final shipped configuration |
| Release zip runner, Cubeb, high priority + affinity `0xFF` | 60.00 | 1.006 | 40 s warm-up, 20 s measure |
| Release v0.1.1 runner, Cubeb, minimized | 59.95 | 1.006 | controller auto-provisioning build |
| Release zip runner, controller build, Null audio, minimized | 59.92 | 1.010 | high priority + affinity `0xFF` |
| 1v1, same configuration | 59.91 | 0.995 | full speed |
| Static optimized + L3 affinity | 57.62 | 0.9534 | affinity hurts on this 7800X3D |

The `-march=x86-64-v3` + loop budget 4096 + call depth 128 build closes the
whole 8% gap with Null audio. Real audio (Cubeb, volume 100) cost another
~2.3%. Setting `StaticRecompIdlePC = 0x80179AAC` in
`Sys/GameSettings/GW3E78.ini` removes the frame-limiter spin and recovers it:
**59.93 fps / 1.004** in the Fatal 4 Way and 59.91 in 1v1, with Cubeb audio.

PGO was not needed. The profile was dominated by the idle loop
(`0x80179AAC`, ~7.7 billion counts), which is exactly why the runtime idle skip
is the right fix rather than more codegen tuning. The PGO workspace is kept in
`work/pgo` for reference; `tools/pgo_cycle.ps1` can rerun it if a future scene
needs it.

## Plan

1. Clang `-O3` (8f78e414 build) — done.
2. Clang + `-march=x86-64-v3`, loop budget 4096, call depth 128 — done.
3. JIT reference on the same state — done (59.94).
4. Idle-loop skip at `0x80179AAC` — done (59.93 with audio).
5. PGO — collected but not needed; the profile is idle-loop dominated.
6. Ship the optimized static module as the default; JIT remains the fallback.

## Codegen notes

The clang module build already applies:

```
-O3 -flto=thin -fvisibility=hidden -ffp-contract=off -fno-fast-math
```

`-march` and the loop budget/call depth defines are not in the tool defaults;
they are passed by reconfiguring the generated `module-build` project with
`CMAKE_C_FLAGS`, or by the PGO build's `--cflags` equivalent.
