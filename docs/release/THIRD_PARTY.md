# Third-party software

This port is built on the following projects. Their licenses apply to their
code; no game data is included.

| Component | License | Source |
|---|---|---|
| Dolphin (runtime base) | GPL-2.0-or-later | https://github.com/dolphin-emu/dolphin |
| ModernGekko | GPL-3.0-or-later | https://github.com/ExpansionPak/ModernGekko |
| DolRecomp | GPL-3.0 | https://github.com/ExpansionPak/DolRecomp |
| SDL3 | Zlib | https://github.com/libsdl-org/SDL |
| Dear ImGui | MIT | https://github.com/ocornut/imgui |
| fmt | MIT | https://github.com/fmtlib/fmt |
| zstd, zlib-ng, libpng, libjpeg-turbo, LZ4, mbedTLS, curl | their own licenses | vendored under Dolphin |

Full license texts are included in the `LICENSES` folder of the release.

## Launcher artwork

The launcher icon and backdrop use community artwork from
[SteamGridDB](https://www.steamgriddb.com) for the game entry *WWE WrestleMania
X8* (SteamGridDB id 5295541). The artwork is not extracted from the game disc;
it is downloaded promotional art and remains the property of its respective
owners.

| Asset | Artist | Used for |
|---|---|---|
| Icon 1024x1024 | Macho Sadness | application icon |
| Hero 3840x1240 | Macho Sadness | blurred launcher backdrop |
| Logo 1280x720 | Macho Sadness | store art and documentation |
| Grid 600x900 | Just.Seani | store art and documentation |

The original high-resolution files are kept in `art/steamgriddb/` in the
project, not in the release.
