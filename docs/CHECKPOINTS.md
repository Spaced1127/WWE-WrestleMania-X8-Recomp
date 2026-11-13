# Checkpoints and how to reproduce

All input goes through the automation protocol. The runner is
`H:\Projects\SB-RFD\build\moderngekko\moderngekko-run.exe`.

## Fastest path into a match

```powershell
$module = (Get-Content build\modules\GW3E78\active-module.txt -Raw).Trim()
& 'H:\Projects\SB-RFD\build\moderngekko\moderngekko-run.exe' `
  --game "work\game" --module "$module" --user-dir "work\user-3" `
  --graphics Vulkan --audio Null --title WMX8 `
  --automation-dir "work\auto" --load-state "work\states\matchup.sav"
```

Wait 20 s, then write short A taps:

```
work\auto\commands\a01.txt:
command=pad_frames
port=0
frames=3
a=1
```

Repeat every 3 s. Around the 10th-14th tap the screen shows `NOW LOADING`,
then the arena, entrance and the live match. A clean in-match state is
`work/states/match.sav`; load that to skip the sequence entirely.

## From a cold boot

1. Start the runner with `--automation-dir work\auto` and no state.
2. After ~50 s answer the System Data prompts with A taps.
3. The intro plays; press Start at the title screen (`start=1`, 60 frames).
4. Main menu: A → Exhibition, A → Single, A → Normal Match, A → OK.
5. Character select: A → The Rock, A → Steve Austin.
6. Arena select: A taps until `NOW LOADING`.
7. In-match: save with `command=save_state`.

## Save states

| Path | Screen |
|---|---|
| `work/states/menu.sav` | title `PRESS START` |
| `work/states/mainmenu.sav` | main menu |
| `work/states/charsel.sav` | empty character grid |
| `work/states/matchup.sav` | arena select, Rock vs Austin |
| `work/states/match.sav` | live in-ring match |

## Screenshots

```
command=screenshot
path=<project>\work\evidence\name.png
```

Screenshots capture the guest framebuffer, not the desktop, so they work even
when the window is behind another application.

## Memory card

`work/user-3/GC/USA/Card A/78-GW3E-__2f__w_mania2002.gci` holds the created
System Data. Use `work/user-3` for any run that should keep it.
