# SPDX-License-Identifier: MIT
# Blender addon for vengi-voxconvert
# Dynamically builds UI from voxconvert's --jsonconfig and --print-formats
# JSON outputs so new cvars, formats and parameters are picked up automatically.

bl_info = {
    "name": "Vengi Voxconvert",
    "author": "Martin Gerhardy",
    "version": (1, 0, 0),
    "blender": (3, 6, 0),
    "location": "File > Import/Export",
    "description": "Import/Export voxel formats via vengi-voxconvert",
    "category": "Import-Export",
    "doc_url": "https://vengi-voxel.github.io/vengi/voxconvert/Blender/",
    "tracker_url": "https://github.com/vengi-voxel/vengi/issues",
}

import bpy
import addon_utils
import os
import shutil
import subprocess
import sys
import tempfile
import threading
from bpy.props import (
    StringProperty, BoolProperty, IntProperty, FloatProperty, EnumProperty, PointerProperty,
)
from bpy.types import (
    AddonPreferences, Operator, PropertyGroup,
    TOPBAR_MT_file_import, TOPBAR_MT_file_export,
)
from bpy_extras.io_utils import ImportHelper, ExportHelper

try:
    from .util import (
        FILTER_GLOB_MAX,
        build_filter_glob,
        cvar_help,
        cvar_numeric_bounds,
        cvar_path_subtype,
        cvar_rna_name,
        cvar_set_args,
        download_windows_voxconvert,
        ensure_gltf_addon,
        find_voxconvert,
        gltf_export_kwargs,
        gltf_export_rna_prop_names,
        int_enum_ident_from_cli,
        int_enum_rna_items,
        is_relevant_cvar,
        parse_leading_json,
        split_cvar_keys,
        IMPORT_PRIMARY_CVARS,
        EXPORT_PRIMARY_CVARS,
    )
except ImportError:
    from util import (
        FILTER_GLOB_MAX,
        build_filter_glob,
        cvar_help,
        cvar_numeric_bounds,
        cvar_path_subtype,
        cvar_rna_name,
        cvar_set_args,
        download_windows_voxconvert,
        ensure_gltf_addon,
        find_voxconvert,
        gltf_export_kwargs,
        gltf_export_rna_prop_names,
        int_enum_ident_from_cli,
        int_enum_rna_items,
        is_relevant_cvar,
        parse_leading_json,
        split_cvar_keys,
        IMPORT_PRIMARY_CVARS,
        EXPORT_PRIMARY_CVARS,
    )

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _addon_dir():
    return os.path.dirname(os.path.abspath(__file__))


def _user_voxconvert_dir():
    try:
        return bpy.utils.user_resource("SCRIPTS", path="vengi")
    except Exception:
        return ""


def _find_voxconvert():
    return find_voxconvert(addon_dir=_addon_dir(), user_install_dir=_user_voxconvert_dir())


def _gltf_rna_prop_names():
    return gltf_export_rna_prop_names(bpy)


def _require_gltf(op):
    ok, msg = ensure_gltf_addon(addon_utils, bpy)
    if not ok:
        op.report({'ERROR'}, msg)
        return False
    return True


def _run(exe, args, timeout=600):
    cmd = [exe] + args
    p = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout)
    return p.stdout, p.stderr, p.returncode


def _get_json(exe, flag):
    out, _, _ = _run(exe, [flag], timeout=10)
    return parse_leading_json(out)


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

def _is_relevant_cvar(key, info):
    return is_relevant_cvar(key, info)


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
    _cache["import_filter"] = build_filter_glob(imp)
    _cache["export_filter"] = build_filter_glob(exp)


# ---------------------------------------------------------------------------
# Build bpy properties from cvar JSON
# ---------------------------------------------------------------------------

def _make_prop(key, info):
    """Return a bpy property definition for a single cvar, or None."""
    typ = info.get("type", "string")
    val = info.get("value", "")
    helptext = cvar_help(info, key)
    name = cvar_rna_name(info, key)

    enum_items = int_enum_rna_items(key)
    if enum_items:
        ident = int_enum_ident_from_cli(key, val)
        return EnumProperty(name=name, items=enum_items, default=ident, description=helptext)

    if typ == "boolean":
        default = (val.lower() == "true") if isinstance(val, str) else bool(val)
        return BoolProperty(name=name, default=default, description=helptext)

    if typ == "enum":
        valid = info.get("valid_values", [])
        if not valid:
            return StringProperty(name=name, default=str(val), description=helptext)
        items = [(v, v, "") for v in valid]
        default = val if val in valid else valid[0]
        return EnumProperty(name=name, items=items, default=default, description=helptext)

    if typ == "int":
        try:
            default = int(val)
        except (ValueError, TypeError):
            default = 0
        kwargs = dict(name=name, default=default, description=helptext)
        mn, mx = cvar_numeric_bounds(info)
        if mn is not None:
            kwargs["min"] = mn
        if mx is not None:
            kwargs["max"] = mx
        return IntProperty(**kwargs)

    if typ == "float":
        try:
            default = float(val)
        except (ValueError, TypeError):
            default = 0.0
        kwargs = dict(name=name, default=default, description=helptext)
        mn, mx = cvar_numeric_bounds(info)
        if mn is not None:
            kwargs["min"] = mn
        if mx is not None:
            kwargs["max"] = mx
        return FloatProperty(**kwargs)

    subtype = cvar_path_subtype(info)
    if subtype:
        return StringProperty(name=name, default=str(val), description=helptext, subtype=subtype)

    return StringProperty(name=name, default=str(val), description=helptext)


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


def _cvar_keys():
    cvars = _cache.get("cvars")
    if not cvars:
        return []
    return [k for k, v in cvars.items() if _is_relevant_cvar(k, v)]


def _cvar_set_args(context):
    settings = getattr(context.window_manager, "vengi_cvars", None)
    if settings is None:
        return []
    return cvar_set_args(_cache.get("cvars") or {}, settings)


_cvar_group_cls = None


def _rebuild_cvar_group():
    """Recreate the WindowManager PropertyGroup from current --jsonconfig."""
    global _cvar_group_cls
    if hasattr(bpy.types.WindowManager, "vengi_cvars"):
        del bpy.types.WindowManager.vengi_cvars
    if _cvar_group_cls is not None:
        try:
            bpy.utils.unregister_class(_cvar_group_cls)
        except Exception:
            pass
        _cvar_group_cls = None
    annotations = _make_cvar_annotations()
    cls = type("VENGI_PG_cvars", (PropertyGroup,), {"__annotations__": annotations})
    bpy.utils.register_class(cls)
    bpy.types.WindowManager.vengi_cvars = PointerProperty(type=cls)
    _cvar_group_cls = cls


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

def _draw_cvars(layout, context, op, primary):
    settings = getattr(context.window_manager, "vengi_cvars", None)
    if settings is None:
        return
    prim, adv = split_cvar_keys(_cvar_keys(), primary)
    box = layout.box()
    box.label(text="Voxconvert", icon='PREFERENCES')
    for key in prim:
        if hasattr(settings, key):
            box.prop(settings, key)
    if not adv:
        return
    box.prop(op, "show_advanced", toggle=True)
    if not op.show_advanced:
        return
    adv_box = layout.box()
    adv_box.label(text="Advanced")
    for key in adv:
        if hasattr(settings, key):
            adv_box.prop(settings, key)


# ---------------------------------------------------------------------------
# Operators
# ---------------------------------------------------------------------------

class IMPORT_SCENE_OT_vengi_voxconvert(Operator, ImportHelper):
    bl_idname = "import_scene.vengi_voxconvert"
    bl_label = "Import Vengi Voxconvert"
    bl_options = {'REGISTER', 'UNDO', 'PRESET'}
    filename_ext = ""

    filter_glob: StringProperty(default="*.*", options={'HIDDEN'}, maxlen=FILTER_GLOB_MAX)
    filepath: StringProperty(subtype='FILE_PATH')
    show_advanced: BoolProperty(
        name="Advanced",
        default=False,
        description="Show all other voxconvert cvars",
    )

    def invoke(self, context, event):
        self.filter_glob = _cache.get("import_filter", "*.*")
        context.window_manager.fileselect_add(self)
        return {'RUNNING_MODAL'}

    def execute(self, context):
        exe = _cache.get("exe")
        if not exe:
            self.report({'ERROR'}, "vengi-voxconvert not found. Set path in addon preferences.")
            return {'CANCELLED'}
        if not _require_gltf(self):
            return {'CANCELLED'}
        self._tmpdir = tempfile.mkdtemp(prefix="vengi_")
        tmp_out = os.path.join(self._tmpdir, "out.glb")
        args = ["--input", self.filepath, "--output", tmp_out, "--force"]
        args += _cvar_set_args(context)
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
        _draw_cvars(self.layout, context, self, IMPORT_PRIMARY_CVARS)


class EXPORT_SCENE_OT_vengi_voxconvert(Operator, ExportHelper):
    bl_idname = "export_scene.vengi_voxconvert"
    bl_label = "Export Vengi Voxconvert"
    bl_options = {'REGISTER', 'UNDO', 'PRESET'}
    filename_ext = ""
    check_extension = False

    filter_glob: StringProperty(default="*.*", options={'HIDDEN'}, maxlen=FILTER_GLOB_MAX)
    filepath: StringProperty(subtype='FILE_PATH')
    show_advanced: BoolProperty(
        name="Advanced",
        default=False,
        description="Show all other voxconvert cvars",
    )
    use_selection: BoolProperty(
        name="Selected objects only",
        default=False,
        description="Export only selected objects through glTF",
    )
    apply_modifiers: BoolProperty(
        name="Apply modifiers",
        default=False,
        description="Apply modifiers when exporting the glTF mesh",
    )
    crop: BoolProperty(name="Crop", default=False, description="Reduce models to real voxel sizes")
    merge: BoolProperty(name="Merge", default=False, description="Merge models into one volume")
    scale_half: BoolProperty(name="Scale 50%", default=False, description="Scale model to 50%")
    script: StringProperty(name="Lua Script", default="", description="Apply a lua script to the output")
    filter_nodes: StringProperty(name="Filter", default="", description="Model filter e.g. '1-4,6'")

    def invoke(self, context, event):
        self.filter_glob = _cache.get("export_filter", "*.*")
        context.window_manager.fileselect_add(self)
        return {'RUNNING_MODAL'}

    def execute(self, context):
        exe = _cache.get("exe")
        if not exe:
            self.report({'ERROR'}, "vengi-voxconvert not found. Set path in addon preferences.")
            return {'CANCELLED'}
        if not _require_gltf(self):
            return {'CANCELLED'}
        self._tmpdir = tempfile.mkdtemp(prefix="vengi_")
        tmp_in = os.path.join(self._tmpdir, "scene.glb")
        try:
            bpy.ops.export_scene.gltf(**gltf_export_kwargs(
                tmp_in,
                blender_version=bpy.app.version,
                rna_prop_names=_gltf_rna_prop_names(),
                use_selection=self.use_selection,
                apply_modifiers=self.apply_modifiers,
            ))
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
        args += _cvar_set_args(context)
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
        box.label(text="Blender", icon='BLENDER')
        box.prop(self, "use_selection")
        box.prop(self, "apply_modifiers")
        box = layout.box()
        box.label(text="Operations", icon='MODIFIER')
        box.prop(self, "crop")
        box.prop(self, "merge")
        box.prop(self, "scale_half")
        box.prop(self, "script")
        box.prop(self, "filter_nodes")
        _draw_cvars(layout, context, self, EXPORT_PRIMARY_CVARS)


# ---------------------------------------------------------------------------
# Preferences
# ---------------------------------------------------------------------------

class VENGI_OT_download_voxconvert(Operator):
    """Download win-voxconvert.zip from the latest GitHub release (Windows only)."""
    bl_idname = "vengi_voxconvert.download_binary"
    bl_label = "Download vengi-voxconvert"
    bl_description = "Download win-voxconvert.zip from GitHub releases into Blender's user scripts/vengi folder"

    def execute(self, context):
        if sys.platform != "win32":
            self.report({'ERROR'}, "Automatic download is only available on Windows")
            return {'CANCELLED'}
        dest = bpy.utils.user_resource("SCRIPTS", path="vengi", create=True)
        try:
            path = download_windows_voxconvert(dest)
        except Exception as e:
            self.report({'ERROR'}, "Download failed: " + str(e)[:400])
            return {'CANCELLED'}
        prefs = context.preferences.addons[__package__].preferences
        prefs.executable = path
        self.report({'INFO'}, "Installed vengi-voxconvert to " + path)
        return {'FINISHED'}


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
        else:
            layout.label(text="vengi-voxconvert not found - set the path above", icon='ERROR')
            if sys.platform == "win32":
                layout.operator("vengi_voxconvert.download_binary", icon='IMPORT')


def _on_exe_changed(exe):
    _refresh_cache(exe)
    try:
        _rebuild_cvar_group()
    except Exception:
        pass


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

def register():
    exe = _find_voxconvert()
    _refresh_cache(exe)

    bpy.utils.register_class(VENGI_OT_download_voxconvert)
    bpy.utils.register_class(VengiVoxconvertPreferences)

    try:
        prefs = bpy.context.preferences.addons[__package__].preferences
        if prefs.executable and prefs.executable != exe:
            _refresh_cache(prefs.executable)
    except Exception:
        pass

    _rebuild_cvar_group()
    bpy.utils.register_class(IMPORT_SCENE_OT_vengi_voxconvert)
    bpy.utils.register_class(EXPORT_SCENE_OT_vengi_voxconvert)

    TOPBAR_MT_file_import.append(_menu_import)
    TOPBAR_MT_file_export.append(_menu_export)


def unregister():
    TOPBAR_MT_file_export.remove(_menu_export)
    TOPBAR_MT_file_import.remove(_menu_import)
    bpy.utils.unregister_class(EXPORT_SCENE_OT_vengi_voxconvert)
    bpy.utils.unregister_class(IMPORT_SCENE_OT_vengi_voxconvert)
    if hasattr(bpy.types.WindowManager, "vengi_cvars"):
        del bpy.types.WindowManager.vengi_cvars
    global _cvar_group_cls
    if _cvar_group_cls is not None:
        try:
            bpy.utils.unregister_class(_cvar_group_cls)
        except Exception:
            pass
        _cvar_group_cls = None
    bpy.utils.unregister_class(VengiVoxconvertPreferences)
    bpy.utils.unregister_class(VENGI_OT_download_voxconvert)


if __name__ == "__main__":
    register()
