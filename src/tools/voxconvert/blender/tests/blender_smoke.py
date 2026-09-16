#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Optional live checks.

  blender --background --factory-startup --python-exit-code 1 --python blender_smoke.py -- /path/to/vengi-voxconvert
"""

import os
import sys
import tempfile
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "io_vengi_voxconvert"))

from util import (  # noqa: E402
    GLTF_ADDON_MODULES,
    REQUIRED_VOXCONVERT_VERSION,
    ensure_gltf_addon,
    gltf_export_kwargs,
    gltf_export_rna_prop_names,
    parse_leading_json,
    parse_print_formats,
    parse_voxconvert_version,
    run_command,
    voxconvert_version_atleast,
)


def _exe():
    if "--" in sys.argv:
        rest = sys.argv[sys.argv.index("--") + 1 :]
        if rest:
            return rest[0]
    return os.environ.get("VOXCONVERT_BIN", "")


class TestLive(unittest.TestCase):
    def test_print_formats_and_jsonconfig(self):
        exe = _exe()
        self.assertTrue(exe and os.path.isfile(exe), "vengi-voxconvert not given")
        out, err, _rc = run_command(exe, ["--version"])
        ver = parse_voxconvert_version((out or "") + "\n" + (err or ""))
        self.assertTrue(
            voxconvert_version_atleast(ver),
            "need vengi-voxconvert %s or newer, got %s" % (REQUIRED_VOXCONVERT_VERSION, ver),
        )
        formats = parse_leading_json(run_command(exe, ["--print-formats"], timeout=30)[0])
        self.assertTrue(formats and formats.get("voxels"))
        self.assertTrue(parse_print_formats(formats)["volume"])
        cfg = parse_leading_json(run_command(exe, ["--jsonconfig"], timeout=30)[0])
        mesh = (cfg or {}).get("voxformat_meshmode") or {}
        self.assertTrue(mesh.get("save"))
        self.assertTrue(mesh.get("primary"))
        self.assertIn("Cubes", mesh.get("value_titles") or [])
        quads = (cfg or {}).get("voxformat_quads") or {}
        self.assertIn("Wavefront Object", quads.get("formats") or [])
        self.assertNotIn("GL Transmission Format", quads.get("formats") or [])

    def test_gltf_rna(self):
        try:
            import addon_utils
            import bpy
        except ImportError:
            self.skipTest("bpy not available")
        ok, msg = ensure_gltf_addon(addon_utils, bpy)
        self.assertTrue(ok, msg)
        names = gltf_export_rna_prop_names(bpy)
        self.assertTrue(names, "empty glTF RNA after enabling %s" % (GLTF_ADDON_MODULES,))
        kw = gltf_export_kwargs(
            os.path.join(tempfile.gettempdir(), "out.glb"),
            blender_version=bpy.app.version,
            rna_prop_names=names,
        )
        for key in kw:
            if key != "filepath":
                self.assertIn(key, names)


if __name__ == "__main__":
    result = unittest.main(argv=[sys.argv[0]], exit=False)
    sys.exit(0 if result.result.wasSuccessful() else 1)
