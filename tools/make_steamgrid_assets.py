"""Build the launcher's icon and backdrop from the SteamGridDB art.

Source images live in art/steamgriddb. The originals are third-party art and
are not committed; run ``python tools/fetch_steamgrid_art.py`` first. The
launcher embeds the .ico files at link time and loads the backdrop at runtime
from resources/background.png, so no game data is involved.

Run: python tools/make_steamgrid_assets.py
"""

from pathlib import Path

from PIL import Image, ImageFilter

ROOT = Path(__file__).resolve().parent.parent
ART = ROOT / "art" / "steamgriddb"
ICONS = ROOT / "lib" / "ModernGekko-Template" / "lib" / "ModernGekko" / "resources" / "icons"
RESOURCES = ROOT / "resources"

ICO_SIZES = [(16, 16), (24, 24), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)]


def require(path: Path) -> Path:
    if not path.is_file():
        raise SystemExit(f"missing source art: {path}")
    return path


def main() -> None:
    icon_source = Image.open(require(ART / "icon-1024.png")).convert("RGBA")
    if icon_source.size != (1024, 1024):
        icon_source = icon_source.resize((1024, 1024), Image.LANCZOS)

    ICONS.mkdir(parents=True, exist_ok=True)
    for name in ("launcher", "game"):
        icon_source.save(ICONS / f"{name}.png", format="PNG", optimize=True)
        icon_source.save(ICONS / f"{name}.ico", format="ICO", sizes=ICO_SIZES)

    RESOURCES.mkdir(parents=True, exist_ok=True)
    hero = Image.open(require(ART / "hero-3840x1240.png")).convert("RGB")
    # The backdrop sits behind translucent cards, so soften it: the art reads
    # as an arena poster without a face competing with the controls.
    backdrop = hero.resize((2560, 827), Image.LANCZOS).filter(
        ImageFilter.GaussianBlur(14)
    )
    backdrop.save(RESOURCES / "background.png", format="PNG", optimize=True)

    # Keep the docs/art copies in sync with what the launcher actually uses.
    icon_source.save(ROOT / "art" / "wmx8-icon.png", format="PNG", optimize=True)
    hero.resize((1920, 620), Image.LANCZOS).save(
        ROOT / "art" / "wmx8-backdrop.png", format="PNG", optimize=True
    )

    print(f"icons    {ICONS / 'launcher.ico'}  {len(ICO_SIZES)} sizes")
    print(f"icon png {ICONS / 'launcher.png'}  {icon_source.size[0]}x{icon_source.size[1]}")
    print(f"backdrop {RESOURCES / 'background.png'}  {backdrop.size[0]}x{backdrop.size[1]}")


if __name__ == "__main__":
    main()
