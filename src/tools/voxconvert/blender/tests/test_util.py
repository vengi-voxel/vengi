#!/usr/bin/env python3
# SPDX-License-Identifier: MIT

import os
import sys
import types
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "io_vengi_voxconvert"))

from util import (
    FILTER_GLOB_MAX,
    build_filter_glob,
    cvar_help,
    cvar_numeric_bounds,
    cvar_path_subtype,
    cvar_rna_name,
    cvar_set_args,
    cvar_value_changed,
    format_cvar_cli_value,
)


class TestBuildFilterGlob(unittest.TestCase):
    def test_empty(self):
        self.assertEqual(build_filter_glob([]), "*.*")
        self.assertEqual(build_filter_glob(None), "*.*")

    def test_short_list_is_joined(self):
        self.assertEqual(build_filter_glob(["vox", "qb"]), "*.qb;*.vox")

    def test_strips_dot_and_lowercases(self):
        self.assertEqual(build_filter_glob([".VOX", "Qb"]), "*.qb;*.vox")

    def test_skips_compound_extensions(self):
        self.assertEqual(build_filter_glob(["vox", "ben.json", "osm.json"]), "*.vox")

    def test_compound_only_falls_back(self):
        self.assertEqual(build_filter_glob(["ben.json", "vmax.zip"]), "*.*")

    def test_over_max_len_falls_back(self):
        exts = ["e%02d" % i for i in range(80)]
        joined_if_kept = ";".join("*." + e for e in sorted(exts))
        self.assertGreater(len(joined_if_kept), FILTER_GLOB_MAX)
        self.assertEqual(build_filter_glob(exts), "*.*")

    def test_fits_max_len_is_kept(self):
        self.assertEqual(build_filter_glob(["vox", "qb", "vengi"], max_len=40), "*.qb;*.vengi;*.vox")

    def test_realistic_voxconvert_import_list_does_not_exceed_limit(self):
        # 76 unique voxel extensions as produced by --print-formats
        exts = [
            "3ds", "3zh", "ase", "aseprite", "b64", "bbmodel", "ben", "ben.json",
            "bin", "binvox", "bp", "bsp", "csm", "csv", "cub", "dat", "fbx",
            "glb", "gltf", "gml", "gox", "kenshape", "kv6", "kvx", "ldr",
            "litematic", "map", "mca", "mcr", "mcskin", "mcworld", "md2", "md3",
            "mdl", "mpd", "mts", "nbt", "nvm", "obj", "osm.json", "particubes",
            "pcubes", "ply", "png", "qb", "qbcl", "qbt", "qef", "schem",
            "schematic", "scn", "smd2", "smd3", "sment", "smtpl", "stl", "thing",
            "txt", "v3a", "v3b", "vbx", "vengi", "vmax.zip", "vmaxb", "vox",
            "voxa", "vrm", "vxb", "vxc", "vxl", "vxm", "vxr", "vxt", "xml",
            "xraw", "zip",
        ]
        glob = build_filter_glob(exts)
        self.assertLessEqual(len(glob), FILTER_GLOB_MAX)
        self.assertEqual(glob, "*.*")


class TestCvarValueChanged(unittest.TestCase):
    def test_float_jsonconfig_padding_is_not_a_change(self):
        self.assertFalse(cvar_value_changed("float", 1.0, "1.000000"))
        self.assertFalse(cvar_value_changed("float", 0.0, "0.000000"))
        self.assertTrue(cvar_value_changed("float", 2.0, "1.000000"))

    def test_int_string_default(self):
        self.assertFalse(cvar_value_changed("int", 2, "2"))
        self.assertTrue(cvar_value_changed("int", 3, "2"))

    def test_boolean_string_default(self):
        self.assertFalse(cvar_value_changed("boolean", True, "true"))
        self.assertFalse(cvar_value_changed("boolean", False, "false"))
        self.assertTrue(cvar_value_changed("boolean", True, "false"))

    def test_enum_and_string(self):
        self.assertFalse(cvar_value_changed("enum", "y", "y"))
        self.assertTrue(cvar_value_changed("enum", "x", "y"))
        self.assertFalse(cvar_value_changed("string", "nippon", "nippon"))
        self.assertTrue(cvar_value_changed("string", "built-in:nippon", "nippon"))


class TestCvarSetArgs(unittest.TestCase):
    def test_unchanged_floats_are_not_emitted(self):
        cvars = {
            "voxformat_scale": {"type": "float", "value": "1.000000"},
            "voxformat_scale_x": {"type": "float", "value": "1.000000"},
            "voxformat_merge": {"type": "boolean", "value": "false"},
            "voxformat_meshmode": {"type": "int", "value": "2"},
        }
        op = types.SimpleNamespace(
            voxformat_scale=1.0,
            voxformat_scale_x=1.0,
            voxformat_merge=False,
            voxformat_meshmode=2,
        )
        self.assertEqual(cvar_set_args(cvars, op), [])

    def test_changed_values_are_emitted(self):
        cvars = {
            "voxformat_scale": {"type": "float", "value": "1.000000"},
            "voxformat_merge": {"type": "boolean", "value": "false"},
            "voxformat_meshmode": {"type": "int", "value": "2"},
        }
        op = types.SimpleNamespace(
            voxformat_scale=0.5,
            voxformat_merge=True,
            voxformat_meshmode=0,
        )
        self.assertEqual(
            cvar_set_args(cvars, op),
            [
                "-set", "voxformat_scale", "0.5",
                "-set", "voxformat_merge", "true",
                "-set", "voxformat_meshmode", "0",
            ],
        )

    def test_missing_operator_attr_is_skipped(self):
        cvars = {"voxformat_scale": {"type": "float", "value": "1.000000"}}
        op = types.SimpleNamespace()
        self.assertEqual(cvar_set_args(cvars, op), [])


class TestFormatCvarCliValue(unittest.TestCase):
    def test_boolean(self):
        self.assertEqual(format_cvar_cli_value("boolean", True), "true")
        self.assertEqual(format_cvar_cli_value("boolean", False), "false")


class TestCvarJsonconfigMetadata(unittest.TestCase):
    def test_rna_name_uses_title(self):
        self.assertEqual(cvar_rna_name({"title": "Voxel size"}, "voxformat_voxelsize"), "Voxel size")
        self.assertEqual(cvar_rna_name({}, "voxformat_voxelsize"), "voxformat_voxelsize")
        self.assertEqual(cvar_rna_name({"title": ""}, "voxformat_voxelsize"), "voxformat_voxelsize")

    def test_help_prefers_description(self):
        self.assertEqual(cvar_help({"description": "desc", "help": "old"}, "k"), "desc")
        self.assertEqual(cvar_help({"help": "old"}, "k"), "old")
        self.assertEqual(cvar_help({}, "k"), "k")

    def test_numeric_bounds(self):
        self.assertEqual(cvar_numeric_bounds({"type": "int", "min": 0, "max": 1024}), (0, 1024))
        self.assertEqual(cvar_numeric_bounds({"type": "float", "min": 0.0, "max": 1.0}), (0.0, 1.0))
        self.assertEqual(cvar_numeric_bounds({"type": "int"}), (None, None))

    def test_path_subtype(self):
        self.assertEqual(cvar_path_subtype({"type": "path"}), "FILE_PATH")
        self.assertEqual(cvar_path_subtype({"type": "directory"}), "DIR_PATH")
        self.assertIsNone(cvar_path_subtype({"type": "string"}))


if __name__ == "__main__":
    unittest.main()
