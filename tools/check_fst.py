#!/usr/bin/env python3
"""Verify every FST file exists in the extracted game root with the right size."""
from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import gcdisc


def main() -> int:
    iso = Path(sys.argv[1])
    root_dir = Path(sys.argv[2])
    entries, fst_off, fst_size = gcdisc.parse_fst(iso)
    files = [(p, e) for p, e in gcdisc.walk(entries) if not e.is_dir]
    dirs = [p for p, e in gcdisc.walk(entries) if e.is_dir]

    # Build a case-insensitive index of what is on disk.
    disk: dict[str, Path] = {}
    for p in root_dir.rglob("*"):
        if p.is_file():
            rel = p.relative_to(root_dir).as_posix()
            disk[rel.lower()] = p

    missing = []
    size_bad = []
    case_mismatch = []
    non_ascii = []
    for path, e in files:
        needle = path.lower()
        if any(ord(c) > 127 for c in path):
            non_ascii.append(path)
        p = disk.get(needle)
        if p is None:
            missing.append((path, e.size))
            continue
        if p.name != path.split("/")[-1]:
            case_mismatch.append((path, p.name))
        if p.stat().st_size != e.size:
            size_bad.append((path, e.size, p.stat().st_size))

    print(f"FST files : {len(files)}")
    print(f"FST dirs  : {len(dirs)}")
    print(f"on disk   : {len(disk)} files")
    print(f"missing   : {len(missing)}")
    for path, size in missing[:40]:
        print(f"  MISSING {size:>10}  {path}")
    print(f"size bad  : {len(size_bad)}")
    for path, want, got in size_bad[:40]:
        print(f"  SIZE {want} != {got}  {path}")
    print(f"case diff : {len(case_mismatch)}")
    for path, name in case_mismatch[:20]:
        print(f"  CASE {path}  ->  {name}")
    print(f"non-ASCII : {len(non_ascii)}")
    for path in non_ascii[:40]:
        print(f"  NONASCII {path}")
    return 1 if (missing or size_bad) else 0


if __name__ == "__main__":
    raise SystemExit(main())
