# SPDX-License-Identifier: MIT
# Blender addon for vengi-voxconvert
# Dynamically builds UI from voxconvert's --jsonconfig and --print-formats
# JSON outputs so new cvars, formats and parameters are picked up automatically.

bl_info = {
    "name": "Vengi Voxconvert",
    "author": "vengi-voxel",
    "version": (1, 0, 0),
    "blender": (3, 6, 0),
    "location": "File > Import/Export",
    "description": "Import/Export voxel formats via vengi-voxconvert",
    "category": "Import-Export",
    "doc_url": "https://vengi-voxel.github.io/vengi/",
    "tracker_url": "https://github.com/vengi-voxel/vengi/issues",
}

import bpy
import json
import os
import shutil
import subprocess
import tempfile
import threading
from bpy.props import (
    StringProperty, BoolProperty, IntProperty, FloatProperty, EnumProperty,
)
from bpy.types import (
    AddonPreferences, Operator,
    TOPBAR_MT_file_import, TOPBAR_MT_file_export,
)
from bpy_extras.io_utils import ImportHelper, ExportHelper

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _find_voxconvert():
    path = shutil.which("vengi-voxconvert")
    if path:
        return path
    for c in ("/usr/bin/vengi-voxconvert", "/usr/local/bin/vengi-voxconvert"):
        if os.path.isfile(c):
            return c
    return ""


def _run(exe, args, timeout=600):
    cmd = [exe] + args
    p = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout)
    return p.stdout, p.stderr, p.returncode


def _get_json(exe, flag):
    out, _, _ = _run(exe, [flag], timeout=10)
    for i, ch in enumerate(out):
        if ch in ('{', '['):
            out = out[i:]
            break
    try:
        return json.loads(out)
    except Exception:
        return None


# ---------------------------------------------------------------------------
# Cached metadata
# ---------------------------------------------------------------------------

_cache = {
    "formats": None,
    "cvars": None,
    "exe": "",
    "import_filter": "*.*",
    "export_filter": "*.*",
}

# Skip cvars that are internal / not useful in a Blender context
_SKIP_PREFIXES = ("app_", "core_", "metric_")
# Flag bit 0 = readonly
_FLAG_READONLY = 1


def _is_relevant_cvar(key, info):
    if any(key.startswith(p) for p in _SKIP_PREFIXES):
        return False
    if info.get("flags", 0) & _FLAG_READONLY:
        return False
    if info.get("type") == "unknown":
        return False
    return True


def _refresh_cache(exe):
    _cache["exe"] = ""
    _cache["formats"] = None
    _cache["cvars"] = None
    if not exe or not os.path.isfile(exe):
        return
    _cache["exe"] = exe
    _cache["formats"] = _get_json(exe, "--print-formats")
    _cache["cvars"] = _get_json(exe, "--jsonconfig")
    imp, exp = set(), set()
    fmt = _cache["formats"]
    if fmt:
        for entry in fmt.get("voxels", []):
            for ext in entry.get("extensions", []):
                imp.add(ext.lower())
                if entry.get("save"):
                    exp.add(ext.lower())
    _cache["import_filter"] = ";".join("*." + e for e in sorted(imp)) if imp else "*.*"
    _cache["export_filter"] = ";".join("*." + e for e in sorted(exp)) if exp else "*.*"


# ---------------------------------------------------------------------------
# Build bpy properties from cvar JSON
# ---------------------------------------------------------------------------

def _make_prop(key, info):
    """Return a bpy property definition for a single cvar, or None."""
    typ = info.get("type", "string")
    val = info.get("value", "")
    helptext = info.get("help", key)

    if typ == "boolean":
        default = (val.lower() == "true") if isinstance(val, str) else bool(val)
        return BoolProperty(name=key, default=default, description=helptext)

    if typ == "enum":
        valid = info.get("valid_values", [])
        if not valid:
            return StringProperty(name=key, default=str(val), description=helptext)
        items = [(v, v, "") for v in valid]
        default = val if val in valid else valid[0]
        return EnumProperty(name=key, items=items, default=default, description=helptext)

    if typ == "int":
        try:
            default = int(val)
        except (ValueError, TypeError):
            default = 0
        return IntProperty(name=key, default=default, description=helptext)

    if typ == "float":
        try:
            default = float(val)
        except (ValueError, TypeError):
            default = 0.0
        return FloatProperty(name=key, default=default, description=helptext)

    # string / fallback
    return StringProperty(name=key, default=str(val), description=helptext)


def _make_cvar_annotations():
    """Build a dict of {cvar_key: bpy_property} from cached cvars."""
    annotations = {}
    cvars = _cache.get("cvars")
    if not cvars:
        return annotations
    for key, info in cvars.items():
        if not _is_relevant_cvar(key, info):
            continue
        prop = _make_prop(key, info)
        if prop is not None:
            annotations[key] = prop
    return annotations


def _cvar_keys_sorted():
    """Return sorted list of relevant cvar keys."""
    cvars = _cache.get("cvars")
    if not cvars:
        return []
    return sorted(k for k, v in cvars.items() if _is_relevant_cvar(k, v))


def _cvar_set_args(op):
    """Build -set key value CLI args for cvars that differ from defaults."""
    args = []
    cvars = _cache.get("cvars") or {}
    for key, info in cvars.items():
        if not hasattr(op, key):
            continue
        cur = getattr(op, key)
        typ = info.get("type", "string")
        default_str = info.get("value", "")
        if typ == "boolean":
            cur_str = "true" if cur else "false"
        else:
            cur_str = str(cur)
        if cur_str != default_str:
            args += ["-set", key, cur_str]
    return args


# ---------------------------------------------------------------------------
# Non-blocking conversion
# ---------------------------------------------------------------------------

class _ConversionState:
    def __init__(self):
        self.done = False
        self.success = False
        self.error = ""
        self.output_path = ""


def _convert_thread(exe, args, state):
    try:
        out, err, rc = _run(exe, args)
        state.success = (rc == 0)
        state.error = (err or out)[:600] if rc != 0 else ""
    except Exception as e:
        state.success = False
        state.error = str(e)[:600]
    state.done = True


# ---------------------------------------------------------------------------
# Draw cvars in the file browser sidebar
# ---------------------------------------------------------------------------

def _draw_cvars(layout, op):
    cvars = _cache.get("cvars")
    if not cvars:
        return
    box = layout.box()
    box.label(text="Voxconvert Settings", icon='PREFERENCES')
    for key in _cvar_keys_sorted():
        if hasattr(op, key):
            box.prop(op, key)


# ---------------------------------------------------------------------------
# Dynamic operator class creation
# ---------------------------------------------------------------------------

def _make_import_class():
    annotations = {
        "filter_glob": StringProperty(default="*.*", options={'HIDDEN'}),
        "filepath": StringProperty(subtype='FILE_PATH'),
    }
    annotations.update(_make_cvar_annotations())

    def invoke(self, context, event):
        self.filter_glob = _cache.get("import_filter", "*.*")
        context.window_manager.fileselect_add(self)
        return {'RUNNING_MODAL'}

    def execute(self, context):
        exe = _cache.get("exe")
        if not exe:
            self.report({'ERROR'}, "vengi-voxconvert not found. Set path in addon preferences.")
            return {'CANCELLED'}
        self._tmpdir = tempfile.mkdtemp(prefix="vengi_")
        tmp_out = os.path.join(self._tmpdir, "out.glb")
        args = ["--input", self.filepath, "--output", tmp_out, "--force"]
        args += _cvar_set_args(self)
        self._state = _ConversionState()
        self._state.output_path = tmp_out
        t = threading.Thread(target=_convert_thread, args=(exe, args, self._state))
        t.daemon = True
        t.start()
        self._timer = context.window_manager.event_timer_add(0.1, window=context.window)
        context.window_manager.modal_handler_add(self)
        return {'RUNNING_MODAL'}

    def modal(self, context, event):
        if event.type != 'TIMER':
            return {'PASS_THROUGH'}
        if not self._state.done:
            return {'PASS_THROUGH'}
        context.window_manager.event_timer_remove(self._timer)
        if not self._state.success:
            self.report({'ERROR'}, "Conversion failed: " + self._state.error)
            shutil.rmtree(self._tmpdir, ignore_errors=True)
            return {'CANCELLED'}
        if not os.path.isfile(self._state.output_path):
            self.report({'ERROR'}, "Conversion produced no output file")
            shutil.rmtree(self._tmpdir, ignore_errors=True)
            return {'CANCELLED'}
        try:
            bpy.ops.import_scene.gltf(filepath=self._state.output_path)
        except Exception as e:
            self.report({'ERROR'}, "glTF import failed: " + str(e)[:300])
            shutil.rmtree(self._tmpdir, ignore_errors=True)
            return {'CANCELLED'}
        shutil.rmtree(self._tmpdir, ignore_errors=True)
        self.report({'INFO'}, "Imported via vengi-voxconvert")
        return {'FINISHED'}

    def draw(self, context):
        _draw_cvars(self.layout, self)

    cls = type("IMPORT_SCENE_OT_vengi_voxconvert", (Operator, ImportHelper), {
        "__annotations__": annotations,
        "bl_idname": "import_scene.vengi_voxconvert",
        "bl_label": "Import Vengi Voxconvert",
        "bl_options": {'REGISTER', 'UNDO', 'PRESET'},
        "filename_ext": "",
        "invoke": invoke,
        "execute": execute,
        "modal": modal,
        "draw": draw,
        "_state": None,
        "_timer": None,
        "_tmpdir": None,
    })
    return cls


def _make_export_class():
    annotations = {
        "filter_glob": StringProperty(default="*.*", options={'HIDDEN'}),
        "filepath": StringProperty(subtype='FILE_PATH'),
        "crop": BoolProperty(name="Crop", default=False, description="Reduce models to real voxel sizes"),
        "merge": BoolProperty(name="Merge", default=False, description="Merge models into one volume"),
        "scale_half": BoolProperty(name="Scale 50%", default=False, description="Scale model to 50%"),
        "script": StringProperty(name="Lua Script", default="", description="Apply a lua script to the output"),
        "filter_nodes": StringProperty(name="Filter", default="", description="Model filter e.g. '1-4,6'"),
    }
    annotations.update(_make_cvar_annotations())

    def invoke(self, context, event):
        self.filter_glob = _cache.get("export_filter", "*.*")
        context.window_manager.fileselect_add(self)
        return {'RUNNING_MODAL'}

    def execute(self, context):
        exe = _cache.get("exe")
        if not exe:
            self.report({'ERROR'}, "vengi-voxconvert not found. Set path in addon preferences.")
            return {'CANCELLED'}
        self._tmpdir = tempfile.mkdtemp(prefix="vengi_")
        tmp_in = os.path.join(self._tmpdir, "scene.glb")
        try:
            bpy.ops.export_scene.gltf(
                filepath=tmp_in, export_format='GLB',
                export_colors=True, export_materials='EXPORT',
            )
        except Exception as e:
            self.report({'ERROR'}, "glTF export failed: " + str(e)[:300])
            shutil.rmtree(self._tmpdir, ignore_errors=True)
            return {'CANCELLED'}
        args = ["--input", tmp_in, "--output", self.filepath, "--force"]
        if self.crop:
            args.append("--crop")
        if self.merge:
            args.append("--merge")
        if self.scale_half:
            args.append("--scale")
        if self.script:
            args += ["--script", self.script]
        if self.filter_nodes:
            args += ["--filter", self.filter_nodes]
        args += _cvar_set_args(self)
        self._state = _ConversionState()
        t = threading.Thread(target=_convert_thread, args=(exe, args, self._state))
        t.daemon = True
        t.start()
        self._timer = context.window_manager.event_timer_add(0.1, window=context.window)
        context.window_manager.modal_handler_add(self)
        return {'RUNNING_MODAL'}

    def modal(self, context, event):
        if event.type != 'TIMER':
            return {'PASS_THROUGH'}
        if not self._state.done:
            return {'PASS_THROUGH'}
        context.window_manager.event_timer_remove(self._timer)
        shutil.rmtree(self._tmpdir, ignore_errors=True)
        if not self._state.success:
            self.report({'ERROR'}, "Export failed: " + self._state.error)
            return {'CANCELLED'}
        self.report({'INFO'}, "Exported via vengi-voxconvert")
        return {'FINISHED'}

    def draw(self, context):
        layout = self.layout
        box = layout.box()
        box.label(text="Operations", icon='MODIFIER')
        box.prop(self, "crop")
        box.prop(self, "merge")
        box.prop(self, "scale_half")
        box.prop(self, "script")
        box.prop(self, "filter_nodes")
        _draw_cvars(layout, self)

    cls = type("EXPORT_SCENE_OT_vengi_voxconvert", (Operator, ExportHelper), {
        "__annotations__": annotations,
        "bl_idname": "export_scene.vengi_voxconvert",
        "bl_label": "Export Vengi Voxconvert",
        "bl_options": {'REGISTER', 'UNDO', 'PRESET'},
        "filename_ext": ".vox",
        "invoke": invoke,
        "execute": execute,
        "modal": modal,
        "draw": draw,
        "_state": None,
        "_timer": None,
        "_tmpdir": None,
    })
    return cls


# ---------------------------------------------------------------------------
# Preferences
# ---------------------------------------------------------------------------

class VengiVoxconvertPreferences(AddonPreferences):
    bl_idname = __package__

    executable: StringProperty(
        name="vengi-voxconvert Path",
        description="Path to the vengi-voxconvert executable",
        subtype='FILE_PATH',
        default=_find_voxconvert(),
        update=lambda self, ctx: _on_exe_changed(self.executable),
    )

    def draw(self, context):
        layout = self.layout
        layout.prop(self, "executable")
        exe = _cache.get("exe")
        if exe:
            cvars = _cache.get("cvars") or {}
            n = sum(1 for k, v in cvars.items() if _is_relevant_cvar(k, v))
            layout.label(text="Loaded %d settings from vengi-voxconvert" % n, icon='CHECKMARK')
            layout.label(text="Restart Blender after changing the executable to reload settings", icon='INFO')
        else:
            layout.label(text="vengi-voxconvert not found - set the path above", icon='ERROR')


def _on_exe_changed(exe):
    _refresh_cache(exe)


# ---------------------------------------------------------------------------
# Menu entries
# ---------------------------------------------------------------------------

def _menu_import(self, context):
    self.layout.operator("import_scene.vengi_voxconvert", text="Vengi Voxconvert (.vox, .qb, ...)")


def _menu_export(self, context):
    self.layout.operator("export_scene.vengi_voxconvert", text="Vengi Voxconvert (.vox, .qb, ...)")


# ---------------------------------------------------------------------------
# Registration
# ---------------------------------------------------------------------------

_dynamic_classes = []


def register():
    exe = _find_voxconvert()
    _refresh_cache(exe)

    bpy.utils.register_class(VengiVoxconvertPreferences)

    # re-read saved preference before building dynamic classes
    try:
        prefs = bpy.context.preferences.addons[__package__].preferences
        if prefs.executable and prefs.executable != exe:
            _refresh_cache(prefs.executable)
    except Exception:
        pass

    imp_cls = _make_import_class()
    exp_cls = _make_export_class()
    _dynamic_classes.clear()
    _dynamic_classes.extend([imp_cls, exp_cls])

    for cls in _dynamic_classes:
        bpy.utils.register_class(cls)

    TOPBAR_MT_file_import.append(_menu_import)
    TOPBAR_MT_file_export.append(_menu_export)


def unregister():
    TOPBAR_MT_file_export.remove(_menu_export)
    TOPBAR_MT_file_import.remove(_menu_import)
    for cls in reversed(_dynamic_classes):
        bpy.utils.unregister_class(cls)
    _dynamic_classes.clear()
    bpy.utils.unregister_class(VengiVoxconvertPreferences)


if __name__ == "__main__":
    register()
