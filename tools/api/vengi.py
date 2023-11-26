#!/usr/bin/env python3

from flask import Flask, request, redirect, Response
from influxdb import InfluxDBClient
import atexit
import threading
import queue
import configparser
import logging

config = configparser.ConfigParser()
config.read('config.ini')
app = Flask(__name__)
q = queue.Queue()
db = None

def worker(db, q):
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

def init():
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

init()

def insertMetric(metric_name, value, tags):
    q.put((metric_name, value, tags))

@app.route('/')
def home():
    return redirect("http://vengi-voxel.github.io/vengi/", code=302)

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

    app.logger.debug('Got a metric - with {} in queue'.format(q.qsize()))
    return Response(status = 204)

if __name__ == "__main__":
    app.run(debug = True, port= 8000)
    q.join()
