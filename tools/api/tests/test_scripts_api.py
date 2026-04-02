#!/usr/bin/env python3

import configparser
import json
import os
import sys
import tempfile

import pytest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))
from vengi import create_app, scan_scripts


def _make_config(scripts_dir):
    config = configparser.ConfigParser()
    config.read_dict({'scripts': {'directory': scripts_dir}})
    return config


@pytest.fixture()
def script_dirs():
    with tempfile.TemporaryDirectory() as base:
        script_dir = os.path.join(base, 'script')
        brush_dir = os.path.join(base, 'brush')
        os.makedirs(script_dir)
        os.makedirs(brush_dir)

        # generator script with metadata
        with open(os.path.join(script_dir, 'noise.lua'), 'w') as f:
            f.write('-- noise generator\nfunction main() end\n')
        with open(os.path.join(script_dir, 'noise.json'), 'w') as f:
            json.dump({
                'name': 'Noise Generator',
                'description': 'Generates noise patterns',
                'version': '1.2.0',
                'author': 'TestAuthor',
            }, f)

        # generator script without metadata
        with open(os.path.join(script_dir, 'gradient.lua'), 'w') as f:
            f.write('-- gradient\nfunction main() end\n')

        # generator script with @author and @version LuaDoc directives but no json
        with open(os.path.join(script_dir, 'tree_oak.lua'), 'w') as f:
            f.write('-- @author OakAuthor\n-- @version 3.2.1\nfunction description()\n\treturn \'Creates an oak tree\'\nend\nfunction main() end\n')

        # brush script with metadata
        with open(os.path.join(brush_dir, 'path.lua'), 'w') as f:
            f.write('-- path brush\nfunction generate() end\n')
        with open(os.path.join(brush_dir, 'path.json'), 'w') as f:
            json.dump({
                'name': 'Path Brush',
                'description': 'Draws a path',
                'version': '0.1.0',
                'author': 'BrushAuthor',
            }, f)

        yield base


@pytest.fixture()
def client(script_dirs):
    app = create_app(config=_make_config(script_dirs))
    app.config['TESTING'] = True
    with app.test_client() as client:
        yield client


class TestScriptsEndpoint:
    def test_returns_json_list(self, client):
        resp = client.get('/scripts')
        assert resp.status_code == 200
        assert resp.content_type == 'application/json'
        data = resp.get_json()
        assert isinstance(data, list)

    def test_contains_all_scripts(self, client):
        data = client.get('/scripts').get_json()
        filenames = {e['filename'] for e in data}
        assert filenames == {'noise.lua', 'gradient.lua', 'tree_oak.lua', 'path.lua'}

    def test_generator_type(self, client):
        data = client.get('/scripts').get_json()
        by_file = {e['filename']: e for e in data}
        assert by_file['noise.lua']['type'] == 'generator'
        assert by_file['gradient.lua']['type'] == 'generator'

    def test_brush_type(self, client):
        data = client.get('/scripts').get_json()
        by_file = {e['filename']: e for e in data}
        assert by_file['path.lua']['type'] == 'brush'

    def test_metadata_from_json(self, client):
        data = client.get('/scripts').get_json()
        by_file = {e['filename']: e for e in data}
        noise = by_file['noise.lua']
        assert noise['name'] == 'Noise Generator'
        assert noise['description'] == 'Generates noise patterns'
        assert noise['version'] == '1.2.0'
        assert noise['author'] == 'TestAuthor'

    def test_metadata_defaults_without_json(self, client):
        data = client.get('/scripts').get_json()
        by_file = {e['filename']: e for e in data}
        gradient = by_file['gradient.lua']
        assert gradient['name'] == 'Gradient'
        assert gradient['description'] == ''
        assert gradient['version'] == '1.0.0'
        assert gradient['author'] == 'vengi'

    def test_metadata_from_luadoc(self, client):
        data = client.get('/scripts').get_json()
        by_file = {e['filename']: e for e in data}
        oak = by_file['tree_oak.lua']
        assert oak['name'] == 'Oak'
        assert oak['description'] == 'Creates an oak tree'
        assert oak['version'] == '3.2.1'
        assert oak['author'] == 'OakAuthor'

    def test_no_content_field_exposed(self, client):
        data = client.get('/scripts').get_json()
        for entry in data:
            assert 'content' not in entry


class TestScriptsDownloadEndpoint:
    def test_download_valid_script(self, client):
        resp = client.get('/scripts/download/noise.lua')
        assert resp.status_code == 200
        assert b'noise generator' in resp.data
        assert resp.headers['Content-Type'].startswith('text/x-lua')

    def test_download_returns_attachment(self, client):
        resp = client.get('/scripts/download/path.lua')
        assert resp.status_code == 200
        disposition = resp.headers.get('Content-Disposition', '')
        assert 'attachment' in disposition
        assert 'path.lua' in disposition

    def test_download_unknown_file_returns_404(self, client):
        resp = client.get('/scripts/download/nonexistent.lua')
        assert resp.status_code == 404

    def test_download_path_traversal_returns_404(self, client):
        resp = client.get('/scripts/download/../../../etc/passwd')
        assert resp.status_code == 404

    def test_download_non_lua_returns_404(self, client):
        resp = client.get('/scripts/download/noise.json')
        assert resp.status_code == 404


class TestScanScripts:
    def test_empty_directory(self):
        with tempfile.TemporaryDirectory() as base:
            os.makedirs(os.path.join(base, 'script'))
            os.makedirs(os.path.join(base, 'brush'))
            catalog = scan_scripts(_make_config(base))
            assert catalog == {}

    def test_missing_directories(self):
        with tempfile.TemporaryDirectory() as base:
            catalog = scan_scripts(_make_config(base))
            assert catalog == {}

    def test_non_lua_files_ignored(self):
        with tempfile.TemporaryDirectory() as base:
            script_dir = os.path.join(base, 'script')
            os.makedirs(script_dir)
            with open(os.path.join(script_dir, 'readme.txt'), 'w') as f:
                f.write('not a lua script')
            catalog = scan_scripts(_make_config(base))
            assert catalog == {}

    def test_file_content_in_memory(self, script_dirs):
        catalog = scan_scripts(_make_config(script_dirs))
        assert b'noise generator' in catalog['noise.lua']['content']
        assert b'path brush' in catalog['path.lua']['content']
