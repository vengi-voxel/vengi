# SPDX-License-Identifier: MIT
# Pure helpers for the vengi-voxconvert Blender addon (no bpy import).

import json
import os
import re
import shutil
import signal
import subprocess
import sys
import threading
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
    """Description text from --jsonconfig, otherwise the cvar key."""
    if not info:
        return key
    return info.get("description") or key


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
# Settings grouping from --jsonconfig (FormatConfig::writeConfigJson)
# ---------------------------------------------------------------------------
# load/save/mesh/image/rgb/all/primary/order, optional formats[] and value_titles[]
# come from voxconvert. Only cvars with load or save metadata are FileDialogOptions
# settings. Blender operators are not 1:1 with a single format:
#   import   = volume load + mesh save (GLB preview)
#   voxelize = mesh/image load + mesh save (GLB preview)
#   export   = mesh load (GLB) + volume save (or mesh save when the target is a mesh)

CVAR_OP_IMPORT = "import"
CVAR_OP_VOXELIZE = "voxelize"
CVAR_OP_EXPORT = "export"
FORMAT_ALL_ID = "ALL"
SCRIPT_NONE_ID = "NONE"
SCRIPT_ARG_MAX = 32


def is_relevant_cvar(info):
    """True if --jsonconfig marked this cvar as a load/save format option."""
    if not info:
        return False
    if info.get("readonly"):
        return False
    if info.get("type") == "unknown":
        return False
    return cvar_has_usage_meta(info)


def _enum_ident_from_title(title, index):
    ident = re.sub(r"[^a-z0-9]+", "_", (title or "").lower()).strip("_")
    if not ident:
        ident = "v%d" % index
    if ident[0].isdigit():
        ident = "v" + ident
    return ident


def int_enum_rows(info=None):
    """Return (ident, cli, label, desc) rows from --jsonconfig value_titles."""
    titles = (info or {}).get("value_titles")
    if not titles:
        return None
    generated = []
    for i, title in enumerate(titles):
        if not title:
            continue
        ident = _enum_ident_from_title(title, i)
        generated.append((ident, str(i), title, title))
    return generated or None


def int_enum_cli_value(cur, info=None):
    """Map a blender enum identifier to the numeric cvar string, or None."""
    rows = int_enum_rows(info)
    if not rows:
        return None
    s = str(cur)
    for ident, cli, _label, _desc in rows:
        if ident == s or cli == s:
            return cli
    return None


def int_enum_ident_from_cli(cli_val, info=None):
    rows = int_enum_rows(info)
    if not rows:
        return None
    s = str(cli_val).split(".")[0]
    for ident, cli, _label, _desc in rows:
        if cli == s:
            return ident
    return rows[0][0]


def int_enum_rna_items(info=None):
    rows = int_enum_rows(info)
    if not rows:
        return None
    return [(ident, label, desc) for ident, _cli, label, desc in rows]


def cvar_order(info):
    """Stable UI order from FormatConfig table index, or a large fallback."""
    if not info:
        return 10 ** 9
    order = info.get("order")
    if isinstance(order, int):
        return order
    try:
        return int(order)
    except (TypeError, ValueError):
        return 10 ** 9


def split_cvar_keys(keys, cvars=None, skip=()):
    """Return (primary, format_specific, advanced) using --jsonconfig flags.

    primary: primary=true
    format_specific: formats[] (already filtered to the active format)
    advanced: remaining, in FormatConfig order
    """
    skipset = set(skip or ())
    cvars = cvars or {}
    kept = [k for k in keys if k not in skipset]
    prim = []
    fmt = []
    adv = []
    for key in kept:
        info = cvars.get(key) or {}
        if info.get("primary"):
            prim.append(key)
        elif info.get("formats"):
            fmt.append(key)
        else:
            adv.append(key)
    prim.sort(key=lambda k: cvar_order(cvars.get(k)))
    fmt.sort(key=lambda k: cvar_order(cvars.get(k)))
    adv.sort(key=lambda k: cvar_order(cvars.get(k)))
    return prim, fmt, adv


def format_is_mesh(formats, format_id):
    fmt = find_format(formats, format_id)
    return bool(fmt and fmt.get("mesh"))


def _format_exts(fmt):
    return {e.lower().lstrip(".") for e in (fmt.get("extensions") or []) if e}


def gltf_format_name(formats):
    """FormatDescription.name of the glTF mesh entry from --print-formats.

    Identified by glb+gltf extensions (and mesh), not by the English name.
    """
    for fmt in formats or []:
        exts = _format_exts(fmt)
        if fmt.get("mesh") and "glb" in exts and "gltf" in exts:
            return fmt.get("name")
    return None


def resolve_format_name(formats, format_id, filepath=None, all_id=None):
    """Selected format's --print-formats name, or infer from filepath when ALL."""
    if all_id is None:
        all_id = FORMAT_ALL_ID
    if format_id and format_id != all_id:
        fmt = find_format(formats, format_id)
        if fmt:
            return fmt.get("name")
    if filepath:
        for fmt in formats or []:
            if path_matches_extensions(filepath, fmt.get("extensions")):
                return fmt.get("name")
    return None


def cvar_has_usage_meta(info):
    return bool(info) and ("load" in info or "save" in info)


def cvar_applies_to_format(info, format_name):
    """True if the cvar is generic, or format_name is listed in formats[]."""
    names = (info or {}).get("formats") or []
    if not names:
        return True
    if not format_name:
        return False
    return format_name in names


def cvar_allowed_for_op(info, op, export_mesh=False, load_format=None, save_format=None):
    """Whether a --jsonconfig entry applies to a Blender operator.

    Import = volume load + mesh save. Voxelize = mesh/image load + mesh save.
    Export = mesh load + volume save (or mesh save when the target is a mesh).
    Format-specific cvars (formats[]) must match the load or save format name.
    """
    if not cvar_has_usage_meta(info):
        return False
    load = bool(info.get("load"))
    save = bool(info.get("save"))
    mesh = bool(info.get("mesh"))
    image = bool(info.get("image"))
    rgb = bool(info.get("rgb"))
    all_fmt = bool(info.get("all"))

    if load and cvar_applies_to_format(info, load_format):
        if op == CVAR_OP_IMPORT:
            if all_fmt or rgb or (not mesh and not image):
                return True
        elif op == CVAR_OP_VOXELIZE:
            if all_fmt or mesh or image or rgb:
                return True
        elif all_fmt or mesh or rgb:
            return True
    if save and cvar_applies_to_format(info, save_format):
        if op == CVAR_OP_IMPORT or op == CVAR_OP_VOXELIZE:
            if all_fmt or mesh:
                return True
        else:
            if mesh:
                return bool(export_mesh)
            return True
    return False


def skip_cvars_not_allowed(keys, op, export_mesh=False, cvars=None, load_format=None, save_format=None):
    if not cvars:
        return ()
    if not any(cvar_has_usage_meta(cvars.get(key)) for key in keys):
        return ()
    return tuple(
        k
        for k in keys
        if not cvar_allowed_for_op(
            cvars.get(k), op, export_mesh, load_format=load_format, save_format=save_format
        )
    )


def format_cvar_cli_value(typ, cur, info=None):
    """Return the voxconvert -set value string for a current operator value."""
    mapped = int_enum_cli_value(cur, info)
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


def cvar_value_changed(typ, cur, default_str, info=None):
    """True if the operator value differs from the --jsonconfig default."""
    mapped = int_enum_cli_value(cur, info)
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


def cvar_set_args(cvars, op, skip_keys=None):
    """Build -set key value CLI args for cvars that differ from defaults."""
    args = []
    if not cvars:
        return args
    skip = set(skip_keys or ())
    for key, info in cvars.items():
        if key in skip:
            continue
        if not hasattr(op, key):
            continue
        cur = getattr(op, key)
        typ = info.get("type", "string")
        default_str = info.get("value", "")
        if not cvar_value_changed(typ, cur, default_str, info=info):
            continue
        args += ["-set", key, format_cvar_cli_value(typ, cur, info=info)]
    return args


# ---------------------------------------------------------------------------
# Process / --progress (stderr)
# ---------------------------------------------------------------------------

_ANSI_RE = re.compile(r"\x1b\[[0-9;?]*[ -/]*[@-~]|\x1b[@-Z\\-_]")
# ProgressBar::format: "[####----]  42% load" (bar width varies; not a TTY => one line per update)
_PROGRESS_RE = re.compile(r"\[[#\-]+\]\s+(\d+)\s*%\s+(.*)$")


def strip_ansi(text):
    """Remove ANSI color/control sequences from log text."""
    if not text:
        return ""
    return _ANSI_RE.sub("", text)


def convert_error_message(stderr_lines, stdout="", limit=600):
    """Prefer ERROR/WARNING lines over the Options dump that leads stderr."""
    picked = []
    for line in stderr_lines or []:
        u = strip_ansi(line).upper()
        if u.startswith("ERROR") or u.startswith("WARNING") or "FAILED" in u:
            picked.append(strip_ansi(line))
    if picked:
        text = "\n".join(picked[-8:])
    else:
        text = "\n".join(stderr_lines or []).strip() or strip_ansi(stdout).strip() or "vengi-voxconvert failed"
        if len(text) > limit:
            text = text[-limit:]
    if len(text) > limit:
        text = text[-limit:]
    return text


def parse_progress_line(line):
    """Parse a voxconvert --progress stderr line. Returns (percent 0-100, name) or None."""
    if not line:
        return None
    s = strip_ansi(line).replace("\r", "\n")
    last = [p for p in s.split("\n") if p.strip()]
    if not last:
        return None
    m = _PROGRESS_RE.search(last[-1].strip())
    if not m:
        return None
    pct = max(0, min(100, int(m.group(1))))
    name = m.group(2).strip()
    return pct, name


def subprocess_hidden_kwargs():
    """Kwargs so Windows does not flash a console window."""
    kw = {}
    if sys.platform == "win32":
        kw["creationflags"] = getattr(subprocess, "CREATE_NO_WINDOW", 0x08000000)
    return kw


def subprocess_popen_kwargs():
    """Kwargs for a killable child (own session on Unix, hidden console on Windows)."""
    kw = subprocess_hidden_kwargs()
    if sys.platform != "win32":
        kw["start_new_session"] = True
    return kw


def kill_process(proc):
    """Terminate a child started with subprocess_popen_kwargs()."""
    if proc is None or proc.poll() is not None:
        return
    try:
        if sys.platform == "win32":
            proc.kill()
            return
        try:
            os.killpg(os.getpgid(proc.pid), signal.SIGTERM)
        except Exception:
            proc.terminate()
        try:
            proc.wait(timeout=1)
        except Exception:
            try:
                os.killpg(os.getpgid(proc.pid), signal.SIGKILL)
            except Exception:
                proc.kill()
    except Exception:
        pass


class ConversionState:
    def __init__(self):
        self.done = False
        self.success = False
        self.cancelled = False
        self.error = ""
        self.output_path = ""
        self.progress = 0
        self.progress_text = ""
        self.proc = None

    def request_cancel(self):
        if self.done:
            return
        self.cancelled = True
        kill_process(self.proc)


def run_command(exe, args, timeout=10):
    """Short-lived voxconvert invocation (jsonconfig / print-formats / version)."""
    kw = subprocess_hidden_kwargs()
    p = subprocess.run([exe] + list(args), capture_output=True, text=True, timeout=timeout, **kw)
    return p.stdout, p.stderr, p.returncode


REQUIRED_VOXCONVERT_VERSION = (0, 6, 0)
_VERSION_RE = re.compile(
    r"(?:^|[\s:])(?:vengi-)?voxconvert(?:\.exe)?\s+(\d+)\.(\d+)\.(\d+)",
    re.IGNORECASE,
)


def parse_voxconvert_version(text):
    """Parse major.minor.patch from `vengi-voxconvert --version` after the binary name."""
    m = _VERSION_RE.search(strip_ansi(text or ""))
    if not m:
        return None
    return (int(m.group(1)), int(m.group(2)), int(m.group(3)))


def format_voxconvert_version(ver):
    if not ver:
        return ""
    return "%d.%d.%d" % tuple(ver)[:3]


def voxconvert_version_atleast(ver, required=None):
    """True if ver is at least required (SDL_VERSION_ATLEAST)."""
    if ver is None:
        return False
    if required is None:
        required = REQUIRED_VOXCONVERT_VERSION
    return tuple(ver)[:3] >= tuple(required)[:3]


def check_voxconvert_version(exe):
    """Return (version, error). error is empty when version is REQUIRED or newer."""
    required = format_voxconvert_version(REQUIRED_VOXCONVERT_VERSION)
    if not exe or not os.path.isfile(exe):
        return None, "vengi-voxconvert not found. Set the path in addon preferences."
    out, err, _rc = run_command(exe, ["--version"])
    ver = parse_voxconvert_version((out or "") + "\n" + (err or ""))
    if ver is None:
        return None, "Could not parse vengi-voxconvert --version (need %s or newer)" % required
    if not voxconvert_version_atleast(ver):
        return ver, "vengi-voxconvert %s or newer required, found %s" % (
            required, format_voxconvert_version(ver)
        )
    return ver, ""


def run_voxconvert(exe, args, state, timeout=600):
    """Run voxconvert with --progress. Updates state from a worker thread."""
    cmd = [exe] + list(args)
    if "--progress" not in cmd:
        cmd.append("--progress")
    try:
        proc = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            bufsize=1,
            **subprocess_popen_kwargs()
        )
    except Exception as e:
        state.success = False
        state.error = strip_ansi(str(e))[:600]
        state.done = True
        return
    state.proc = proc
    if state.cancelled:
        kill_process(proc)

    stdout_chunks = []
    stderr_err = []

    def read_stdout():
        try:
            stdout_chunks.append(proc.stdout.read() or "")
        except Exception:
            pass

    def read_stderr():
        try:
            for line in proc.stderr:
                if state.cancelled:
                    break
                cleaned = strip_ansi(line).replace("\r", "\n")
                for part in cleaned.split("\n"):
                    part = part.strip()
                    if not part:
                        continue
                    parsed = parse_progress_line(part)
                    if parsed is not None:
                        state.progress, state.progress_text = parsed
                    else:
                        stderr_err.append(part)
        except Exception:
            pass

    t_out = threading.Thread(target=read_stdout)
    t_err = threading.Thread(target=read_stderr)
    t_out.daemon = True
    t_err.daemon = True
    t_out.start()
    t_err.start()
    rc = None
    try:
        rc = proc.wait(timeout=timeout)
    except subprocess.TimeoutExpired:
        state.cancelled = True
        kill_process(proc)
        try:
            rc = proc.wait(timeout=5)
        except Exception:
            rc = -1
        state.error = "vengi-voxconvert timed out"
    t_out.join(timeout=2)
    t_err.join(timeout=2)
    try:
        if proc.stdout:
            proc.stdout.close()
        if proc.stderr:
            proc.stderr.close()
    except Exception:
        pass
    if rc == 0:
        state.success = True
        state.cancelled = False
    elif state.cancelled:
        state.success = False
        if not state.error:
            state.error = "Cancelled"
    else:
        state.success = False
        state.error = convert_error_message(stderr_err, "".join(stdout_chunks))
    state.done = True


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
# --print-formats: per-format enum (name + extensions + save + mesh)
# ---------------------------------------------------------------------------
# Import lists voxel/volume formats only (not mesh). Mesh/image files
# go through voxelize mode on the same import operator (--print-formats "mesh" flag + images[]).

_IDENT_UNSAFE_RE = re.compile(r"[^a-z0-9_]+")

ROTATE_ITEMS = (
    ("NONE", "None", "No extra rotation. MagicaVoxel Z-up is already mapped through vengi Y-up and glTF to Blender Z-up"),
    ("x", "X +90", "Rotate 90 degrees around X (--rotate x)"),
    ("y", "Y +90", "Rotate 90 degrees around Y (--rotate y)"),
    ("z", "Z +90", "Rotate 90 degrees around Z (--rotate z)"),
    ("x_180", "X 180", "Rotate 180 degrees around X (--rotate x:180)"),
    ("y_180", "Y 180", "Rotate 180 degrees around Y (--rotate y:180)"),
    ("z_180", "Z 180", "Rotate 180 degrees around Z (--rotate z:180)"),
    ("x_270", "X -90", "Rotate 270 degrees around X (--rotate x:270)"),
    ("y_270", "Y -90", "Rotate 270 degrees around Y (--rotate y:270)"),
    ("z_270", "Z -90", "Rotate 270 degrees around Z (--rotate z:270)"),
)
ROTATE_CLI = {
    "NONE": None,
    "x": "x",
    "y": "y",
    "z": "z",
    "x_180": "x:180",
    "y_180": "y:180",
    "z_180": "z:180",
    "x_270": "x:270",
    "y_270": "y:270",
    "z_270": "z:270",
}
MIRROR_ITEMS = (
    ("NONE", "None", "No mirroring"),
    ("x", "X", "Mirror on X (--mirror x)"),
    ("y", "Y", "Mirror on Y (--mirror y)"),
    ("z", "Z", "Mirror on Z (--mirror z)"),
)


def format_ident(name, used):
    """Stable RNA identifier from a --print-formats name."""
    base = _IDENT_UNSAFE_RE.sub("_", (name or "").lower()).strip("_")
    if not base:
        base = "fmt"
    if base[0].isdigit():
        base = "f_" + base
    ident = base
    n = 2
    while ident in used:
        ident = "%s_%d" % (base, n)
        n += 1
    used.add(ident)
    return ident


def _entries_from_group(raw_list, used):
    out = []
    for raw in raw_list or []:
        name = raw.get("name") or ""
        exts = [e for e in (raw.get("extensions") or []) if e]
        ident = format_ident(name, used)
        out.append({
            "id": ident,
            "name": name or ident,
            "extensions": exts,
            "save": bool(raw.get("save")),
            "mesh": bool(raw.get("mesh")),
        })
    return out


def parse_print_formats(data):
    """Split --print-formats JSON using the mesh/save flags voxconvert already emits.

    volume: voxels without mesh (MagicaVoxel, Qubicle, ...)
    voxelize: voxels with mesh plus images[] (obj, glb, png, md2, ... )
    saveable: voxels with save
    """
    used = set([FORMAT_ALL_ID])
    voxels = _entries_from_group((data or {}).get("voxels"), used)
    images = _entries_from_group((data or {}).get("images"), used)
    volume = [f for f in voxels if not f["mesh"]]
    mesh = [f for f in voxels if f["mesh"]]
    saveable = [f for f in voxels if f["save"]]
    return {
        "volume": volume,
        "saveable": saveable,
        "voxelize": mesh + images,
    }


def _script_enum_list(raw):
    if isinstance(raw, list):
        return [str(x) for x in raw if x not in (None, "")]
    if isinstance(raw, str) and raw:
        sep = ";" if ";" in raw else ","
        return [x.strip() for x in raw.split(sep) if x.strip()]
    return []


def parse_print_scripts(data):
    """Parse --print-scripts JSON into dicts with stable RNA ids."""
    used = set([SCRIPT_NONE_ID])
    scripts = []
    for raw in (data or {}).get("scripts") or []:
        name = raw.get("name") or ""
        ident = format_ident(name, used)
        params = []
        for p in raw.get("parameters") or []:
            params.append({
                "name": p.get("name") or "",
                "type": p.get("type") or "string",
                "description": p.get("description") or "",
                "default": "" if p.get("default") is None else str(p.get("default")),
                "enum": _script_enum_list(p.get("enum")),
                "min": p.get("min"),
                "max": p.get("max"),
            })
        scripts.append({
            "id": ident,
            "name": name,
            "valid": bool(raw.get("valid")),
            "description": raw.get("description") or "",
            "parameters": params,
        })
    return scripts


def find_script(scripts, script_id):
    if not script_id or script_id == SCRIPT_NONE_ID:
        return None
    for script in scripts or []:
        if script.get("id") == script_id:
            return script
    return None


def script_enum_items(scripts):
    items = [(SCRIPT_NONE_ID, "None", "Do not run a lua script")]
    for script in scripts or []:
        if not script.get("valid"):
            continue
        label = script.get("name") or script["id"]
        if label.endswith(".lua"):
            label = label[:-4]
        desc = script.get("description") or script.get("name") or script["id"]
        items.append((script["id"], label, desc[:1024]))
    return items


def quote_script_token(val):
    s = "" if val is None else str(val)
    # Tokenizer treats # and // as comments, and splits on space and (){};
    if not s or any(c in s for c in " (){};\"\\#"):
        return '"%s"' % s.replace("\\", "\\\\").replace('"', '\\"')
    return s


def script_cli_arg(script, arg_values):
    """Build the --script value: 'name.lua arg0 arg1 ...'."""
    if not script:
        return ""
    name = script.get("name") or ""
    if not name:
        return ""
    parts = [name]
    for i, param in enumerate(script.get("parameters") or []):
        if i >= SCRIPT_ARG_MAX:
            break
        if i < len(arg_values):
            val = arg_values[i]
        else:
            val = param.get("default") or ""
        parts.append(quote_script_token(val))
    return " ".join(parts)


def find_format(formats, format_id):
    for fmt in formats or []:
        if fmt.get("id") == format_id:
            return fmt
    return None


def format_enum_items(formats, include_all=True, all_label="All formats"):
    """RNA EnumProperty items from parsed format dicts."""
    items = []
    if include_all:
        items.append((FORMAT_ALL_ID, all_label, "Do not filter by a single format"))
    for fmt in formats or []:
        exts = [e for e in (fmt.get("extensions") or []) if e and "." not in e]
        shown = ", ".join("." + e for e in exts[:3])
        label = "%s (%s)" % (fmt["name"], shown) if shown else fmt["name"]
        items.append((fmt["id"], label, fmt.get("name") or fmt["id"]))
    if not items:
        items.append((FORMAT_ALL_ID, all_label, "Do not filter by a single format"))
    return items


def format_filter_glob(formats, format_id, all_id=FORMAT_ALL_ID):
    """File-browser glob for a format id. ALL joins every extension (or *.* if too long)."""
    if format_id == all_id or not format_id:
        exts = []
        for fmt in formats or []:
            exts.extend(fmt.get("extensions") or [])
        return build_filter_glob(exts)
    fmt = find_format(formats, format_id)
    if not fmt:
        return "*.*"
    return build_filter_glob(fmt.get("extensions") or [])


def format_filename_ext(formats, format_id, all_id=FORMAT_ALL_ID):
    """Primary suffix for ExportHelper, or empty for ALL."""
    if format_id == all_id or not format_id:
        return ""
    fmt = find_format(formats, format_id)
    if not fmt:
        return ""
    for e in fmt.get("extensions") or []:
        if e and "." not in e:
            return "." + e
    if fmt.get("extensions"):
        return "." + fmt["extensions"][0]
    return ""


def default_save_format_id(formats, current_id=None):
    """Keep the current format if it is still in the list, else the first saveable."""
    if current_id and current_id != FORMAT_ALL_ID and find_format(formats, current_id):
        return current_id
    if formats:
        return formats[0]["id"]
    return FORMAT_ALL_ID


def ensure_filepath_ext(filepath, filename_ext):
    """Replace or append filename_ext (ExportHelper / file-browser filename)."""
    if not filepath or not filename_ext:
        return filepath
    desired = filename_ext if filename_ext.startswith(".") else "." + filename_ext
    directory, name = os.path.split(filepath)
    if not name:
        return filepath
    stem, ext = os.path.splitext(name)
    if stem.startswith(".") and not ext:
        stem, ext = "", stem
    if ext.lower() == desired.lower():
        return filepath
    new_name = stem + desired
    if directory:
        return os.path.join(directory, new_name)
    return new_name


def sync_filename_for_format(filename, formats, format_id):
    """Replace the filename suffix with the selected save format's extension."""
    ext = format_filename_ext(formats, format_id)
    if not ext:
        return filename
    return ensure_filepath_ext(filename, ext)


def operator_idnames_match(bl_idname, dotted, rna):
    """True if bl_idname is the dotted or RNA form, including a dotted extension prefix."""
    if not bl_idname:
        return False
    if bl_idname in (dotted, rna):
        return True
    return bl_idname.endswith("." + dotted) or bl_idname.endswith("." + rna)


def path_matches_extensions(path, extensions):
    name = os.path.basename(path or "").lower()
    if not name:
        return False
    for ext in extensions or []:
        e = (ext or "").lower().lstrip(".")
        if not e:
            continue
        if name.endswith("." + e):
            return True
    return False


def path_allowed_for_format(path, formats, format_id, all_id=FORMAT_ALL_ID):
    """True if path's suffix matches the selected format (or any format when ALL)."""
    if format_id == all_id or not format_id:
        return any(path_matches_extensions(path, f.get("extensions")) for f in (formats or []))
    fmt = find_format(formats, format_id)
    if not fmt:
        return False
    return path_matches_extensions(path, fmt.get("extensions"))


def import_has_input_file(path):
    """True if path is an existing file. Empty paths and directories are not."""
    return bool(path) and os.path.isfile(path)


def import_transform_args(rotate="NONE", mirror="NONE"):
    """CLI args for --rotate / --mirror."""
    args = []
    rot = ROTATE_CLI.get(rotate, rotate)
    if rot and rot != "NONE":
        args += ["--rotate", rot]
    if mirror and mirror != "NONE":
        args += ["--mirror", mirror]
    return args


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
    program_files=None,
    program_files_x86=None,
    user_install_dir=None,
    which=None,
    use_well_known=True,
):
    """Locate vengi-voxconvert the way VoxConvertUI/sysFindBinary does, plus OS paths.

    Search order (cwd is not searched; a random binary there is not the install):
    1. PATH (shutil.which)
    2. sibling to the addon folder, and the addon folder itself
    3. user install dir (Windows download target)
    4. OS-specific: Program Files, /usr/bin, Mac .app bundle
    """
    system = system or sys.platform
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
