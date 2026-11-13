"""Fetch the SteamGridDB art used by the launcher.

The original images are third-party promotional art, so they are not committed.
Run this once, then ``python tools/make_steamgrid_assets.py`` to build the
launcher icons and backdrop. SteamGridDB game entry: WWE WrestleMania X8
(id 5295541).

The CDN URLs are the assets chosen for this port; they do not need an API key.
"""

from pathlib import Path
from urllib.request import Request, urlopen

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "art" / "steamgriddb"

ASSETS = {
    "icon-1024.png": "https://cdn2.steamgriddb.com/icon/e1d3b791194e0ceb9dfaed5f03da88b8.png",
    "hero-3840x1240.png": "https://cdn2.steamgriddb.com/hero/e166794e138d2c04171588e4372e1a4a.png",
    "logo-1280x720.png": "https://cdn2.steamgriddb.com/logo/7bc1a1dd03acc7c854973e4f76df0c68.png",
    "grid-600x900.png": "https://cdn2.steamgriddb.com/grid/51048c28024e94bf9a99a0daa728113b.png",
}


def main() -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    for name, url in ASSETS.items():
        destination = OUT / name
        if destination.is_file() and destination.stat().st_size > 0:
            print(f"keep  {name}")
            continue
        request = Request(url, headers={"User-Agent": "WMX8-Recomp/0.1"})
        with urlopen(request, timeout=60) as response:
            data = response.read()
        if not data:
            raise SystemExit(f"empty download: {url}")
        destination.write_bytes(data)
        print(f"fetch {name}  {len(data):,} bytes")
    print(f"art in {OUT}")


if __name__ == "__main__":
    main()
