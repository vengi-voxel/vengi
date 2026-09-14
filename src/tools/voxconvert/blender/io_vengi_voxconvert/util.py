# SPDX-License-Identifier: MIT
# Pure helpers for the vengi-voxconvert Blender addon (no bpy import).

import json
import os
import shutil
import sys
import urllib.request
import zipfile

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


# ---------------------------------------------------------------------------
# Settings grouping / int-as-enum overlays (FileDialogOptions labels)
# ---------------------------------------------------------------------------
# Per-format load/save panels live in FileDialogOptions.cpp and are not cvar
# metadata. Do not tag every cvar with format names. Until vengi exposes a
# structured option map, blender shows a small always-on set plus Advanced.

SKIP_CVAR_PREFIXES = ("app_", "metric_")
KEEP_CVARS = ("core_colorreduction",)

# ident must be a valid RNA identifier (not a leading digit).
INT_ENUMS = {
    "voxformat_meshmode": (
        ("cubic", "0", "Cubes", "Cubic / MagicaVoxel-style cubes"),
        ("marching_cubes", "1", "Marching cubes", "Smooth isosurface"),
        ("binary", "2", "Binary", "Fast binary mesher"),
        ("greedy_texture", "3", "Greedy texture", "Greedy meshing with a texture atlas"),
    ),
    "voxformat_voxelizemode": (
        ("high_quality", "0", "High quality", "Accurate voxelization"),
        ("fast", "1", "Fast", "Faster and uses less memory"),
    ),
}

IMPORT_PRIMARY_CVARS = (
    "voxformat_voxelsize",
    "voxformat_meshmode",
    "palette",
    "core_colorreduction",
    "voxformat_fillhollow",
)

EXPORT_PRIMARY_CVARS = (
    "voxformat_voxelizemode",
    "voxformat_fillhollow",
    "palette",
    "core_colorreduction",
    "voxformat_savevisibleonly",
)


def is_relevant_cvar(key, info):
    if key in KEEP_CVARS:
        pass
    elif key.startswith("core_"):
        return False
    elif any(key.startswith(p) for p in SKIP_CVAR_PREFIXES):
        return False
    if not info:
        return False
    if info.get("flags", 0) & 1:
        return False
    if info.get("type") == "unknown":
        return False
    return True


def int_enum_cli_value(key, cur):
    """Map a blender enum identifier to the numeric cvar string, or None."""
    rows = INT_ENUMS.get(key)
    if not rows:
        return None
    s = str(cur)
    for ident, cli, _label, _desc in rows:
        if ident == s or cli == s:
            return cli
    return None


def int_enum_ident_from_cli(key, cli_val):
    rows = INT_ENUMS.get(key)
    if not rows:
        return None
    s = str(cli_val).split(".")[0]
    for ident, cli, _label, _desc in rows:
        if cli == s:
            return ident
    return rows[0][0]


def int_enum_rna_items(key):
    rows = INT_ENUMS.get(key)
    if not rows:
        return None
    return [(ident, label, desc) for ident, _cli, label, desc in rows]


def split_cvar_keys(keys, primary):
    """Return (primary_in_order, advanced_sorted) for keys that exist."""
    keyset = set(keys)
    prim = [k for k in primary if k in keyset]
    adv = sorted(k for k in keys if k not in primary)
    return prim, adv


def format_cvar_cli_value(typ, cur, key=None):
    """Return the voxconvert -set value string for a current operator value."""
    mapped = int_enum_cli_value(key, cur)
    if mapped is not None:
        return mapped
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


def cvar_value_changed(typ, cur, default_str, key=None):
    """True if the operator value differs from the --jsonconfig default."""
    mapped = int_enum_cli_value(key, cur)
    if mapped is not None:
        cur = mapped
        typ = "int"
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
        if not cvar_value_changed(typ, cur, default_str, key=key):
            continue
        args += ["-set", key, format_cvar_cli_value(typ, cur, key=key)]
    return args


# ---------------------------------------------------------------------------
# --print-formats / --jsonconfig stdout
# ---------------------------------------------------------------------------

def parse_leading_json(text):
    """Parse JSON that may be preceded by log lines."""
    if not text:
        return None
    for i, ch in enumerate(text):
        if ch in "{[":
            text = text[i:]
            break
    try:
        return json.loads(text)
    except Exception:
        return None


# ---------------------------------------------------------------------------
# glTF 2.0 (Blender bundled Khronos addon)
# ---------------------------------------------------------------------------
# This addon round-trips through Blender's bundled glTF 2.0 importer/exporter:
#   bpy.ops.import_scene.gltf / bpy.ops.export_scene.gltf
# Module names:
#   io_scene_gltf2                          Blender 3.6-4.1 (Import-Export: glTF 2.0)
#   bl_ext.blender_org.io_scene_gltf2       Blender 4.2+ bundled extension
# Not a third-party glTF addon.

GLTF_ADDON_MODULES = (
    "io_scene_gltf2",
    "bl_ext.blender_org.io_scene_gltf2",
)

GLTF_MISSING_MSG = (
    "Enable Blender's bundled glTF 2.0 add-on (Edit > Preferences > Add-ons: "
    "'Import-Export: glTF 2.0'). This vengi addon converts through "
    "bpy.ops.import_scene.gltf / bpy.ops.export_scene.gltf from that module "
    "(io_scene_gltf2, or bl_ext.blender_org.io_scene_gltf2 on Blender 4.2+)."
)


def gltf_export_kwargs(filepath, blender_version=None, rna_prop_names=None, use_selection=False, apply_modifiers=False):
    """Kwargs for bpy.ops.export_scene.gltf across Blender 3.6 / 4.x.

    export_colors exists through 4.0, was dropped in 4.1, and 4.2+ uses
    export_vertex_color. Prefer live RNA property names when given.
    """
    kwargs = {
        "filepath": filepath,
        "export_format": "GLB",
        "export_materials": "EXPORT",
    }
    names = set(rna_prop_names or [])
    if names:
        if "export_vertex_color" in names:
            kwargs["export_vertex_color"] = "ACTIVE"
        elif "export_colors" in names:
            kwargs["export_colors"] = True
    else:
        ver = tuple(blender_version or (3, 6))[:2]
        if ver >= (4, 2):
            kwargs["export_vertex_color"] = "ACTIVE"
        elif ver < (4, 1):
            kwargs["export_colors"] = True
        names = None
    if use_selection:
        if names is None or "use_selection" in names:
            kwargs["use_selection"] = True
    if apply_modifiers:
        if names is None or "export_apply" in names:
            kwargs["export_apply"] = True
    return kwargs


def gltf_operators_available(bpy_mod):
    """True if import_scene.gltf and export_scene.gltf exist."""
    try:
        return hasattr(bpy_mod.ops.import_scene, "gltf") and hasattr(bpy_mod.ops.export_scene, "gltf")
    except Exception:
        return False


def gltf_export_rna_prop_names(bpy_mod):
    """Property identifiers of bpy.ops.export_scene.gltf for this Blender."""
    try:
        rna = bpy_mod.ops.export_scene.gltf.get_rna_type()
        return [p.identifier for p in rna.properties]
    except Exception:
        return []


def ensure_gltf_addon(addon_utils_mod, bpy_mod):
    """Enable the bundled glTF 2.0 addon if needed. Returns (ok, error_message)."""
    if gltf_operators_available(bpy_mod):
        return True, ""
    if addon_utils_mod is not None:
        for name in GLTF_ADDON_MODULES:
            try:
                addon_utils_mod.enable(name, default_set=True)
            except Exception:
                continue
            if gltf_operators_available(bpy_mod):
                return True, ""
    return False, GLTF_MISSING_MSG


# ---------------------------------------------------------------------------
# vengi-voxconvert binary lookup (sysFindBinary + OS-specific paths)
# ---------------------------------------------------------------------------

VOXCONVERT_NAME = "vengi-voxconvert"
GITHUB_RELEASES_LATEST = "https://api.github.com/repos/vengi-voxel/vengi/releases/latest"
WIN_VOXCONVERT_ASSET = "win-voxconvert.zip"
_GITHUB_USER_AGENT = "vengi-blender-addon"


def voxconvert_exe_name(system=None):
    system = system or sys.platform
    if system in ("win32", "Windows"):
        return VOXCONVERT_NAME + ".exe"
    return VOXCONVERT_NAME


def _is_file(path):
    return bool(path) and os.path.isfile(path)


def _mac_bundle_binary(root):
    """Return Contents/MacOS/vengi-voxconvert inside an .app bundle, if present."""
    if not root:
        return ""
    if root.endswith(".app"):
        inner = os.path.join(root, "Contents", "MacOS", VOXCONVERT_NAME)
        if _is_file(inner):
            return inner
        return ""
    bundle = os.path.join(root, VOXCONVERT_NAME + ".app")
    inner = os.path.join(bundle, "Contents", "MacOS", VOXCONVERT_NAME)
    if _is_file(inner):
        return inner
    return ""


def _look_in_dir(directory, system=None):
    if not directory:
        return ""
    exe = os.path.join(directory, voxconvert_exe_name(system))
    if _is_file(exe):
        return os.path.abspath(exe)
    found = _mac_bundle_binary(directory)
    if found:
        return os.path.abspath(found)
    nested = os.path.join(directory, "voxconvert", voxconvert_exe_name(system))
    if _is_file(nested):
        return os.path.abspath(nested)
    return ""


def find_voxconvert(
    addon_dir=None,
    extra_dirs=None,
    path_env=None,
    system=None,
    cwd=None,
    program_files=None,
    program_files_x86=None,
    user_install_dir=None,
    which=None,
    use_well_known=True,
):
    """Locate vengi-voxconvert the way VoxConvertUI/sysFindBinary does, plus OS paths.

    Search order:
    1. cwd
    2. PATH (shutil.which)
    3. sibling to the addon folder, and the addon folder itself
    4. user install dir (Windows download target)
    5. OS-specific: Program Files, /usr/bin, Mac .app bundle
    """
    system = system or sys.platform
    cwd = os.getcwd() if cwd is None else cwd
    extra_dirs = list(extra_dirs or [])

    def _first_in(dirs):
        seen = set()
        for d in dirs:
            if not d:
                continue
            key = os.path.normcase(os.path.abspath(d))
            if key in seen:
                continue
            seen.add(key)
            located = _look_in_dir(d, system)
            if located:
                return located
        return ""

    # sysFindBinary: cwd, then PATH
    found = _first_in([cwd] if cwd else [])
    if found:
        return found

    which_fn = which if which is not None else shutil.which
    which_kwargs = {}
    if path_env is not None:
        which_kwargs["path"] = path_env
    names = [VOXCONVERT_NAME]
    if system in ("win32", "Windows"):
        names = [voxconvert_exe_name(system), VOXCONVERT_NAME]
    if which_fn:
        for name in names:
            found = which_fn(name, **which_kwargs)
            if found and _is_file(found):
                return os.path.abspath(found)

    # sibling-to-addon, user install (Windows download), extra test dirs
    local_dirs = list(extra_dirs)
    if user_install_dir:
        local_dirs.append(user_install_dir)
    if addon_dir:
        local_dirs.append(addon_dir)
        local_dirs.append(os.path.dirname(os.path.abspath(addon_dir)))
        local_dirs.append(os.path.join(addon_dir, "bin"))
    found = _first_in(local_dirs)
    if found:
        return found

    if not use_well_known:
        return ""

    well_known = []
    if system in ("win32", "Windows"):
        pf = program_files if program_files is not None else os.environ.get("ProgramFiles", r"C:\Program Files")
        pf86 = program_files_x86 if program_files_x86 is not None else os.environ.get(
            "ProgramFiles(x86)", r"C:\Program Files (x86)"
        )
        for root in (pf, pf86):
            if not root:
                continue
            well_known.extend(
                [
                    os.path.join(root, "vengi"),
                    os.path.join(root, "vengi", "voxconvert"),
                    os.path.join(root, "voxconvert"),
                ]
            )
    elif system == "darwin":
        well_known.extend(
            [
                "/Applications",
                os.path.expanduser("~/Applications"),
                "/usr/local/bin",
                "/opt/homebrew/bin",
            ]
        )
    else:
        well_known.extend(["/usr/bin", "/usr/local/bin", "/opt/vengi"])

    for d in well_known:
        found = _look_in_dir(d, system)
        if found:
            return found
        if system == "darwin":
            found = _mac_bundle_binary(os.path.join(d, VOXCONVERT_NAME + ".app"))
            if found:
                return os.path.abspath(found)
    return ""


# ---------------------------------------------------------------------------
# Windows-only: download win-voxconvert.zip from GitHub releases
# ---------------------------------------------------------------------------

def pick_windows_release_asset(release_json):
    """Return the browser_download_url for win-voxconvert.zip (not -debug)."""
    if not release_json:
        return None
    for asset in release_json.get("assets") or []:
        if asset.get("name") == WIN_VOXCONVERT_ASSET:
            return asset.get("browser_download_url") or None
    return None


def _safe_extractall(zf, dest):
    dest = os.path.abspath(dest)
    for info in zf.infolist():
        target = os.path.abspath(os.path.join(dest, info.filename))
        if target != dest and not target.startswith(dest + os.sep):
            raise ValueError("unsafe path in zip: %s" % info.filename)
    zf.extractall(dest)


def find_voxconvert_in_tree(root, system=None):
    """Walk an extracted release tree and return the vengi-voxconvert path."""
    name = voxconvert_exe_name(system)
    if _is_file(os.path.join(root, name)):
        return os.path.abspath(os.path.join(root, name))
    found = _look_in_dir(root, system)
    if found:
        return found
    for dirpath, dirnames, filenames in os.walk(root):
        if name in filenames:
            return os.path.abspath(os.path.join(dirpath, name))
        if (VOXCONVERT_NAME + ".app") in dirnames:
            found = _mac_bundle_binary(os.path.join(dirpath, VOXCONVERT_NAME + ".app"))
            if found:
                return found
    return ""


def install_voxconvert_from_zip(zip_path, dest_dir, system=None):
    """Extract a voxconvert zip into dest_dir and return the executable path."""
    os.makedirs(dest_dir, exist_ok=True)
    with zipfile.ZipFile(zip_path, "r") as zf:
        _safe_extractall(zf, dest_dir)
    found = find_voxconvert_in_tree(dest_dir, system=system)
    if not found:
        raise FileNotFoundError("vengi-voxconvert not found in %s" % zip_path)
    return found


def fetch_url(url, timeout=60, opener=None):
    if opener is not None:
        return opener(url)
    req = urllib.request.Request(url, headers={"User-Agent": _GITHUB_USER_AGENT})
    with urllib.request.urlopen(req, timeout=timeout) as resp:
        return resp.read()


def download_windows_voxconvert(dest_dir, opener=None, latest_url=GITHUB_RELEASES_LATEST):
    """Download win-voxconvert.zip from the latest GitHub release into dest_dir.

    Windows-only by design. Returns the extracted executable path.
    """
    raw = fetch_url(latest_url, opener=opener)
    if isinstance(raw, bytes):
        release = json.loads(raw.decode("utf-8"))
    else:
        release = json.loads(raw)
    url = pick_windows_release_asset(release)
    if not url:
        raise FileNotFoundError("GitHub release has no %s asset" % WIN_VOXCONVERT_ASSET)
    zip_bytes = fetch_url(url, opener=opener)
    os.makedirs(dest_dir, exist_ok=True)
    zip_path = os.path.join(dest_dir, WIN_VOXCONVERT_ASSET)
    with open(zip_path, "wb") as f:
        f.write(zip_bytes)
    return install_voxconvert_from_zip(zip_path, dest_dir, system="Windows")
