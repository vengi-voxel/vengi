#!/usr/bin/env python3
"""Check that headers under src/ use #pragma once.

Skips **/external/**, **/dearimgui/**, and generated flextGL/flextVk headers.
"""

from __future__ import annotations

import argparse
import os
import sys
from pathlib import Path

HEADER_SUFFIXES = {".h", ".hpp", ".hh", ".hxx"}
PRAGMA_ONCE = "#pragma once"
# Directory names to skip entirely (anywhere under the scan root)
SKIP_DIR_NAMES = frozenset({"external", "dearimgui"})
# Generated / vendored header basenames to skip
SKIP_FILE_NAMES = frozenset({"flextGL.h", "flextVk.h"})


def is_skipped(path: Path) -> bool:
    return path.name in SKIP_FILE_NAMES or bool(SKIP_DIR_NAMES.intersection(path.parts))


def is_header(path: Path) -> bool:
    return path.suffix.lower() in HEADER_SUFFIXES


def has_pragma_once(path: Path) -> bool:
    try:
        with path.open("r", encoding="utf-8", errors="replace") as f:
            for line in f:
                if line.strip() == PRAGMA_ONCE:
                    return True
    except OSError as e:
        print(f"error: failed to read {path}: {e}", file=sys.stderr)
        return False
    return False


def collect_headers(root: Path) -> list[Path]:
    headers: list[Path] = []
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames[:] = [d for d in dirnames if d not in SKIP_DIR_NAMES]
        base = Path(dirpath)
        for name in filenames:
            path = base / name
            if is_header(path):
                headers.append(path)
    return sorted(headers)


def main() -> int:
    parser = argparse.ArgumentParser(
        description=(
            "Report src/ headers missing #pragma once "
            "(skips **/external/**, **/dearimgui/**, flextGL.h, flextVk.h)."
        )
    )
    parser.add_argument(
        "root",
        nargs="?",
        default="src",
        type=Path,
        help="directory to scan (default: src)",
    )
    parser.add_argument(
        "-q",
        "--quiet",
        action="store_true",
        help="only print missing paths (no summary)",
    )
    args = parser.parse_args()

    root = args.root
    if not root.is_dir():
        print(f"error: not a directory: {root}", file=sys.stderr)
        return 2

    missing: list[Path] = []
    checked = 0
    for path in collect_headers(root):
        if is_skipped(path):
            continue
        checked += 1
        if not has_pragma_once(path):
            missing.append(path)

    for path in missing:
        print(path.as_posix())

    if not args.quiet:
        print(
            f"checked {checked} headers, {len(missing)} missing #pragma once",
            file=sys.stderr,
        )

    return 1 if missing else 0


if __name__ == "__main__":
    sys.exit(main())
