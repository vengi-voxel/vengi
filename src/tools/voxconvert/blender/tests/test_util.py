#!/usr/bin/env python3
# SPDX-License-Identifier: MIT

import os
import sys
import tempfile
import types
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "io_vengi_voxconvert"))

from util import (
    CVAR_OP_EXPORT,
    CVAR_OP_IMPORT,
    CVAR_OP_VOXELIZE,
    FORMAT_ALL_ID,
    REQUIRED_VOXCONVERT_VERSION,
    build_filter_glob,
    cvar_allowed_for_op,
    cvar_set_args,
    default_save_format_id,
    find_voxconvert,
    format_filename_ext,
    gltf_export_kwargs,
    gltf_format_name,
    int_enum_cli_value,
    int_enum_ident_from_cli,
    is_relevant_cvar,
    operator_idnames_match,
    parse_leading_json,
    parse_print_formats,
    parse_progress_line,
    parse_voxconvert_version,
    path_allowed_for_format,
    resolve_format_name,
    split_cvar_keys,
    strip_ansi,
    voxconvert_version_atleast,
)


class TestHelpers(unittest.TestCase):
    def test_filter_glob(self):
        self.assertEqual(build_filter_glob([".VOX", "qb"]), "*.qb;*.vox")
        self.assertEqual(build_filter_glob(["ben.json", "osm.json"]), "*.*")
        self.assertEqual(build_filter_glob(["vox", "qb"], max_len=5), "*.*")

    def test_json_and_progress(self):
        self.assertEqual(parse_leading_json('INFO:\n{"a": 1}')["a"], 1)
        self.assertEqual(strip_ansi("\033[31mERROR\033[00m"), "ERROR")
        self.assertEqual(parse_progress_line("[##------]  25% save"), (25, "save"))

    def test_version(self):
        self.assertEqual(REQUIRED_VOXCONVERT_VERSION, (0, 6, 0))
        self.assertEqual(parse_voxconvert_version("vengi-voxconvert 0.6.0.0"), (0, 6, 0))
        self.assertEqual(parse_voxconvert_version("INFO: voxconvert 0.6.0\n"), (0, 6, 0))
        self.assertEqual(parse_voxconvert_version("vengi-voxconvert 0.6.1"), (0, 6, 1))
        self.assertEqual(parse_voxconvert_version("vengi-voxconvert.exe 0.6.0"), (0, 6, 0))
        self.assertIsNone(parse_voxconvert_version("not a version"))
        self.assertIsNone(parse_voxconvert_version("failed with 1.2.3 in the log"))
        self.assertFalse(voxconvert_version_atleast(None))
        self.assertFalse(voxconvert_version_atleast((0, 5, 9)))
        self.assertTrue(voxconvert_version_atleast((0, 6, 0)))
        self.assertTrue(voxconvert_version_atleast((0, 6, 1)))
        self.assertTrue(voxconvert_version_atleast((1, 0, 0)))

    def test_gltf_kwargs(self):
        old = gltf_export_kwargs("/t.glb", blender_version=(3, 6, 0), use_selection=True)
        self.assertTrue(old["export_colors"])
        self.assertTrue(old["use_selection"])
        new = gltf_export_kwargs("/t.glb", blender_version=(4, 2, 0), rna_prop_names=["export_vertex_color"])
        self.assertEqual(new["export_vertex_color"], "ACTIVE")
        self.assertNotIn("use_selection", new)

class TestCvars(unittest.TestCase):
    def test_relevant_and_split(self):
        self.assertTrue(is_relevant_cvar({"load": True, "type": "path"}))
        self.assertFalse(is_relevant_cvar({"type": "int"}))
        self.assertFalse(is_relevant_cvar({"load": True, "readonly": True}))
        self.assertTrue(is_relevant_cvar({"load": True, "flags": 1}))
        cvars = {
            "palette": {"primary": True, "order": 1},
            "voxformat_binvoxversion": {"order": 2, "formats": ["BinVox"]},
            "voxformat_mergequads": {"order": 0},
        }
        self.assertEqual(
            split_cvar_keys(list(cvars), cvars),
            (["palette"], ["voxformat_binvoxversion"], ["voxformat_mergequads"]),
        )

    def test_allowed_for_op(self):
        gltf = "GL Transmission Format"
        meshmode = {"save": True, "mesh": True}
        fill = {"load": True, "mesh": True}
        magica = {"load": True, "formats": ["MagicaVoxel"]}
        binvox = {"save": True, "formats": ["BinVox"]}
        self.assertTrue(cvar_allowed_for_op(meshmode, CVAR_OP_IMPORT, save_format=gltf))
        self.assertFalse(cvar_allowed_for_op(meshmode, CVAR_OP_EXPORT, load_format=gltf, save_format="MagicaVoxel"))
        self.assertTrue(cvar_allowed_for_op(fill, CVAR_OP_VOXELIZE, load_format="Wavefront Object"))
        self.assertFalse(cvar_allowed_for_op(fill, CVAR_OP_IMPORT, load_format="MagicaVoxel", save_format=gltf))
        self.assertTrue(cvar_allowed_for_op(magica, CVAR_OP_IMPORT, load_format="MagicaVoxel", save_format=gltf))
        self.assertFalse(cvar_allowed_for_op(magica, CVAR_OP_IMPORT, save_format=gltf))
        self.assertTrue(cvar_allowed_for_op(binvox, CVAR_OP_EXPORT, load_format=gltf, save_format="BinVox"))
        self.assertFalse(cvar_allowed_for_op(binvox, CVAR_OP_EXPORT, load_format=gltf, save_format="MagicaVoxel"))
        quads = {"save": True, "mesh": True, "formats": ["Wavefront Object", "Polygon File Format"]}
        self.assertFalse(cvar_allowed_for_op(quads, CVAR_OP_IMPORT, save_format=gltf))
        self.assertTrue(cvar_allowed_for_op(quads, CVAR_OP_EXPORT, export_mesh=True, save_format="Wavefront Object"))

    def test_value_titles_and_set_args(self):
        info = {"value_titles": ["Cubes", "Marching cubes", "Binary"]}
        self.assertEqual(int_enum_ident_from_cli("2", info), "binary")
        self.assertEqual(int_enum_cli_value("cubes", info), "0")
        cvars = {"m": {"type": "int", "value": "2", "value_titles": info["value_titles"]}}
        self.assertEqual(cvar_set_args(cvars, types.SimpleNamespace(m="cubes")), ["-set", "m", "0"])
        self.assertEqual(cvar_set_args(cvars, types.SimpleNamespace(m="binary")), [])
        scale = {"voxformat_scale": {"type": "float", "value": "1.000000"}}
        self.assertEqual(cvar_set_args(scale, types.SimpleNamespace(voxformat_scale=1.0)), [])

    def test_operator_idnames(self):
        dotted = "export_scene.vengi_voxconvert"
        rna = "EXPORT_SCENE_OT_vengi_voxconvert"
        self.assertTrue(operator_idnames_match(dotted, dotted, rna))
        self.assertTrue(operator_idnames_match(rna, dotted, rna))
        self.assertTrue(operator_idnames_match("bl_ext.foo." + dotted, dotted, rna))
        self.assertFalse(operator_idnames_match("my" + dotted, dotted, rna))

    def test_find_voxconvert_extra_dir(self):
        with tempfile.TemporaryDirectory() as d:
            path = os.path.join(d, "vengi-voxconvert")
            with open(path, "w"):
                pass
            os.chmod(path, 0o755)
            found = find_voxconvert(
                extra_dirs=[d],
                which=lambda *a, **k: None,
                use_well_known=False,
            )
            self.assertEqual(os.path.abspath(found), os.path.abspath(path))


class TestPrintFormats(unittest.TestCase):
    SAMPLE = {
        "voxels": [
            {"name": "OtherVox", "extensions": ["vox"], "save": True},
            {"name": "MagicaVoxel", "extensions": ["vox"], "save": True},
            {"name": "NotGLTF", "extensions": ["glb"], "mesh": True},
            {"name": "GL Transmission Format", "extensions": ["glb", "gltf"], "save": True, "mesh": True},
            {"name": "Quake 2 Model", "extensions": ["md2"], "mesh": True},
        ],
        "images": [{"name": "PNG", "extensions": ["png"], "save": True}],
    }

    def test_split_and_paths(self):
        parsed = parse_print_formats(self.SAMPLE)
        self.assertEqual({e for f in parsed["volume"] for e in f["extensions"]}, {"vox"})
        self.assertEqual(
            {e for f in parsed["voxelize"] for e in f["extensions"]},
            {"glb", "gltf", "md2", "png"},
        )
        self.assertTrue(path_allowed_for_format("a.vox", parsed["volume"], FORMAT_ALL_ID))
        self.assertFalse(path_allowed_for_format("a.obj", parsed["volume"], FORMAT_ALL_ID))
        ident = parsed["volume"][0]["id"]
        self.assertTrue(path_allowed_for_format("a.vox", parsed["volume"], ident))
        self.assertFalse(path_allowed_for_format("a.obj", parsed["volume"], ident))
        self.assertFalse(path_allowed_for_format("a.vox", parsed["volume"], "no_such_format"))
        ident = default_save_format_id(parsed["saveable"])
        self.assertEqual(
            next(f["name"] for f in parsed["saveable"] if f["id"] == ident),
            "OtherVox",
        )
        self.assertEqual(format_filename_ext(parsed["saveable"], ident), ".vox")
        self.assertEqual(gltf_format_name(parsed["voxelize"]), "GL Transmission Format")
        self.assertEqual(resolve_format_name(parsed["volume"], FORMAT_ALL_ID, filepath="/t.vox"), "OtherVox")


if __name__ == "__main__":
    unittest.main()
