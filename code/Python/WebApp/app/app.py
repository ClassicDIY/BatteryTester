from flask import Flask, Response, request, url_for, render_template, jsonify, session
from multiprocessing import Queue, Value
import json
from .settings import settings
from .mqtt import run, mqttPublish
from .logger import log
from .message import Message

# --------------------------------------------------------------------------- # 
# Sharing Data Between Processes Using multiprocessing Queue
# --------------------------------------------------------------------------- # 
theQueue                  = Queue()
cellCount                 = Value('i', 0)

def create_app(test_config=None):
  # create and configure the app
  app = Flask(__name__)
  app.register_blueprint(settings, url_prefix="")
  init_app()
  
  @app.route("/")
  def render_index():
      return render_template("index.html", operation = 'Monitor', cellCount = cellCount.value)

  @app.route('/operation', methods=['POST'])
  def operation():
      current_operation = request.form.get("operation")
      mqttPublish(current_operation, "operation")
      return jsonify(status="success")

  @app.route("/listen")
  def listen():
    def respond_to_client():
      global theQueue
      while  not theQueue.empty():
        theMessage = theQueue.get()
        yield f"data: {theMessage.data}\nevent: {theMessage.topic}\n\n"
    return Response(respond_to_client(), mimetype='text/event-stream')
  return app
  
# --------------------------------------------------------------------------- # 
# MQTT On Message
# --------------------------------------------------------------------------- # 

def on_stat(client, userdata, message):

        global theQueue
        msg = message.payload.decode(encoding='UTF-8')
        log.debug("Received STAT  {} : {}".format(message.topic, msg))
        if "monitor" in message.topic:
          message = json.loads(msg)
          theMessage = Message("monitor", json.dumps(message))
          theQueue.put(theMessage)
        elif "mode" in message.topic:
          message = json.loads(msg)
          theMessage = Message("mode", json.dumps(message))
          theQueue.put(theMessage)
        elif "result" in message.topic:
          message = json.loads(msg)
          theMessage = Message("result", json.dumps(message))
          theQueue.put(theMessage)

def on_tele(client, userdata, message):

        global cellCount
        msg = message.payload.decode(encoding='UTF-8').upper()
        log.debug("Received TELE  {} : {}".format(message.topic, msg))
        if "ping" in message.topic:
            cellCount.value += 2
            log.debug("cellCount at on_tele  {} ".format(cellCount.value))

def on_cmnd(client, userdata, message):

        global theQueue
        msg = message.payload.decode(encoding='UTF-8')
        log.debug("Received CMND  {} : {}".format(message.topic, msg))
        if "operation" in message.topic:
          theMessage = Message("operation", msg)
          theQueue.put(theMessage)

def init_app():
  log.info("********************************************BatteryTester init()")
  cellCount.value = 0
  run(on_stat, on_tele, on_cmnd)

# if __name__ == "__main__":
#   log.info("BatteryTester starting up in debug mode...")
#   init_app()
#   app.run(host='0.0.0.0', debug=True, port=80)



    