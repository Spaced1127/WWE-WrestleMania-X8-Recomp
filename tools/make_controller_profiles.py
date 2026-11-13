from pathlib import Path

root = Path(__file__).resolve().parent.parent
defaults = root / "defaults"

profiles = {
    "GCPadNew.dualshock3.ini": ("GCPadNew.dualsense.ini", "SDL/0/PS3 Controller"),
    "GCPadNew.dualshock4.ini": ("GCPadNew.dualsense.ini", "SDL/0/PS4 Controller"),
    "GCPadNew.xbox360.ini": ("GCPadNew.xbox.ini", "SDL/0/Xbox 360 Controller"),
    "GCPadNew.steamdeck.ini": ("GCPadNew.xbox.ini", "SDL/0/Steam Deck Controller"),
    "GCPadNew.joycon.ini": ("GCPadNew.switch-pro.ini", "SDL/0/Joy-Con (L)"),
    "GCPadNew.wiiu-pro.ini": ("GCPadNew.switch-pro.ini", "SDL/0/Wii U Pro Controller"),
    "GCPadNew.8bitdo.ini": ("GCPadNew.xbox.ini", "SDL/0/8BitDo Pro 2"),
    "GCPadNew.gamecube.ini": (
        "GCPadNew.xbox.ini",
        "SDL/0/GameCube Controller Adapter",
    ),
    "GCPadNew.generic.ini": ("GCPadNew.xbox.ini", "SDL/0/Gamepad"),
}

for name, (source, device) in profiles.items():
    text = (defaults / source).read_text(encoding="utf-8")
    lines = []
    replaced = False
    for line in text.splitlines(keepends=True):
        if line.startswith("Device ="):
            line = f"Device = {device}\n"
            replaced = True
        lines.append(line)
    if not replaced:
        raise SystemExit(f"{source} has no Device line")
    (defaults / name).write_text("".join(lines), encoding="utf-8", newline="")
    print(f"{name} <- {source} ({device})")
