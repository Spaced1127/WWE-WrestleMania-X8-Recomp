# ModernGekko overlay

These are the local files changed for this port. The upstream
`ModernGekko-Template` checkout is not committed (`lib/` is git-ignored), so
this folder carries the launcher, controller and runtime changes needed to
reproduce the release.

Copy the files over a matching template checkout:

```powershell
Copy-Item overlay\ModernGekko\* lib\ModernGekko-Template\lib\ModernGekko\ -Recurse -Force
```

| Overlay file | Purpose |
|---|---|
| `tools/moderngekko_launcher.cpp` | WMX8 theme, parity pages, diagnostics, background loading, controller layout UI |
| `tools/moderngekko_run.cpp` | RecompLauncher controller auto-provisioning, layout selection and keyboard fallback |
| `tools/frontend_config.cpp` / `.hpp` | `controller_layout` config, SDL pad detection, Nintendo A/B swap, keyboard profile |
| `src/runtime/game.cpp` | `InspectGame` no longer hashes the extracted `files/` tree |
| `vendor/dolphin/module-template/CMakeLists.txt` | `-march=x86-64-v3`, loop budget 4096, call depth 128 |
| `resources/icons/*` | SteamGridDB icon, 7-size ICOs and 1024 px PNGs |

The launcher background is `resources/background.png` at the repository root.
It is a blurred derivative of the SteamGridDB hero; the raw art is not
committed. See `docs/CREDITS.md`.

These files are GPL-3.0 like the runtime they patch. The icon/hero artwork is
third-party promotional art and remains the property of its creators.
