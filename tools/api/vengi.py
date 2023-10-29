#!/usr/bin/env python3

from flask import Flask, request, Response
import sqlite3
import atexit
import threading
import queue

q = queue.Queue()

DATABASE = 'metrics.db'

db = None

def worker():
    while True:
        item = q.get()
        metric_name = item[0]
        value = item[1]
        tags = item[2]
        cursor = db.cursor()
        cursor.execute("INSERT INTO metrics (timestamp, metric_name, value) VALUES (unixepoch(), ?, ?)", (metric_name, value))
        db.commit()
        metric_id = cursor.lastrowid
        tag_items = []
        for tag in tags:
            tag_items.append((metric_id, tag, tags[tag]))
        cursor.executemany("INSERT INTO tags (metric_id, tag_key, tag_value) VALUES (?, ?, ?)", tag_items)
        db.commit()
        q.task_done()

app = Flask(__name__)

def insertMetric(metric_name, value, tags):
    q.put((metric_name, value, tags))

@app.route('/')
def home():
    return "<p>Vengi Voxel Tools API</p>"

@app.route('/metric', methods = ['POST'])
def metric():
    data = request.json
    if data is None:
        return Response(status_code = 400)

    tags = data['tags']
    if tags is None:
        return Response(status_code = 400)

    name = data['name']
    value = data['value']

    userAgent = request.headers['User-Agent']
    if ' ' in userAgent:
        userAgent = userAgent[0 : userAgent.index(' ')]

    if '/' in userAgent:
        application, version = userAgent.split('/', 2)
    else:
        application = userAgent
        version = '0.0.0'
    tags['application'] = application
    tags['version'] = version

    insertMetric(name, value, tags)

    return Response(status = 204)

def initDB():
    print("Initializing database tables\n")
    cursor = db.cursor()
    # Create the 'metrics' table
    cursor.execute('''
        CREATE TABLE IF NOT EXISTS metrics (
            id INTEGER PRIMARY KEY,
            timestamp INTEGER,
            metric_name TEXT,
            value REAL
        )
    ''')
    # Create the 'tags' table
    cursor.execute('''
        CREATE TABLE IF NOT EXISTS tags (
            metric_id INTEGER,
            tag_key TEXT,
            tag_value TEXT,
            FOREIGN KEY (metric_id) REFERENCES metrics(id)
        )
    ''')
    cursor.close()

if __name__ == "__main__":
    db = sqlite3.connect(DATABASE, check_same_thread = False)
    def shutdown():
        db.close()
    atexit.register(shutdown)

    initDB()

    threading.Thread(target=worker, daemon=True).start()

    app.run()
    q.join()
