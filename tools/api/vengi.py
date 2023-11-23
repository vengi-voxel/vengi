#!/usr/bin/env python3

from flask import Flask, request, Response
from influxdb import InfluxDBClient
import atexit
import threading
import queue
import configparser

config = configparser.ConfigParser()
config.read('config.ini')

q = queue.Queue()

DATABASE = 'metrics.db'

db = None

def worker():
    while True:
        item = q.get()
        metric_name = item[0]
        value = item[1]
        tags = item[2]
        metric_json = [
            {
                "measurement": metric_name,
                "tags": tags,
                "fields": {
                    "value": value
                }
            }
        ]
        db.write_points(metric_json)
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

    insertMetric(name, value, tags)

    return Response(status = 204)

if __name__ == "__main__":
    host = config.get('influx', 'host')
    port = int(config.get('influx', 'port'))
    database = config.get('influx', 'database')
    username = config.get('influx', 'username')
    password = config.get('influx', 'password')

    db = InfluxDBClient(host, port, username, password, database)
    def shutdown():
        db.close()
    atexit.register(shutdown)

    threading.Thread(target=worker, daemon=True).start()

    app.run(port=8000)
    q.join()
