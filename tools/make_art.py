#!/usr/bin/env python3
"""Legacy procedural art generator.

Superseded by ``tools/make_steamgrid_assets.py``, which installs the
SteamGridDB icon and backdrop. Kept for reference: running this file overwrites
``resources/icons`` and ``art/wmx8-*.png`` with the older procedural art.
"""
from __future__ import annotations

import math
import random
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter, ImageFont

ROOT = Path(__file__).resolve().parent.parent
ART = ROOT / "art"
RES = ROOT / "lib" / "ModernGekko-Template" / "lib" / "ModernGekko" / "resources" / "icons"
STORE = ROOT / "docs" / "store"

IMPACT = r"C:\Windows\Fonts\impact.ttf"
ARIBLK = r"C:\Windows\Fonts\ariblk.ttf"
SEGOE = r"C:\Windows\Fonts\segoeuib.ttf"


def lerp(a, b, t):
    return tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(3))


def background(draw, w, h):
    for y in range(h):
        t = y / (h - 1)
        # Dark arena with a slightly warmer band in the middle.
        mid = math.sin(math.pi * t)
        draw.line([(0, y), (w, y)], fill=(int(7 + 10 * mid), int(10 + 13 * mid), int(18 + 24 * mid)))


def spotlights(draw, w, h):
    beams = [(0.16, 22), (0.34, 30), (0.50, 34), (0.66, 30), (0.84, 22)]
    for cx, alpha in beams:
        x = w * cx
        draw.polygon(
            [(x, -10), (x - w * 0.14, h * 0.80), (x + w * 0.17, h * 0.80)],
            fill=(255, 222, 150, alpha),
        )


def crowd(draw, w, h, seed=7, top=0.10, bottom=0.42, count=220):
    rng = random.Random(seed)
    for _ in range(count):
        x = rng.uniform(0, w)
        y = rng.uniform(h * top, h * bottom)
        r = rng.uniform(w * 0.002, w * 0.006)
        alpha = rng.randint(16, 64)
        color = rng.choice([(210, 214, 225), (245, 205, 130), (130, 165, 225)])
        draw.ellipse([x - r, y - r, x + r, y + r], fill=color + (alpha,))


def ring(draw, w, h, cx, cy, rx, ry, scale=1.0):
    # Canvas floor and apron.
    draw.ellipse([cx - rx, cy - ry, cx + rx, cy + ry], fill=(26, 28, 35, 255),
                 outline=(92, 94, 106, 255), width=int(5 * scale))
    draw.ellipse([cx - rx, cy - ry * 0.45, cx + rx, cy + ry * 1.55], fill=(18, 20, 26, 255))
    draw.ellipse([cx - rx, cy - ry, cx + rx, cy + ry], outline=(150, 28, 40, 255),
                 width=int(9 * scale))

    # Three sagging ropes.
    for index, (offset, sag, color) in enumerate(
        [(-0.95, 0.22, (185, 28, 42)), (-0.35, 0.26, (200, 34, 48)), (0.28, 0.30, (185, 28, 42))]
    ):
        y = cy + offset * ry
        pts = []
        for x in range(int(cx - rx * 1.06), int(cx + rx * 1.06), max(2, int(6 * scale))):
            t = (x - (cx - rx * 1.06)) / (rx * 2.12)
            pts.append((x, y + sag * ry * math.sin(math.pi * t)))
        draw.line(pts, fill=color + (255,), width=max(3, int(13 * scale)), joint="curve")

    # Corner posts.
    for x in (cx - rx * 1.05, cx + rx * 1.05):
        draw.rounded_rectangle(
            [x - 14 * scale, cy - ry * 1.35, x + 14 * scale, cy + ry * 0.55],
            radius=int(10 * scale),
            fill=(118, 122, 136, 255),
            outline=(58, 60, 70, 255),
            width=max(2, int(3 * scale)),
        )


def belt(draw, w, h, cx, cy, scale=1.0):
    plate_w, plate_h = 340 * scale, 190 * scale
    draw.rounded_rectangle(
        [cx - plate_w / 2, cy - plate_h / 2, cx + plate_w / 2, cy + plate_h / 2],
        radius=int(70 * scale),
        fill=(110, 76, 14, 255),
        outline=(255, 216, 120, 255),
        width=max(3, int(7 * scale)),
    )
    draw.rounded_rectangle(
        [cx - plate_w / 2 + 22 * scale, cy - plate_h / 2 + 20 * scale,
         cx + plate_w / 2 - 22 * scale, cy + plate_h / 2 - 20 * scale],
        radius=int(56 * scale),
        fill=(194, 148, 38, 255),
    )
    for side in (-1, 1):
        sx = cx + side * (plate_w / 2 + 52 * scale)
        draw.ellipse(
            [sx - 42 * scale, cy - 52 * scale, sx + 42 * scale, cy + 52 * scale],
            fill=(150, 108, 26, 255), outline=(255, 216, 120, 255), width=max(2, int(4 * scale)),
        )


def gold_text(base, draw, text, font, center, max_width, outline=16, shadow=10):
    """Draw text with a red outline, drop shadow and vertical gold gradient."""
    bbox = draw.textbbox((0, 0), text, font=font, stroke_width=outline)
    width = bbox[2] - bbox[0]
    if width > max_width:
        return False
    x = center[0] - width / 2 - bbox[0]
    y = center[1] - (bbox[3] - bbox[1]) / 2 - bbox[1]

    draw.text((x + shadow, y + shadow), text, font=font, fill=(0, 0, 0, 170),
              stroke_width=outline, stroke_fill=(0, 0, 0, 170))
    draw.text((x, y), text, font=font, fill=(255, 236, 170, 255),
              stroke_width=outline, stroke_fill=(110, 12, 22, 255))

    mask = Image.new("L", base.size, 0)
    ImageDraw.Draw(mask).text((x, y), text, font=font, fill=255)
    gold = Image.new("RGB", base.size, (0, 0, 0))
    gd = ImageDraw.Draw(gold)
    top, bottom = bbox[1] + y, bbox[3] + y
    for py in range(base.size[1]):
        t = min(max((py - top) / max(1, bottom - top), 0.0), 1.0)
        gd.line([(0, py), (base.size[0], py)], fill=lerp((255, 250, 215), (146, 94, 6), t))
    base.paste(gold, (0, 0), mask)
    return True


def fit_font(path, text, max_width, start, draw, min_size=20):
    size = start
    while size > min_size:
        font = ImageFont.truetype(path, size)
        bbox = draw.textbbox((0, 0), text, font=font)
        if bbox[2] - bbox[0] <= max_width:
            return font
        size -= 8
    return ImageFont.truetype(path, min_size)


def make_icon() -> Image.Image:
    w = h = 1024
    base = Image.new("RGBA", (w, h), (0, 0, 0, 255))
    draw = ImageDraw.Draw(base, "RGBA")
    background(draw, w, h)
    spotlights(draw, w, h)
    crowd(draw, w, h, count=170, top=0.08, bottom=0.40)

    ring(draw, w, h, cx=w * 0.5, cy=h * 0.72, rx=w * 0.43, ry=h * 0.15)
    belt(draw, w, h, cx=w * 0.5, cy=h * 0.865, scale=0.95)

    font = fit_font(IMPACT, "WMX8", w * 0.82, 430, draw)
    gold_text(base, draw, "WMX8", font, (w * 0.5, h * 0.38), w * 0.86, outline=18, shadow=12)

    small = ImageFont.truetype(ARIBLK, 34)
    label = "S T A T I C   R E C O M P"
    lb = draw.textbbox((0, 0), label, font=small)
    draw.text(((w - (lb[2] - lb[0])) / 2, h * 0.60), label,
              font=small, fill=(232, 236, 246, 175))

    # Vignette.
    vignette = Image.new("L", (w, h), 0)
    vd = ImageDraw.Draw(vignette)
    vd.ellipse([-w * 0.25, -h * 0.25, w * 1.25, h * 1.25], fill=255)
    vignette = vignette.filter(ImageFilter.GaussianBlur(w * 0.12))
    black = Image.new("RGBA", (w, h), (0, 0, 0, 255))
    base = Image.composite(base, black, vignette)
    return base.convert("RGB")


def make_backdrop() -> Image.Image:
    w, h = 1920, 1080
    base = Image.new("RGBA", (w, h), (0, 0, 0, 255))
    draw = ImageDraw.Draw(base, "RGBA")
    background(draw, w, h)
    spotlights(draw, w, h)
    crowd(draw, w, h, count=300, top=0.05, bottom=0.45)

    ring(draw, w, h, cx=w * 0.72, cy=h * 0.70, rx=w * 0.30, ry=h * 0.16, scale=1.5)

    font = fit_font(IMPACT, "WRESTLEMANIA", w * 0.46, 210, draw)
    gold_text(base, draw, "WRESTLEMANIA", font, (w * 0.29, h * 0.40), w * 0.50,
              outline=14, shadow=10)
    font_x8 = fit_font(IMPACT, "X8", w * 0.24, 320, draw)
    gold_text(base, draw, "X8", font_x8, (w * 0.29, h * 0.60), w * 0.28,
              outline=16, shadow=12)

    small = ImageFont.truetype(ARIBLK, 42)
    draw.text((w * 0.08, h * 0.73), "STATIC RECOMPILATION", font=small,
              fill=(235, 239, 250, 220))
    tiny = ImageFont.truetype(SEGOE, 30)
    draw.text((w * 0.08, h * 0.79), "GW3E78  /  PC", font=tiny,
              fill=(190, 198, 214, 190))

    # Bottom fade.
    for y in range(int(h * 0.86), h):
        t = (y - h * 0.86) / (h * 0.14)
        draw.line([(0, y), (w, y)], fill=(0, 0, 0, int(170 * t)))

    return base.convert("RGB")


def main() -> None:
    ART.mkdir(parents=True, exist_ok=True)
    RES.mkdir(parents=True, exist_ok=True)
    STORE.mkdir(parents=True, exist_ok=True)

    icon = make_icon()
    icon.save(ART / "wmx8-icon.png")
    icon.save(RES / "launcher.png")
    icon.save(RES / "game.png")
    for name in ("launcher", "game"):
        icon.save(RES / f"{name}.ico", format="ICO",
                  sizes=[(16, 16), (24, 24), (32, 32), (48, 48), (64, 64),
                         (128, 128), (256, 256)])

    backdrop = make_backdrop()
    backdrop.save(ART / "wmx8-backdrop.png")
    backdrop.save(STORE / "backdrop.png")
    icon.resize((512, 512), Image.LANCZOS).save(STORE / "icon-512.png")
    icon.save(STORE / "icon.png")

    print(f"icon     {ART / 'wmx8-icon.png'}")
    print(f"backdrop {ART / 'wmx8-backdrop.png'}")
    print(f"icons    {RES}")
    print(f"store    {STORE}")


if __name__ == "__main__":
    main()
