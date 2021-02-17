from flask.helpers import url_for
from gevent import monkey; monkey.patch_all()
from flask import Flask, Response, request, url_for, render_template, jsonify
from gevent.pywsgi import WSGIServer
import json
import sys
import queue
from settings import settings
from mqtt import run, mqttPublish
from logger import log

# --------------------------------------------------------------------------- # 
# Counters and status variables
# --------------------------------------------------------------------------- # 
theQueue                  = queue.Queue()
cellCount                   = 0
current_operation           = "Monitor"

app = Flask(__name__)
app.register_blueprint(settings, url_prefix="")

@app.route("/")
def render_index():
   return render_template("index.html", operation = current_operation, cellCount = cellCount)

@app.route('/operation', methods=['POST'])
def operation():
  global operation
  current_operation = request.form.get("operation")
  log.debug(current_operation)
  mqttPublish(current_operation, "operation")
  return jsonify(status="success")

@app.route("/listen")
def listen():

  def respond_to_client():
    global theQueue
    while  not theQueue.empty():
      theMessage = theQueue.get()
      log.debug(theMessage)
      yield f"id: 1\ndata: {theMessage}\nevent: online\n\n"
  return Response(respond_to_client(), mimetype='text/event-stream')
  
# --------------------------------------------------------------------------- # 
# MQTT On Message
# --------------------------------------------------------------------------- # 

def on_stat(client, userdata, message):

        global theQueue
        msg = message.payload.decode(encoding='UTF-8').upper()
        log.debug("Received STAT  {} : {}".format(message.topic, msg))
        message = json.loads(message.payload.decode(encoding='UTF-8'))
        theMessage = json.dumps(message)
        theQueue.put(theMessage)

def on_tele(client, userdata, message):

        global theQueue, cellCount
        msg = message.payload.decode(encoding='UTF-8').upper()
        log.debug("Received TELE  {} : {}".format(message.topic, msg))
        if "ping" in message.topic:
            cellCount += 2
            log.debug("cellCount  {} ".format(cellCount))


def on_cmnd(client, userdata, message):

        global theQueue
        msg = message.payload.decode(encoding='UTF-8').upper()
        log.debug("Received CMND  {} : {}".format(message.topic, msg))
        if "operation" in message.topic:
            log.debug("Todo Operation")


if __name__ == "__main__":
  run(on_stat, on_tele, on_cmnd)
  app.run(port=80)
  # http_server = WSGIServer(("localhost", 8008), app)
  # http_server.serve_forever()








