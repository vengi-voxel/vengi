#!/usr/bin/env python3
# SPDX-License-Identifier: MIT

import os
import sys
import types
import tempfile
import unittest
import zipfile

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "io_vengi_voxconvert"))

from util import (
    FILTER_GLOB_MAX,
    WIN_VOXCONVERT_ASSET,
    ConversionState,
    build_filter_glob,
    cvar_help,
    cvar_numeric_bounds,
    cvar_path_subtype,
    cvar_rna_name,
    cvar_set_args,
    cvar_value_changed,
    find_voxconvert,
    find_voxconvert_in_tree,
    format_cvar_cli_value,
    gltf_export_kwargs,
    install_voxconvert_from_zip,
    int_enum_cli_value,
    int_enum_ident_from_cli,
    int_enum_rna_items,
    is_relevant_cvar,
    parse_leading_json,
    parse_progress_line,
    pick_windows_release_asset,
    run_voxconvert,
    split_cvar_keys,
    strip_ansi,
    subprocess_hidden_kwargs,
    subprocess_popen_kwargs,
    voxconvert_exe_name,
    EXPORT_PRIMARY_CVARS,
    IMPORT_PRIMARY_CVARS,
    KEEP_CVARS,
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


class TestParseLeadingJson(unittest.TestCase):
    def test_strips_log_prefix(self):
        payload = '{"voxels":[{"name":"MagicaVoxel","extensions":["vox"],"save":true}]}'
        text = "INFO: starting\n" + payload
        data = parse_leading_json(text)
        self.assertEqual(data["voxels"][0]["extensions"], ["vox"])
        self.assertTrue(data["voxels"][0]["save"])

    def test_empty_and_invalid(self):
        self.assertIsNone(parse_leading_json(""))
        self.assertIsNone(parse_leading_json("not json"))


class TestGltfExportKwargs(unittest.TestCase):
    def test_blender_36(self):
        kw = gltf_export_kwargs("/tmp/out.glb", blender_version=(3, 6, 0))
        self.assertEqual(kw["export_format"], "GLB")
        self.assertTrue(kw["export_colors"])
        self.assertNotIn("export_vertex_color", kw)

    def test_blender_41_drops_export_colors(self):
        kw = gltf_export_kwargs("/tmp/out.glb", blender_version=(4, 1, 0))
        self.assertNotIn("export_colors", kw)
        self.assertNotIn("export_vertex_color", kw)

    def test_blender_42(self):
        kw = gltf_export_kwargs("/tmp/out.glb", blender_version=(4, 2, 0))
        self.assertEqual(kw["export_vertex_color"], "ACTIVE")
        self.assertNotIn("export_colors", kw)

    def test_rna_names_win_over_version(self):
        kw = gltf_export_kwargs(
            "/tmp/out.glb",
            blender_version=(3, 6, 0),
            rna_prop_names=["export_vertex_color", "export_format"],
        )
        self.assertEqual(kw["export_vertex_color"], "ACTIVE")
        self.assertNotIn("export_colors", kw)


class TestFindVoxconvert(unittest.TestCase):
    def _touch(self, path):
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, "wb") as f:
            f.write(b"")
        os.chmod(path, 0o755)

    def test_missing_returns_empty(self):
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

    def test_cwd(self):
        with tempfile.TemporaryDirectory() as td:
            exe = os.path.join(td, "vengi-voxconvert")
            self._touch(exe)
            found = find_voxconvert(
                cwd=td,
                which=lambda *a, **k: None,
                use_well_known=False,
                system="Linux",
            )
            self.assertEqual(os.path.realpath(found), os.path.realpath(exe))

    def test_sibling_to_addon(self):
        with tempfile.TemporaryDirectory() as td:
            addon = os.path.join(td, "io_vengi_voxconvert")
            os.makedirs(addon)
            exe = os.path.join(td, "vengi-voxconvert")
            self._touch(exe)
            found = find_voxconvert(
                addon_dir=addon,
                cwd=os.path.join(td, "emptycwd"),
                which=lambda *a, **k: None,
                use_well_known=False,
                system="Linux",
            )
            self.assertEqual(os.path.realpath(found), os.path.realpath(exe))

    def test_windows_exe_and_program_files(self):
        with tempfile.TemporaryDirectory() as td:
            pf = os.path.join(td, "Program Files")
            exe = os.path.join(pf, "vengi", "vengi-voxconvert.exe")
            self._touch(exe)
            found = find_voxconvert(
                cwd=os.path.join(td, "cwd"),
                which=lambda *a, **k: None,
                program_files=pf,
                program_files_x86="",
                use_well_known=True,
                system="Windows",
            )
            self.assertEqual(os.path.realpath(found), os.path.realpath(exe))

    def test_mac_app_bundle(self):
        with tempfile.TemporaryDirectory() as td:
            inner = os.path.join(td, "vengi-voxconvert.app", "Contents", "MacOS", "vengi-voxconvert")
            self._touch(inner)
            found = find_voxconvert(
                cwd=td,
                which=lambda *a, **k: None,
                use_well_known=False,
                system="darwin",
            )
            self.assertEqual(os.path.realpath(found), os.path.realpath(inner))

    def test_exe_name(self):
        self.assertEqual(voxconvert_exe_name("Windows"), "vengi-voxconvert.exe")
        self.assertEqual(voxconvert_exe_name("Linux"), "vengi-voxconvert")


class TestWindowsZipInstall(unittest.TestCase):
    def test_picks_release_asset_not_debug(self):
        release = {
            "assets": [
                {"name": "win-voxconvert-debug.zip", "browser_download_url": "http://debug"},
                {"name": WIN_VOXCONVERT_ASSET, "browser_download_url": "http://release"},
            ]
        }
        self.assertEqual(pick_windows_release_asset(release), "http://release")
        self.assertIsNone(pick_windows_release_asset({"assets": []}))

    def test_extract_nested_exe(self):
        with tempfile.TemporaryDirectory() as td:
            zip_path = os.path.join(td, "win-voxconvert.zip")
            with zipfile.ZipFile(zip_path, "w") as zf:
                zf.writestr("voxconvert-install/voxconvert/vengi-voxconvert.exe", b"exe")
            dest = os.path.join(td, "out")
            found = install_voxconvert_from_zip(zip_path, dest, system="Windows")
            self.assertTrue(found.endswith("vengi-voxconvert.exe"))
            self.assertTrue(os.path.isfile(found))
            self.assertEqual(find_voxconvert_in_tree(dest, system="Windows"), os.path.abspath(found))

    def test_rejects_zip_slip(self):
        with tempfile.TemporaryDirectory() as td:
            zip_path = os.path.join(td, "bad.zip")
            with zipfile.ZipFile(zip_path, "w") as zf:
                zf.writestr("../evil.exe", b"x")
            with self.assertRaises(ValueError):
                install_voxconvert_from_zip(zip_path, os.path.join(td, "out"), system="Windows")


class TestCvarGrouping(unittest.TestCase):
    def test_keeps_colorreduction_skips_other_core(self):
        ok = {"type": "enum", "flags": 0, "value": "MedianCut"}
        self.assertTrue(is_relevant_cvar("core_colorreduction", ok))
        self.assertIn("core_colorreduction", KEEP_CVARS)
        self.assertFalse(is_relevant_cvar("core_loglevel", ok))
        self.assertFalse(is_relevant_cvar("app_version", ok))
        self.assertTrue(is_relevant_cvar("palette", ok))

    def test_split_primary_and_advanced(self):
        keys = ["voxformat_meshmode", "voxformat_mergequads", "palette", "voxformat_binvoxversion"]
        prim, adv = split_cvar_keys(keys, IMPORT_PRIMARY_CVARS)
        self.assertEqual(prim, ["voxformat_meshmode", "palette"])
        self.assertEqual(adv, ["voxformat_binvoxversion", "voxformat_mergequads"])

    def test_import_primary_includes_voxelsize(self):
        self.assertIn("voxformat_voxelsize", IMPORT_PRIMARY_CVARS)
        self.assertNotIn("voxformat_voxelsize", EXPORT_PRIMARY_CVARS)


class TestIntEnumOverlay(unittest.TestCase):
    def test_items_are_valid_identifiers(self):
        items = int_enum_rna_items("voxformat_meshmode")
        for ident, label, desc in items:
            self.assertTrue(ident[0].isalpha() or ident[0] == "_")
            self.assertTrue(label)
            self.assertTrue(desc)

    def test_cli_roundtrip(self):
        self.assertEqual(int_enum_ident_from_cli("voxformat_meshmode", "2"), "binary")
        self.assertEqual(int_enum_cli_value("voxformat_meshmode", "binary"), "2")
        self.assertEqual(format_cvar_cli_value("int", "binary", key="voxformat_meshmode"), "2")
        self.assertFalse(cvar_value_changed("int", "binary", "2", key="voxformat_meshmode"))
        self.assertTrue(cvar_value_changed("int", "cubic", "2", key="voxformat_meshmode"))

    def test_set_args_emits_numeric_meshmode(self):
        cvars = {"voxformat_meshmode": {"type": "int", "value": "2"}}
        op = types.SimpleNamespace(voxformat_meshmode="cubic")
        self.assertEqual(cvar_set_args(cvars, op), ["-set", "voxformat_meshmode", "0"])


class TestProgressAndProcess(unittest.TestCase):
    def test_strip_ansi(self):
        self.assertEqual(strip_ansi(""), "")
        self.assertEqual(strip_ansi(None), "")
        self.assertEqual(strip_ansi("plain"), "plain")
        self.assertEqual(strip_ansi("\033[31mERROR: boom\033[00m"), "ERROR: boom")
        self.assertEqual(strip_ansi("\033[33mwarn\033[00m and \033[31merr\033[00m"), "warn and err")

    def test_parse_progress_line(self):
        self.assertIsNone(parse_progress_line(""))
        self.assertIsNone(parse_progress_line("ERROR: failed"))
        self.assertEqual(parse_progress_line("[----------]   0% load"), (0, "load"))
        self.assertEqual(parse_progress_line("[##------]  25% save"), (25, "save"))
        self.assertEqual(parse_progress_line("[##########] 100% load"), (100, "load"))
        self.assertEqual(parse_progress_line("\r[####----]  42% mesh"), (42, "mesh"))
        self.assertEqual(
            parse_progress_line("[----------]   0% load\n[##------]  25% load"),
            (25, "load"),
        )

    def test_subprocess_kwargs(self):
        import subprocess as sp
        import util as u

        old = u.sys.platform
        try:
            u.sys.platform = "linux"
            self.assertEqual(subprocess_hidden_kwargs(), {})
            self.assertTrue(subprocess_popen_kwargs().get("start_new_session"))
            u.sys.platform = "win32"
            hidden = subprocess_hidden_kwargs()
            self.assertEqual(hidden.get("creationflags"), getattr(sp, "CREATE_NO_WINDOW", 0x08000000))
            self.assertNotIn("start_new_session", subprocess_popen_kwargs())
        finally:
            u.sys.platform = old

    def _write_script(self, td, body):
        path = os.path.join(td, "fake_voxconvert.py")
        with open(path, "w") as f:
            f.write(body)
        return path

    def test_run_voxconvert_progress_and_success(self):
        script = (
            "import sys\n"
            "if '--progress' not in sys.argv:\n"
            "    sys.stderr.write('missing --progress\\n')\n"
            "    sys.exit(1)\n"
            "sys.stderr.write('[----------]   0% load\\n')\n"
            "sys.stderr.flush()\n"
            "sys.stderr.write('[##------]  25% load\\n')\n"
            "sys.stderr.flush()\n"
            "sys.stderr.write('[##########] 100% load\\n')\n"
            "sys.stderr.flush()\n"
        )
        with tempfile.TemporaryDirectory() as td:
            path = self._write_script(td, script)
            state = ConversionState()
            run_voxconvert(sys.executable, [path], state)
            self.assertTrue(state.done)
            self.assertTrue(state.success)
            self.assertEqual(state.progress, 100)
            self.assertEqual(state.progress_text, "load")

    def test_run_voxconvert_strips_ansi_errors(self):
        script = (
            "import sys\n"
            "sys.stderr.write('\\033[31mERROR: boom\\033[00m\\n')\n"
            "sys.exit(1)\n"
        )
        with tempfile.TemporaryDirectory() as td:
            path = self._write_script(td, script)
            state = ConversionState()
            run_voxconvert(sys.executable, [path], state)
            self.assertTrue(state.done)
            self.assertFalse(state.success)
            self.assertNotIn("\033", state.error)
            self.assertIn("ERROR: boom", state.error)

    def test_cancel_kills_subprocess(self):
        import threading
        import time

        script = (
            "import sys, time\n"
            "sys.stderr.write('[----------]   0% wait\\n')\n"
            "sys.stderr.flush()\n"
            "time.sleep(30)\n"
        )
        with tempfile.TemporaryDirectory() as td:
            path = self._write_script(td, script)
            state = ConversionState()
            t = threading.Thread(target=run_voxconvert, args=(sys.executable, [path], state))
            t.daemon = True
            t.start()
            deadline = time.time() + 5
            while time.time() < deadline and state.proc is None:
                time.sleep(0.05)
            self.assertIsNotNone(state.proc)
            pid = state.proc.pid
            state.request_cancel()
            t.join(timeout=10)
            self.assertTrue(t.is_alive() is False)
            self.assertTrue(state.done)
            self.assertTrue(state.cancelled)
            self.assertFalse(state.success)
            try:
                os.kill(pid, 0)
                still_alive = True
            except OSError:
                still_alive = False
            self.assertFalse(still_alive)

    def test_late_cancel_keeps_success(self):
        state = ConversionState()
        state.done = True
        state.success = True
        state.request_cancel()
        self.assertFalse(state.cancelled)
        self.assertTrue(state.success)


if __name__ == "__main__":
    unittest.main()
