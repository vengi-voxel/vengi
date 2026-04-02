#!/usr/bin/env python3
"""Generate JSON metadata files for vengi lua scripts.

Reads the description() function from each .lua file and produces a
sidecar .json with name, description, version, author and type fields.

Usage examples:
  # Single file
  python3 generate_script_json.py path/to/tree_pine.lua

  # All tree scripts
  python3 generate_script_json.py src/modules/voxelgenerator/lua/scripts/tree_*.lua

  # Override defaults
  python3 generate_script_json.py -a "someone" -v 2.0.0 -t brush script.lua
"""

import argparse
import json
import os
import re
import sys


def extract_description(lua_path):
    """Extract the return value of the description() function from a lua file."""
    with open(lua_path, "r") as f:
        content = f.read()

    match = re.search(
        r"function\s+description\s*\(\s*\)\s*\n\s*return\s+['\"](.+?)['\"]\s*\n",
        content,
    )
    if match:
        return match.group(1)
    return ""


def extract_luadoc(lua_path, directive):
    """Extract a @directive value from LuaDoc comments in a lua file."""
    with open(lua_path, "r") as f:
        content = f.read()
    match = re.search(r'--\s*@' + re.escape(directive) + r'\s+(.+)', content)
    if match:
        return match.group(1).strip()
    return ""


def derive_name(filename):
    """Derive a human-readable name from the lua filename stem."""
    stem = os.path.splitext(os.path.basename(filename))[0]
    # Split on underscores, skip prefix like 'tree'
    parts = stem.split("_")
    if len(parts) > 1 and parts[0] in ("tree", "brush"):
        parts = parts[1:]
    # Capitalize each part, join with camelCase-aware splitting
    words = []
    for part in parts:
        # Split camelCase within each part
        sub = re.sub(r"([a-z])([A-Z])", r"\1 \2", part)
        words.extend(sub.split())
    return " ".join(w.capitalize() for w in words)


def generate_json(lua_path, author, version, script_type):
    """Generate a JSON metadata file for the given lua script."""
    name = derive_name(lua_path)
    description = extract_description(lua_path)
    if not author or author == "vengi":
        author = extract_luadoc(lua_path, "author") or author
    if not version or version == "1.0.0":
        version = extract_luadoc(lua_path, "version") or version

    json_path = os.path.splitext(lua_path)[0] + ".json"

    data = {
        "name": name,
        "description": description,
        "version": version,
        "author": author,
    }
    if script_type:
        data["type"] = script_type

    with open(json_path, "w") as f:
        json.dump(data, f, indent=4)
        f.write("\n")

    return json_path


def main():
    parser = argparse.ArgumentParser(
        description="Generate JSON metadata files for vengi lua scripts"
    )
    parser.add_argument("files", nargs="+", help="Lua script file(s) to process")
    parser.add_argument(
        "-a", "--author", default="vengi", help="Author name (default: vengi)"
    )
    parser.add_argument(
        "-v", "--version", default="1.0.0", help="Version string (default: 1.0.0)"
    )
    parser.add_argument(
        "-t",
        "--type",
        default="generator",
        choices=["generator", "brush"],
        help="Script type (default: generator)",
    )
    args = parser.parse_args()

    for lua_path in args.files:
        if not os.path.isfile(lua_path):
            print(f"Warning: {lua_path} not found, skipping", file=sys.stderr)
            continue
        if not lua_path.endswith(".lua"):
            print(f"Warning: {lua_path} is not a .lua file, skipping", file=sys.stderr)
            continue

        json_path = generate_json(lua_path, args.author, args.version, args.type)
        desc = extract_description(lua_path)
        status = "with description" if desc else "no description found"
        print(f"{json_path} ({status})")


if __name__ == "__main__":
    main()
