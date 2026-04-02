#!/usr/bin/env python3

from flask import Flask, request, redirect, send_file, Response, jsonify
from influxdb import InfluxDBClient
import atexit
import threading
import queue
import configparser
import logging
import uuid
import os
import json
import io
import re


def extract_description(lua_content):
    """Extract the return value of the description() function from lua content."""
    if isinstance(lua_content, bytes):
        lua_content = lua_content.decode('utf-8', errors='replace')
    match = re.search(
        r"function\s+description\s*\(\s*\)\s*\n\s*return\s+['\"](.+?)['\"]\s*\n",
        lua_content,
    )
    if match:
        return match.group(1)
    return ''


def extract_luadoc(lua_content, directive):
    """Extract a @directive value from LuaDoc comments."""
    if isinstance(lua_content, bytes):
        lua_content = lua_content.decode('utf-8', errors='replace')
    match = re.search(r'--\s*@' + re.escape(directive) + r'\s+(.+)', lua_content)
    if match:
        return match.group(1).strip()
    return ''


def derive_name(filename):
    """Derive a human-readable name from the lua filename stem."""
    stem = os.path.splitext(os.path.basename(filename))[0]
    parts = stem.split('_')
    if len(parts) > 1 and parts[0] in ('tree', 'brush'):
        parts = parts[1:]
    words = []
    for part in parts:
        sub = re.sub(r'([a-z])([A-Z])', r'\1 \2', part)
        words.extend(sub.split())
    return ' '.join(w.capitalize() for w in words)


def scan_scripts(config):
    base_dir = config.get('scripts', 'directory', fallback='scripts')
    catalog = {}
    for script_type, subdir in [('generator', 'script'), ('brush', 'brush')]:
        dir_path = os.path.join(base_dir, subdir)
        if not os.path.isdir(dir_path):
            logging.warning("Script directory not found: %s", dir_path)
            continue
        for filename in os.listdir(dir_path):
            if not filename.endswith('.lua'):
                continue
            lua_path = os.path.join(dir_path, filename)
            json_path = os.path.join(dir_path, filename[:-4] + '.json')
            meta = {}
            if os.path.isfile(json_path):
                with open(json_path, 'r') as f:
                    meta = json.load(f)
            with open(lua_path, 'rb') as f:
                content = f.read()
            name = filename[:-4]
            if meta:
                display_name = meta.get('name', name)
                description = meta.get('description', '')
                version = meta.get('version', '')
                author = meta.get('author', '')
            else:
                display_name = derive_name(filename)
                description = extract_description(content)
                version = extract_luadoc(content, 'version') or '1.0.0'
                author = extract_luadoc(content, 'author') or 'vengi'
            catalog[filename] = {
                'type': script_type,
                'name': display_name,
                'description': description,
                'version': version,
                'author': author,
                'filename': filename,
                'content': content,
            }
    return catalog


def worker(db, q):
    while True:
        item = q.get()
        metric_name = item[0]
        value = item[1]
        uuid = item[2]
        tags = item[3]
        metric_json = [
            {
                "measurement": metric_name,
                "tags": tags,
                "fields": {
                    "value": value,
                    "uuid": uuid
                }
            }
        ]
        db.write_points(metric_json)
        q.task_done()


def create_app(config=None):
    app = Flask(__name__)
    q = queue.Queue()

    if config is None:
        config = configparser.ConfigParser()
        config.read('config.ini')

        logging.basicConfig(level=logging.INFO)
        host = config.get('influx', 'host')
        port = int(config.get('influx', 'port'))
        database = config.get('influx', 'database')
        username = config.get('influx', 'username')
        password = config.get('influx', 'password')
        db = InfluxDBClient(host, port, username, password, database)
        def shutdown():
            db.close()
        atexit.register(shutdown)
        threading.Thread(target=worker, name='vengi-api-worker', daemon=True, args=(db, q)).start()

    script_catalog = scan_scripts(config)
    logging.info("Loaded %d lua scripts into memory", len(script_catalog))

    reload_interval = config.getint('scripts', 'reload_interval', fallback=300)

    def reload_scripts():
        nonlocal script_catalog
        script_catalog = scan_scripts(config)
        logging.info("Reloaded %d lua scripts into memory", len(script_catalog))
        if reload_interval > 0:
            t = threading.Timer(reload_interval, reload_scripts)
            t.daemon = True
            t.start()

    if reload_interval > 0:
        t = threading.Timer(reload_interval, reload_scripts)
        t.daemon = True
        t.start()
        logging.info("Script reload scheduled every %d seconds", reload_interval)

    def insertMetric(metric_name, value, uuid, tags):
        q.put((metric_name, value, uuid, tags))

    @app.route('/')
    def home():
        return redirect("http://vengi-voxel.github.io/vengi/", code=302)

    @app.route('/browser-data')
    def browserData():
        return send_file("voxbrowser-sources.json", as_attachment=True, download_name="voxbrowser-sources.json")

    @app.route('/crashlog', methods = ['POST'])
    def crashlog():
        if request.data is None:
            return Response("Missing crashlog data", status_code = 400)
        fileuuid = uuid.uuid4()
        fileuuidstr = str(fileuuid)

        userAgent = request.headers['User-Agent']
        operatingSystem = request.headers['X-OperatingSystem']
        operatingSystemVersion = request.headers['X-OperatingSystemVersion']
        if operatingSystem is None:
            operatingSystem = "Unknown"
        if operatingSystemVersion is None:
            operatingSystemVersion = "0.0"

        # this directory must exists in the current working dir and must be writable
        with open(f"crashlogs/{fileuuidstr}.txt", "w") as fo:
            fo.write("----------------------------\n")
            fo.write(f"Application: {userAgent}\n")
            fo.write(f"OS: {operatingSystem}/{operatingSystemVersion}\n")
            fo.write(request.data.decode("utf-8"))

        tags = {}
        if '/' in userAgent:
            application, version = userAgent.split('/', 2)
        else:
            application = userAgent
            version = '0.0.0'
        tags['application'] = application
        tags['version'] = version
        tags['os'] = operatingSystem
        tags['osversion'] = operatingSystemVersion
        insertMetric("crashdump", "1", fileuuidstr, tags)

        return Response(status = 204)

    @app.route('/metric', methods = ['POST'])
    def metric():
        data = request.json
        if data is None:
            return Response("Missing data", status_code = 400)

        tags = data['tags']
        if tags is None:
            return Response("Missing tags", status_code = 400)

        name = data['name']
        if name is None:
            return Response("Missing name", status_code = 400)

        value = data['value']
        if value is None:
            return Response("Missing value", status_code = 400)

        uuid = data['uuid']
        if uuid is None:
            return Response("Missing uuid", status_code = 400)

        userAgent = request.headers['User-Agent']
        if userAgent is None:
            return Response("Missing User-Agent", status_code = 400)

        if ' ' in userAgent:
            userAgent = userAgent[0 : userAgent.index(' ')]

        if '/' in userAgent:
            application, version = userAgent.split('/', 2)
        else:
            application = userAgent
            version = '0.0.0'
        tags['application'] = application
        tags['version'] = version

        insertMetric(name, value, uuid, tags)

        app.logger.debug('Got a metric - with {} in queue'.format(q.qsize()))
        return Response(status = 204)

    @app.route('/scripts')
    def scripts():
        result = []
        for entry in script_catalog.values():
            result.append({
                'type': entry['type'],
                'name': entry['name'],
                'description': entry['description'],
                'version': entry['version'],
                'author': entry['author'],
                'filename': entry['filename'],
            })
        return jsonify(result)

    @app.route('/scripts/download/<filename>')
    def scripts_download(filename):
        entry = script_catalog.get(filename)
        if entry is None:
            return Response("Not found", status=404)
        insertMetric("script_download", "1", str(uuid.uuid4()), {
            'script_name': entry['name'],
            'script_type': entry['type'],
        })
        return send_file(
            io.BytesIO(entry['content']),
            as_attachment=True,
            download_name=filename,
            mimetype='text/x-lua',
        )

    return app

app = create_app()

if __name__ == "__main__":
    app.run(debug = True, port= 8000)
