from flask.helpers import url_for
from gevent import monkey; monkey.patch_all()
from flask import Flask, Response, request, url_for, render_template, jsonify
from gevent.pywsgi import WSGIServer
import json
import sys
from settings import settings
from mqtt import run, mqttPublish
from logger import log

# --------------------------------------------------------------------------- # 
# Counters and status variables
# --------------------------------------------------------------------------- # 
theMessage                  = "{nothing}"
cellCount                   = 0

app = Flask(__name__)
app.register_blueprint(settings, url_prefix="")

@app.route("/")
def render_index():
   return render_template("index.html", cellCount = cellCount)

@app.route('/operation', methods=['POST'])
def operation():
  operation = request.form.get("operation")
  log.debug(operation)
  mqttPublish(operation, "operation")
  return jsonify(status="success")

@app.route("/listen")
def listen():

  def respond_to_client():
    global theMessage
    log.debug(theMessage)
    yield f"id: 1\ndata: {theMessage}\nevent: online\n\n"
  return Response(respond_to_client(), mimetype='text/event-stream')
  
# --------------------------------------------------------------------------- # 
# MQTT On Message
# --------------------------------------------------------------------------- # 

def on_stat(client, userdata, message):

        global theMessage
        msg = message.payload.decode(encoding='UTF-8').upper()
        log.debug("Received STAT  {} : {}".format(message.topic, msg))
        message = json.loads(message.payload.decode(encoding='UTF-8'))
        theMessage = json.dumps(message)

def on_tele(client, userdata, message):

        global theMessage, cellCount
        msg = message.payload.decode(encoding='UTF-8').upper()
        log.debug("Received TELE  {} : {}".format(message.topic, msg))
        if "ping" in message.topic:
          try:
            cellCount += 2
            log.debug("cellCount  {} ".format(cellCount))
          except Exception as e: #LWT
            log.debug("BatteryTester Online")

def on_cmnd(client, userdata, message):

        global theMessage
        msg = message.payload.decode(encoding='UTF-8').upper()
        log.debug("Received CMND  {} : {}".format(message.topic, msg))

if __name__ == "__main__":
  run(on_stat, on_tele, on_cmnd)
  app.run(port=80)
  # http_server = WSGIServer(("localhost", 8008), app)
  # http_server.serve_forever()








