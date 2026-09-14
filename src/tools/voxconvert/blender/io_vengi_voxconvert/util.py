# SPDX-License-Identifier: MIT
# Pure helpers for the vengi-voxconvert Blender addon (no bpy import).

# Blender file-browser filter_glob is truncated at 255 characters.
FILTER_GLOB_MAX = 255


def build_filter_glob(extensions, max_len=FILTER_GLOB_MAX):
    """Build a Blender filter_glob string from file extensions.

    Compound extensions (e.g. ben.json) are omitted because the file browser
    matches the last suffix only. If the joined glob would exceed max_len,
    fall back to '*.*' so formats are not silently dropped by truncation.
    """
    if not extensions:
        return "*.*"
    parts = []
    seen = set()
    for ext in extensions:
        if not ext:
            continue
        e = ext.lower().lstrip(".")
        if "." in e:
            continue
        token = "*." + e
        if token in seen:
            continue
        seen.add(token)
        parts.append(token)
    parts.sort()
    if not parts:
        return "*.*"
    joined = ";".join(parts)
    if len(joined) > max_len:
        return "*.*"
    return joined


def cvar_rna_name(info, key):
    """RNA label from --jsonconfig title, otherwise the cvar key."""
    title = (info or {}).get("title")
    if title:
        return title
    return key


def cvar_help(info, key):
    """Description text: description, then help, then the key."""
    if not info:
        return key
    return info.get("description") or info.get("help") or key


def cvar_numeric_bounds(info):
    """Return (min, max) from --jsonconfig, or (None, None)."""
    if not info or ("min" not in info and "max" not in info):
        return None, None
    typ = info.get("type", "string")
    mn = info.get("min")
    mx = info.get("max")
    if typ == "int":
        if mn is not None:
            mn = int(mn)
        if mx is not None:
            mx = int(mx)
    elif typ == "float":
        if mn is not None:
            mn = float(mn)
        if mx is not None:
            mx = float(mx)
    return mn, mx


def cvar_path_subtype(info):
    """Blender StringProperty subtype for path/directory cvars, or None."""
    typ = (info or {}).get("type")
    if typ == "directory":
        return "DIR_PATH"
    if typ == "path":
        return "FILE_PATH"
    return None


def format_cvar_cli_value(typ, cur):
    """Return the voxconvert -set value string for a current operator value."""
    if typ == "boolean":
        if isinstance(cur, str):
            return "true" if cur.lower() in ("true", "1", "yes") else "false"
        return "true" if cur else "false"
    if typ == "int":
        try:
            return str(int(cur))
        except (TypeError, ValueError):
            return str(cur)
    if typ == "float":
        try:
            return str(float(cur))
        except (TypeError, ValueError):
            return str(cur)
    return str(cur)


def cvar_value_changed(typ, cur, default_str):
    """True if the operator value differs from the --jsonconfig default."""
    if default_str is None:
        default_str = ""
    if typ == "boolean":
        def as_bool(v):
            if isinstance(v, bool):
                return v
            if isinstance(v, (int, float)) and not isinstance(v, bool):
                return bool(v)
            return str(v).lower() in ("true", "1", "yes")
        return as_bool(cur) != as_bool(default_str)
    if typ == "float":
        try:
            return float(cur) != float(default_str)
        except (TypeError, ValueError):
            return str(cur) != str(default_str)
    if typ == "int":
        try:
            return int(cur) != int(float(default_str))
        except (TypeError, ValueError):
            return str(cur) != str(default_str)
    return str(cur) != str(default_str)


def cvar_set_args(cvars, op):
    """Build -set key value CLI args for cvars that differ from defaults."""
    args = []
    if not cvars:
        return args
    for key, info in cvars.items():
        if not hasattr(op, key):
            continue
        cur = getattr(op, key)
        typ = info.get("type", "string")
        default_str = info.get("value", "")
        if not cvar_value_changed(typ, cur, default_str):
            continue
        args += ["-set", key, format_cvar_cli_value(typ, cur)]
    return args
