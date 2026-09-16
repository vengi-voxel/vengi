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
    from . import util
except ImportError:
    import util

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
    return util.find_voxconvert(addon_dir=_addon_dir(), user_install_dir=_user_voxconvert_dir())


def _gltf_rna_prop_names():
    return util.gltf_export_rna_prop_names(bpy)


def _require_gltf(op):
    ok, msg = util.ensure_gltf_addon(addon_utils, bpy)
    if not ok:
        op.report({'ERROR'}, msg)
        return False
    return True


def _get_json(exe, flag):
    out, _, _ = util.run_command(exe, [flag], timeout=10)
    return util.parse_leading_json(out)


# ---------------------------------------------------------------------------
# Cached metadata
# ---------------------------------------------------------------------------

_cache = {
    "cvars": None,
    "exe": "",
    "version": None,
    "version_error": "",
    "voxel_formats": [],
    "save_formats": [],
    "voxelize_formats": [],
    "import_enum_items": (),
    "export_enum_items": (),
    "voxelize_enum_items": (),
}
# Keep the last few EnumProperty item tuples alive (Blender holds char pointers).
_ENUM_ITEMS_KEEP_MAX = 8
_enum_items_keep = []


def _pin_enum_items(*groups):
    for items in groups:
        _enum_items_keep.append(tuple(items))
    del _enum_items_keep[:-_ENUM_ITEMS_KEEP_MAX]


def _empty_format_filters():
    _cache["voxel_formats"] = []
    _cache["save_formats"] = []
    _cache["voxelize_formats"] = []
    _cache["import_enum_items"] = ()
    _cache["export_enum_items"] = ()
    _cache["voxelize_enum_items"] = ()


def _refresh_cache(exe):
    _cache["exe"] = ""
    _cache["version"] = None
    _cache["version_error"] = ""
    _cache["cvars"] = None
    _empty_format_filters()
    ver, err = util.check_voxconvert_version(exe)
    _cache["version"] = ver
    if err:
        _cache["version_error"] = err
        return
    _cache["exe"] = exe
    parsed = util.parse_print_formats(_get_json(exe, "--print-formats"))
    _cache["cvars"] = _get_json(exe, "--jsonconfig")
    _cache["voxel_formats"] = parsed["volume"]
    _cache["save_formats"] = parsed["saveable"]
    _cache["voxelize_formats"] = parsed["voxelize"]
    _cache["import_enum_items"] = tuple(
        util.format_enum_items(parsed["volume"], include_all=True, all_label="All voxel formats")
    )
    _cache["export_enum_items"] = tuple(util.format_enum_items(parsed["saveable"], include_all=False))
    _cache["voxelize_enum_items"] = tuple(
        util.format_enum_items(parsed["voxelize"], include_all=True, all_label="All mesh/image formats")
    )
    _pin_enum_items(
        _cache["import_enum_items"],
        _cache["export_enum_items"],
        _cache["voxelize_enum_items"],
    )


# ---------------------------------------------------------------------------
# Build bpy properties from cvar JSON
# ---------------------------------------------------------------------------

def _make_prop(key, info):
    """Return a bpy property definition for a single cvar, or None."""
    typ = info.get("type", "string")
    val = info.get("value", "")
    helptext = util.cvar_help(info, key)
    name = util.cvar_rna_name(info, key)

    enum_items = util.int_enum_rna_items(info)
    if enum_items:
        ident = util.int_enum_ident_from_cli(val, info)
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
        mn, mx = util.cvar_numeric_bounds(info)
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
        mn, mx = util.cvar_numeric_bounds(info)
        if mn is not None:
            kwargs["min"] = mn
        if mx is not None:
            kwargs["max"] = mx
        return FloatProperty(**kwargs)

    subtype = util.cvar_path_subtype(info)
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
        if not util.is_relevant_cvar(info):
            continue
        prop = _make_prop(key, info)
        if prop is not None:
            annotations[key] = prop
    return annotations


def _cvar_keys():
    cvars = _cache.get("cvars")
    if not cvars:
        return []
    return [k for k, v in cvars.items() if util.is_relevant_cvar(v)]


def _cvar_set_args(context, skip_keys=None):
    settings = getattr(context.window_manager, "vengi_cvars", None)
    if settings is None:
        return []
    return util.cvar_set_args(_cache.get("cvars") or {}, settings, skip_keys=skip_keys)


def _export_mesh_output(file_format):
    return util.format_is_mesh(_save_formats(), file_format)


def _gltf_preview_name():
    """Name of the glTF format used for the Blender mesh preview round-trip."""
    return util.gltf_format_name(_voxelize_formats()) or util.gltf_format_name(_save_formats())


def _skip_cvars(kind, file_format=None, filepath=None):
    cvars = _cache.get("cvars")
    export_mesh = bool(kind == util.CVAR_OP_EXPORT and file_format and _export_mesh_output(file_format))
    load_format = None
    save_format = None
    if kind == util.CVAR_OP_IMPORT:
        load_format = util.resolve_format_name(_volume_formats(), file_format, filepath=filepath)
        save_format = _gltf_preview_name()
    elif kind == util.CVAR_OP_VOXELIZE:
        load_format = util.resolve_format_name(_voxelize_formats(), file_format, filepath=filepath)
        save_format = _gltf_preview_name()
    else:
        load_format = _gltf_preview_name()
        save_format = util.resolve_format_name(_save_formats(), file_format, filepath=filepath)
    return util.skip_cvars_not_allowed(
        _cvar_keys(),
        kind,
        export_mesh=export_mesh,
        cvars=cvars,
        load_format=load_format,
        save_format=save_format,
    )


_cvar_group_cls = None
_active_converts = []


def _unregister_cvar_group():
    global _cvar_group_cls
    if hasattr(bpy.types.WindowManager, "vengi_cvars"):
        del bpy.types.WindowManager.vengi_cvars
    cls = _cvar_group_cls
    _cvar_group_cls = None
    if cls is not None and cls.is_registered:
        bpy.utils.unregister_class(cls)


def _rebuild_cvar_group():
    """Recreate the WindowManager PropertyGroup from current --jsonconfig."""
    _unregister_cvar_group()
    annotations = _make_cvar_annotations()
    cls = type("VENGI_PG_cvars", (PropertyGroup,), {"__annotations__": annotations})
    bpy.utils.register_class(cls)
    global _cvar_group_cls
    _cvar_group_cls = cls
    bpy.types.WindowManager.vengi_cvars = PointerProperty(type=cls)


# ---------------------------------------------------------------------------
# Non-blocking conversion
# ---------------------------------------------------------------------------

def _start_convert(op, context, exe, args, tmpdir, output_path=""):
    op._tmpdir = tmpdir
    op._state = util.ConversionState()
    op._state.output_path = output_path
    _active_converts.append(op)
    t = threading.Thread(target=util.run_voxconvert, args=(exe, args, op._state))
    t.daemon = True
    t.start()
    op._timer = context.window_manager.event_timer_add(0.1, window=context.window)
    op._progress_active = False
    try:
        context.window_manager.progress_begin(0, 100)
        op._progress_active = True
    except Exception:
        pass
    context.window_manager.modal_handler_add(op)
    return {'RUNNING_MODAL'}


def _cleanup_convert(op, context=None):
    if context is not None:
        try:
            context.workspace.status_text_set(None)
        except Exception:
            pass
    tmpdir = getattr(op, "_tmpdir", None)
    if tmpdir:
        shutil.rmtree(tmpdir, ignore_errors=True)
        op._tmpdir = None
    try:
        _active_converts.remove(op)
    except ValueError:
        pass


def _cancel_active_converts():
    for op in list(_active_converts):
        state = getattr(op, "_state", None)
        if state is not None:
            state.request_cancel()
        _cleanup_convert(op)


def _modal_convert(op, context, event, error_prefix="Conversion failed: "):
    """Drive progress / cancel. Returns a set if the convert finished or was cancelled."""
    wm = context.window_manager
    if event.type in {'ESC'}:
        op._state.request_cancel()
        return {'RUNNING_MODAL'}
    if event.type != 'TIMER':
        return {'PASS_THROUGH'}
    if op._progress_active:
        try:
            wm.progress_update(int(op._state.progress))
        except Exception:
            pass
        text = op._state.progress_text
        if text:
            try:
                context.workspace.status_text_set("%d%% %s" % (int(op._state.progress), text))
            except Exception:
                pass
    if not op._state.done:
        return {'RUNNING_MODAL'}
    wm.event_timer_remove(op._timer)
    if op._progress_active:
        try:
            wm.progress_end()
        except Exception:
            pass
        op._progress_active = False
    if op._state.cancelled:
        _cleanup_convert(op, context)
        op.report({'INFO'}, "Cancelled")
        return {'CANCELLED'}
    if not op._state.success:
        op.report({'ERROR'}, error_prefix + util.strip_ansi(op._state.error))
        _cleanup_convert(op, context)
        return {'CANCELLED'}
    return None


# ---------------------------------------------------------------------------
# Draw cvars in the file browser sidebar
# ---------------------------------------------------------------------------

def _ui_props(layout):
    layout.use_property_split = True
    layout.use_property_decorate = False
    return layout


def _section_column(layout, title, icon='NONE'):
    """Box with an unsplit header so property-split tables stay aligned."""
    box = layout.box()
    header = box.row()
    header.use_property_split = False
    header.use_property_decorate = False
    header.label(text=title, icon=icon)
    return _ui_props(box.column())


def _draw_cvars(layout, context, op, skip=()):
    settings = getattr(context.window_manager, "vengi_cvars", None)
    if settings is None:
        return
    cvars = _cache.get("cvars") or {}
    prim, fmt, adv = util.split_cvar_keys(_cvar_keys(), cvars, skip=skip)
    col = _section_column(layout, "Voxconvert", 'PREFERENCES')
    for key in prim:
        if hasattr(settings, key):
            col.prop(settings, key)
    if fmt:
        fmt_col = _section_column(layout, "Format options")
        for key in fmt:
            if hasattr(settings, key):
                fmt_col.prop(settings, key)
    if not adv:
        return
    row = col.row()
    row.use_property_split = False
    row.prop(op, "show_advanced", toggle=True)
    if not op.show_advanced:
        return
    adv_col = _section_column(layout, "Advanced")
    for key in adv:
        if hasattr(settings, key):
            adv_col.prop(settings, key)


def _draw_axis(layout, op):
    col = _section_column(layout, "Axis / scale", 'EMPTY_ARROWS')
    col.prop(op, "rotate")
    col.prop(op, "mirror")
    col.prop(op, "import_scale")


def _volume_formats():
    return _cache.get("voxel_formats") or []


def _save_formats():
    return _cache.get("save_formats") or []


def _voxelize_formats():
    return _cache.get("voxelize_formats") or []


_ENUM_FALLBACK = ((util.FORMAT_ALL_ID, "All formats", "vengi-voxconvert --print-formats not loaded yet"),)


def _keep_enum_items(items):
    if not items:
        items = _ENUM_FALLBACK
    items = tuple(items)
    _pin_enum_items(items)
    return items


def _import_format_items(self, context):
    return _cache.get("import_enum_items") or _keep_enum_items(
        util.format_enum_items(_volume_formats(), include_all=True, all_label="All voxel formats")
    )


def _export_format_items(self, context):
    return _cache.get("export_enum_items") or _keep_enum_items(
        util.format_enum_items(_save_formats(), include_all=False)
    )


def _voxelize_format_items(self, context):
    return _cache.get("voxelize_enum_items") or _keep_enum_items(
        util.format_enum_items(_voxelize_formats(), include_all=True, all_label="All mesh/image formats")
    )


def _invoke_import_browser(op, context, formats):
    if not _cache.get("exe"):
        op.report({'ERROR'}, _exe_error())
        return {'CANCELLED'}
    op.filter_glob = util.format_filter_glob(formats, op.file_format)
    context.window_manager.fileselect_add(op)
    return {'RUNNING_MODAL'}


def _update_import_format(self, context):
    self.filter_glob = util.format_filter_glob(_volume_formats(), self.file_format)


def _apply_export_format(op):
    fmts = _save_formats()
    op.filter_glob = util.format_filter_glob(fmts, op.file_format)
    ext = util.format_filename_ext(fmts, op.file_format)
    if ext:
        op.filename_ext = ext
    return ext


def _file_browser_spaces(context):
    sfile = getattr(context, "space_data", None)
    if sfile is not None and getattr(sfile, "type", None) == 'FILE_BROWSER':
        yield sfile
        return
    wm = getattr(context, "window_manager", None)
    if wm is None:
        return
    for window in wm.windows:
        screen = getattr(window, "screen", None)
        if screen is None:
            continue
        for area in screen.areas:
            if area.type != 'FILE_BROWSER':
                continue
            space = area.spaces.active
            if space is not None:
                yield space


def _sync_export_file_browser(op, context):
    """Match glTF: rewrite the visible filename and filter when Save as changes."""
    ext = _apply_export_format(op)
    if getattr(op, "filepath", None) and ext:
        op.filepath = util.ensure_filepath_ext(op.filepath, ext)
    for sfile in _file_browser_spaces(context):
        active = getattr(sfile, "active_operator", None)
        if active is not None and not util.operator_idnames_match(
            getattr(active, "bl_idname", ""),
            "export_scene.vengi_voxconvert",
            "EXPORT_SCENE_OT_vengi_voxconvert",
        ):
            continue
        params = getattr(sfile, "params", None)
        if params is None:
            continue
        if ext:
            params.filename = util.sync_filename_for_format(
                params.filename, _save_formats(), op.file_format
            )
        if op.filter_glob:
            params.filter_glob = op.filter_glob
        try:
            bpy.ops.file.refresh()
        except Exception:
            pass
        return


def _update_export_format(self, context):
    _sync_export_file_browser(self, context)


def _update_voxelize_format(self, context):
    self.filter_glob = util.format_filter_glob(_voxelize_formats(), self.file_format)


def _import_file_error(voxelize=False):
    if voxelize:
        return "Expected a mesh or image file from vengi-voxconvert --print-formats"
    return (
        "This importer is for voxel/volume files. "
        "Use File > Import > Vengi voxelize and import for mesh and image formats."
    )


def _exe_error():
    return _cache.get("version_error") or "vengi-voxconvert not found. Set path in addon preferences."


def _start_file_import(op, context, formats, format_id, voxelize=False):
    exe = _cache.get("exe")
    if not exe:
        op.report({'ERROR'}, _exe_error())
        return {'CANCELLED'}
    if not _require_gltf(op):
        return {'CANCELLED'}
    if not util.path_allowed_for_format(op.filepath, formats, format_id):
        op.report({'ERROR'}, _import_file_error(voxelize=voxelize))
        return {'CANCELLED'}
    tmpdir = tempfile.mkdtemp(prefix="vengi_")
    tmp_out = os.path.join(tmpdir, "out.glb")
    args = ["--input", op.filepath, "--output", tmp_out, "--force"]
    args += util.import_transform_args(op.rotate, op.mirror, op.import_scale)
    kind = util.CVAR_OP_VOXELIZE if voxelize else util.CVAR_OP_IMPORT
    args += _cvar_set_args(
        context,
        skip_keys=_skip_cvars(kind, format_id, filepath=op.filepath),
    )
    return _start_convert(op, context, exe, args, tmpdir, output_path=tmp_out)


def _modal_file_import(op, context, event):
    result = _modal_convert(op, context, event)
    if result is not None:
        return result
    if not os.path.isfile(op._state.output_path):
        op.report({'ERROR'}, "Conversion produced no output file")
        _cleanup_convert(op, context)
        return {'CANCELLED'}
    try:
        bpy.ops.import_scene.gltf(filepath=op._state.output_path)
    except Exception as e:
        op.report({'ERROR'}, "glTF import failed: " + str(e)[:300])
        _cleanup_convert(op, context)
        return {'CANCELLED'}
    _cleanup_convert(op, context)
    op.report({'INFO'}, "Imported via vengi-voxconvert (meshed preview, not voxels)")
    return {'FINISHED'}


# ---------------------------------------------------------------------------
# Operators
# ---------------------------------------------------------------------------

class IMPORT_SCENE_OT_vengi_voxconvert(Operator, ImportHelper):
    bl_idname = "import_scene.vengi_voxconvert"
    bl_label = "Import Vengi Voxel"
    bl_description = "Import a voxel/volume file as a meshed preview (voxel -> voxconvert -> GLB -> Blender mesh)"
    bl_options = {'REGISTER', 'UNDO', 'PRESET'}
    filename_ext = ""

    filter_glob: StringProperty(default="*.*", options={'HIDDEN'}, maxlen=util.FILTER_GLOB_MAX)
    filepath: StringProperty(subtype='FILE_PATH')
    file_format: EnumProperty(
        name="Format",
        description="Filter the file browser to one voxel format from --print-formats",
        items=_import_format_items,
        update=_update_import_format,
    )
    show_advanced: BoolProperty(
        name="Advanced",
        default=False,
        description="Show additional voxconvert cvars that apply to this direction",
    )
    rotate: EnumProperty(
        name="Rotate",
        items=util.ROTATE_ITEMS,
        default="NONE",
        description="Optional --rotate after load (Blender is Z-up; MagicaVoxel is converted by vengi already)",
    )
    mirror: EnumProperty(
        name="Mirror",
        items=util.MIRROR_ITEMS,
        default="NONE",
        description="Optional --mirror after load",
    )
    import_scale: FloatProperty(
        name="Scale",
        default=1.0,
        min=0.001,
        max=100.0,
        description="Uniform scale (voxformat_scale)",
    )

    def invoke(self, context, event):
        return _invoke_import_browser(self, context, _volume_formats())

    def execute(self, context):
        return _start_file_import(self, context, _volume_formats(), self.file_format, voxelize=False)

    def modal(self, context, event):
        return _modal_file_import(self, context, event)

    def draw(self, context):
        layout = self.layout
        _ui_props(layout)
        box = _section_column(layout, "Format", 'FILE_FOLDER')
        box.prop(self, "file_format")
        hint = box.row()
        hint.use_property_split = False
        hint.label(text="Imports as a mesh preview, not voxels")
        _draw_axis(layout, self)
        _draw_cvars(
            layout,
            context,
            self,
            skip=_skip_cvars(util.CVAR_OP_IMPORT, self.file_format, filepath=self.filepath),
        )


class IMPORT_SCENE_OT_vengi_voxelize(Operator, ImportHelper):
    bl_idname = "import_scene.vengi_voxelize"
    bl_label = "Voxelize and Import"
    bl_description = "Voxelize a mesh or image from vengi-voxconvert --print-formats, then import the mesh preview"
    bl_options = {'REGISTER', 'UNDO', 'PRESET'}
    filename_ext = ""

    filter_glob: StringProperty(default="*.*", options={'HIDDEN'}, maxlen=util.FILTER_GLOB_MAX)
    filepath: StringProperty(subtype='FILE_PATH')
    file_format: EnumProperty(
        name="Format",
        description="Filter to a mesh or image format voxconvert can voxelize",
        items=_voxelize_format_items,
        update=_update_voxelize_format,
    )
    show_advanced: BoolProperty(
        name="Advanced",
        default=False,
        description="Show additional voxconvert cvars that apply to this direction",
    )
    rotate: EnumProperty(
        name="Rotate",
        items=util.ROTATE_ITEMS,
        default="NONE",
        description="Optional --rotate after voxelize",
    )
    mirror: EnumProperty(
        name="Mirror",
        items=util.MIRROR_ITEMS,
        default="NONE",
        description="Optional --mirror after voxelize",
    )
    import_scale: FloatProperty(
        name="Scale",
        default=1.0,
        min=0.001,
        max=100.0,
        description="Uniform scale (voxformat_scale)",
    )

    def invoke(self, context, event):
        return _invoke_import_browser(self, context, _voxelize_formats())

    def execute(self, context):
        return _start_file_import(self, context, _voxelize_formats(), self.file_format, voxelize=True)

    def modal(self, context, event):
        return _modal_file_import(self, context, event)

    def draw(self, context):
        layout = self.layout
        _ui_props(layout)
        box = _section_column(layout, "Format", 'FILE_FOLDER')
        box.prop(self, "file_format")
        hint = box.row()
        hint.use_property_split = False
        hint.label(text="Voxelize, then import as a mesh preview")
        _draw_axis(layout, self)
        _draw_cvars(
            layout,
            context,
            self,
            skip=_skip_cvars(util.CVAR_OP_VOXELIZE, self.file_format, filepath=self.filepath),
        )


class EXPORT_SCENE_OT_vengi_voxconvert(Operator, ExportHelper):
    bl_idname = "export_scene.vengi_voxconvert"
    bl_label = "Export Vengi Voxel"
    bl_description = "Export the scene through glTF, voxelize with voxconvert, and save a voxel format"
    bl_options = {'REGISTER', 'UNDO', 'PRESET'}
    filename_ext = ".vox"
    check_extension = True

    filter_glob: StringProperty(default="*.vox", options={'HIDDEN'}, maxlen=util.FILTER_GLOB_MAX)
    file_format: EnumProperty(
        name="Format",
        description="Output format from vengi-voxconvert --print-formats (sets the filename extension)",
        items=_export_format_items,
        update=_update_export_format,
    )
    show_advanced: BoolProperty(
        name="Advanced",
        default=False,
        description="Show additional voxconvert cvars that apply to this direction",
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

    def invoke(self, context, event):
        if not _cache.get("exe"):
            self.report({'ERROR'}, _exe_error())
            return {'CANCELLED'}
        self.file_format = util.default_save_format_id(_save_formats(), self.file_format)
        _apply_export_format(self)
        if self.filepath and self.filename_ext:
            self.filepath = util.ensure_filepath_ext(self.filepath, self.filename_ext)
        result = ExportHelper.invoke(self, context, event)
        _sync_export_file_browser(self, context)
        return result

    def check(self, context):
        ext = util.format_filename_ext(_save_formats(), self.file_format)
        if ext:
            self.filename_ext = ext
        return ExportHelper.check(self, context)

    def execute(self, context):
        exe = _cache.get("exe")
        if not exe:
            self.report({'ERROR'}, _exe_error())
            return {'CANCELLED'}
        if not _require_gltf(self):
            return {'CANCELLED'}
        tmpdir = tempfile.mkdtemp(prefix="vengi_")
        tmp_in = os.path.join(tmpdir, "scene.glb")
        try:
            bpy.ops.export_scene.gltf(**util.gltf_export_kwargs(
                tmp_in,
                blender_version=bpy.app.version,
                rna_prop_names=_gltf_rna_prop_names(),
                use_selection=self.use_selection,
                apply_modifiers=self.apply_modifiers,
            ))
        except Exception as e:
            self.report({'ERROR'}, "glTF export failed: " + str(e)[:300])
            shutil.rmtree(tmpdir, ignore_errors=True)
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
        args += _cvar_set_args(
            context,
            skip_keys=_skip_cvars(util.CVAR_OP_EXPORT, self.file_format, filepath=self.filepath),
        )
        return _start_convert(self, context, exe, args, tmpdir)

    def modal(self, context, event):
        result = _modal_convert(self, context, event, error_prefix="Export failed: ")
        if result is not None:
            return result
        _cleanup_convert(self, context)
        self.report({'INFO'}, "Exported via vengi-voxconvert")
        return {'FINISHED'}

    def draw(self, context):
        layout = self.layout
        _ui_props(layout)
        if _save_formats():
            layout.prop(self, "file_format", text="Save as")
        else:
            layout.label(text="Set vengi-voxconvert Path in Preferences to list formats", icon='ERROR')
        blender = _section_column(layout, "Blender", 'BLENDER')
        blender.prop(self, "use_selection")
        blender.prop(self, "apply_modifiers")
        ops = _section_column(layout, "Operations", 'MODIFIER')
        ops.prop(self, "crop")
        ops.prop(self, "merge")
        ops.prop(self, "scale_half")
        ops.prop(self, "script")
        skip = _skip_cvars(util.CVAR_OP_EXPORT, self.file_format, filepath=self.filepath)
        _draw_cvars(layout, context, self, skip=skip)


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
            path = util.download_windows_voxconvert(dest)
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
            n = sum(1 for v in cvars.values() if util.is_relevant_cvar(v))
            ver = util.format_voxconvert_version(_cache.get("version"))
            layout.label(text="vengi-voxconvert %s (%d settings)" % (ver, n), icon='CHECKMARK')
        else:
            layout.label(text=_exe_error(), icon='ERROR')
            if sys.platform == "win32":
                layout.operator("vengi_voxconvert.download_binary", icon='IMPORT')


def _on_exe_changed(exe):
    _refresh_cache(exe)
    _rebuild_cvar_group()


# ---------------------------------------------------------------------------
# Menu entries
# ---------------------------------------------------------------------------

def _menu_import(self, context):
    self.layout.operator("import_scene.vengi_voxconvert", text="Vengi voxel (.vox, .qb, ...)")
    self.layout.operator("import_scene.vengi_voxelize", text="Vengi voxelize mesh/image")


def _menu_export(self, context):
    self.layout.operator("export_scene.vengi_voxconvert", text="Vengi voxel (.vox, .qb, ...)")


# ---------------------------------------------------------------------------
# Registration
# ---------------------------------------------------------------------------

def register():
    exe = _find_voxconvert()
    _refresh_cache(exe)

    bpy.utils.register_class(VENGI_OT_download_voxconvert)
    bpy.utils.register_class(VengiVoxconvertPreferences)

    if __package__:
        addon = bpy.context.preferences.addons.get(__package__)
        if addon is not None:
            executable = addon.preferences.executable
            if executable and executable != exe:
                _refresh_cache(executable)

    _rebuild_cvar_group()
    bpy.utils.register_class(IMPORT_SCENE_OT_vengi_voxconvert)
    bpy.utils.register_class(IMPORT_SCENE_OT_vengi_voxelize)
    bpy.utils.register_class(EXPORT_SCENE_OT_vengi_voxconvert)

    TOPBAR_MT_file_import.append(_menu_import)
    TOPBAR_MT_file_export.append(_menu_export)


def unregister():
    _cancel_active_converts()
    TOPBAR_MT_file_export.remove(_menu_export)
    TOPBAR_MT_file_import.remove(_menu_import)
    bpy.utils.unregister_class(EXPORT_SCENE_OT_vengi_voxconvert)
    bpy.utils.unregister_class(IMPORT_SCENE_OT_vengi_voxelize)
    bpy.utils.unregister_class(IMPORT_SCENE_OT_vengi_voxconvert)
    _unregister_cvar_group()
    bpy.utils.unregister_class(VengiVoxconvertPreferences)
    bpy.utils.unregister_class(VENGI_OT_download_voxconvert)


if __name__ == "__main__":
    register()
