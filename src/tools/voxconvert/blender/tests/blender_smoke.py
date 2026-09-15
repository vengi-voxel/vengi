#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Blender smoke test for the vengi-voxconvert addon helpers.

Run with:
  blender --background --factory-startup --python-exit-code 1 --python blender_smoke.py -- /path/to/vengi-voxconvert

Also collected by unittest discover. Tests that need bpy skip when it is missing.
"""

import os
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "io_vengi_voxconvert"))

from util import (  # noqa: E402
    GLTF_ADDON_MODULES,
    ensure_gltf_addon,
    find_voxconvert,
    gltf_export_kwargs,
    gltf_export_rna_prop_names,
    parse_leading_json,
    run_command,
)


def _cli_voxconvert():
    if "--" in sys.argv:
        rest = sys.argv[sys.argv.index("--") + 1 :]
        if rest:
            return rest[0]
    env = os.environ.get("VOXCONVERT_BIN", "")
    if env:
        return env
    for a in sys.argv[1:]:
        if a.endswith("vengi-voxconvert") or a.endswith("vengi-voxconvert.exe"):
            return a
    return ""


class TestMissingExe(unittest.TestCase):
    def test_empty_search_returns_empty(self):
        import tempfile

        with tempfile.TemporaryDirectory() as td:
            self.assertEqual(
                find_voxconvert(
                    addon_dir=td,
                    cwd=td,
                    path_env=td,
                    which=lambda *a, **k: None,
                    use_well_known=False,
                    system="Linux",
                ),
                "",
            )


class TestPrintFormatsParse(unittest.TestCase):
    def test_parse_real_binary_if_given(self):
        exe = _cli_voxconvert()
        if not exe or not os.path.isfile(exe):
            exe = find_voxconvert(use_well_known=True)
        if not exe or not os.path.isfile(exe):
            self.skipTest("vengi-voxconvert not available")
        out, err, _rc = run_command(exe, ["--print-formats"], timeout=30)
        data = parse_leading_json(out or err)
        self.assertIsNotNone(data, "failed to parse --print-formats JSON")
        self.assertIn("voxels", data)
        self.assertTrue(data["voxels"])
        first = data["voxels"][0]
        self.assertIn("extensions", first)
        self.assertIn("save", first)


class TestGltfOperatorKwargs(unittest.TestCase):
    def test_live_rna(self):
        try:
            import addon_utils
            import bpy
        except ImportError:
            self.skipTest("bpy not available (run under blender --background --python)")
        ok, msg = ensure_gltf_addon(addon_utils, bpy)
        self.assertTrue(ok, msg)
        names = gltf_export_rna_prop_names(bpy)
        self.assertTrue(names, "export_scene.gltf RNA is empty after enabling %s" % (GLTF_ADDON_MODULES,))
        kw = gltf_export_kwargs("/tmp/out.glb", blender_version=bpy.app.version, rna_prop_names=names)
        self.assertEqual(kw["export_format"], "GLB")
        for key in kw:
            if key == "filepath":
                continue
            self.assertIn(key, names, "glTF kwarg %s is not a property of this Blender" % key)


if __name__ == "__main__":
    argv = [sys.argv[0]]
    result = unittest.main(argv=argv, exit=False)
    sys.exit(0 if result.result.wasSuccessful() else 1)
