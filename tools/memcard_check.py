#!/usr/bin/env python3
"""Inspect a GameCube raw memory card (GC/MemoryCardA.USA.raw).

Slot A is a raw card (Core/SlotA = 1), so the game's saves live in one
16,777,216-byte file. This tool validates the file and lists the directory
entries, which is how a save is confirmed without a GUI:

    python tools/memcard_check.py "work/user/GC/MemoryCardA.USA.raw"

Exit codes: 0 = valid card, 2 = not a usable card.

Layout (Dolphin Source/Core/Core/HW/GCMemcard/GCMemcard.h):
  block 0x2000 bytes each: 0 header, 1 directory, 2 directory backup,
  3 block allocation table, 4 BAT backup. 127 directory entries of 0x40.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import sys
from pathlib import Path

BLOCK_SIZE = 0x2000
DIR_ENTRIES = 0x7F
ENTRY_SIZE = 0x40
CARD_SIZE = 16 * 1024 * 1024
OFFSET_DIRECTORY = BLOCK_SIZE * 1
OFFSET_DIRECTORY_BACKUP = BLOCK_SIZE * 2
OFFSET_BAT = BLOCK_SIZE * 3

# Directory: 127 * 0x40 entries, 0x3a padding, 2-byte update counter, then the
# additive checksum and its inverse (0x1ffc / 0x1ffe).
DIR_CHECKSUM_AREA = 0x1FFC


def _printable(raw: bytes) -> str:
    return raw.decode("ascii", "replace") if all(32 <= b < 127 for b in raw) else ""


def _printable_text(raw: bytes) -> str:
    """Text field padded with NULs or spaces (file names)."""
    trimmed = raw.rstrip(b"\x00 ")
    return trimmed.decode("ascii", "replace") if all(32 <= b < 127 for b in trimmed) else ""


def parse_entry(entry: bytes, index: int = 0) -> dict:
    return {
        "index": index,
        "gamecode": _printable(entry[0:4]),
        "makercode": _printable(entry[4:6]),
        "filename": _printable_text(entry[0x08:0x28]),
        "first_block": int.from_bytes(entry[0x36:0x38], "big"),
        "blocks": int.from_bytes(entry[0x38:0x3A], "big"),
        "comment_offset": int.from_bytes(entry[0x3C:0x40], "big"),
    }


def parse_directory(block: bytes) -> list[dict]:
    entries = []
    for index in range(DIR_ENTRIES):
        entry = block[index * ENTRY_SIZE:(index + 1) * ENTRY_SIZE]
        gamecode = entry[0:4]
        if gamecode == b"\xff\xff\xff\xff" or gamecode == b"\x00\x00\x00\x00":
            continue
        parsed = parse_entry(entry, index)
        if parsed["gamecode"]:
            entries.append(parsed)
    return entries


def checksum_ok(block: bytes) -> bool:
    total = 0
    total_inv = 0
    for offset in range(0, DIR_CHECKSUM_AREA, 2):
        word = int.from_bytes(block[offset:offset + 2], "big")
        total = (total + word) & 0xFFFF
        total_inv = (total_inv + (word ^ 0xFFFF)) & 0xFFFF
    if total == 0xFFFF:
        total = 0
    if total_inv == 0xFFFF:
        total_inv = 0
    stored = int.from_bytes(block[DIR_CHECKSUM_AREA:DIR_CHECKSUM_AREA + 2], "big")
    stored_inv = int.from_bytes(
        block[DIR_CHECKSUM_AREA + 2:DIR_CHECKSUM_AREA + 4], "big"
    )
    return total == stored and total_inv == stored_inv


def parse_bat(block: bytes) -> dict:
    return {
        "checksum": int.from_bytes(block[0:2], "big"),
        "checksum_inverse": int.from_bytes(block[2:4], "big"),
        "update_counter": int.from_bytes(block[4:6], "big"),
        "free_blocks": int.from_bytes(block[6:8], "big"),
        "last_allocated_block": int.from_bytes(block[8:10], "big"),
    }


def inspect(path: Path) -> dict:
    data = path.read_bytes()
    report: dict = {
        "path": str(path),
        "size": len(data),
        "size_ok": len(data) == CARD_SIZE,
        "sha256": hashlib.sha256(data).hexdigest(),
        "usable": False,
        "entries": [],
        "entries_backup": [],
        "directory_checksum_ok": False,
        "bat": {},
        "error": "",
    }
    if len(data) != CARD_SIZE:
        report["error"] = (
            f"expected {CARD_SIZE} bytes for a 128 Mbit raw card, found {len(data)}"
        )
        return report

    directory = data[OFFSET_DIRECTORY:OFFSET_DIRECTORY + BLOCK_SIZE]
    backup = data[OFFSET_DIRECTORY_BACKUP:OFFSET_DIRECTORY_BACKUP + BLOCK_SIZE]
    bat = data[OFFSET_BAT:OFFSET_BAT + BLOCK_SIZE]

    report["entries"] = parse_directory(directory)
    report["entries_backup"] = parse_directory(backup)
    report["directory_checksum_ok"] = checksum_ok(directory)
    report["bat"] = parse_bat(bat)
    report["usable"] = report["directory_checksum_ok"] and bool(report["bat"])
    if not report["directory_checksum_ok"]:
        report["error"] = "directory checksum mismatch"
    return report


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("card", type=Path, nargs="?", help="raw memory card file")
    parser.add_argument("--gci", type=Path, help="print one .gci directory entry")
    parser.add_argument("--json", action="store_true", help="machine-readable output")
    args = parser.parse_args(argv)

    if args.gci:
        if not args.gci.is_file():
            print(f"no .gci file: {args.gci}", file=sys.stderr)
            return 2
        entry = parse_entry(args.gci.read_bytes()[:ENTRY_SIZE])
        payload = args.gci.stat().st_size - ENTRY_SIZE
        entry["file_bytes"] = payload
        entry["blocks_declared"] = entry["blocks"]
        if args.json:
            print(json.dumps(entry, indent=2))
        else:
            print(f"gci     {args.gci}")
            print(
                'entry   {gamecode}-{makercode} "{filename}" blocks={blocks} '
                "first={first_block} file_bytes={file_bytes}".format(**entry)
            )
        return 0 if entry["gamecode"] else 2

    if args.card is None:
        parser.print_help()
        return 2
    if not args.card.is_file():
        print(f"no card file: {args.card}", file=sys.stderr)
        return 2
    report = inspect(args.card)
    if args.json:
        print(json.dumps(report, indent=2))
    else:
        print(f"card    {report['path']}")
        print(f"size    {report['size']} bytes ({'ok' if report['size_ok'] else 'WRONG'})")
        print(f"sha256  {report['sha256']}")
        print(f"dir     {'checksum ok' if report['directory_checksum_ok'] else 'CHECKSUM BAD'}")
        bat = report["bat"]
        if bat:
            print(
                "bat     free={free_blocks} last={last_allocated_block} "
                "counter={update_counter}".format(**bat)
            )
        entries = report["entries"]
        print(f"saves   {len(entries)}")
        for entry in entries:
            print(
                "        [{index:03d}] {gamecode}-{makercode} "
                "\"{filename}\" blocks={blocks} first={first_block}".format(**entry)
            )
        if report["error"]:
            print(f"error   {report['error']}")
    return 0 if report["usable"] else 2


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
