from pathlib import Path

p = Path(__file__).resolve().parent.parent / "lib" / "ModernGekko-Template" / "lib" / "ModernGekko" / "tools" / "moderngekko_launcher.cpp"
t = p.read_text(encoding="utf-8")
t = t.replace('"GSQE78  \ufffd  READY"', '"GW3E78  |  READY"')
t = t.replace('"GSQE78  \u2022  READY"', '"GW3E78  |  READY"')
t = t.replace('"Static recompilation  \ufffd  PC"', '"Static recompilation  |  PC"')
t = t.replace('"Static recompilation  \u2022  PC"', '"Static recompilation  |  PC"')
t = t.replace('"Disc ID %s  \ufffd  retail DOL verified"', '"Disc ID %s  |  retail DOL verified"')
t = t.replace('"Disc ID %s  \u2022  retail DOL verified"', '"Disc ID %s  |  retail DOL verified"')
p.write_text(t, encoding="utf-8")
for i, line in enumerate(t.splitlines(), 1):
    if "GW3E78" in line or "Static recompilation" in line or "Disc ID %s" in line:
        print(i, line.strip())
