from flask.helpers import url_for
from gevent import monkey; monkey.patch_all()
from flask import Flask, Response, redirect, url_for, render_template, stream_with_context
from gevent.pywsgi import WSGIServer
import json
import sys
from admin import admin
from mqtt import run
from logger import log

# --------------------------------------------------------------------------- # 
# Counters and status variables
# --------------------------------------------------------------------------- # 
theMessage                  = "{nothing}"

app = Flask(__name__)
app.register_blueprint(admin, url_prefix="")


@app.route("/")
def render_index():
  return render_template("index.html")


# @app.route("/<operation>")
# def operation(operation):
#   return render_template("index.html", operation = f"{operation}")

# @app.route("/Test")
# def test():
#   return redirect(url_for("operation", operation="Charge"))

@app.route("/Charge")
def Charge():
  return render_template("index.html", operation = "Charge")

@app.route("/Discharge")
def Discharge():
  return render_template("index.html", operation = "Discharge")

@app.route("/Monitor")
def Monitor():
  return render_template("index.html", operation = "Monitor")
    

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
def on_message(client, userdata, message):
  
        #print("Received message '" + str(message.payload) + "' on topic '"
        #+ message.topic + "' with QoS " + str(message.qos))

        global theMessage

        mqttConnected = True #got a message so we must be up again...
        mqttErrorCount = 0

        msg = message.payload.decode(encoding='UTF-8').upper()
        log.debug("Received MQTT message {}".format(msg))

        #if we get a WAKE or INFO, reset the counters, re-puplish the INFO and stop snoozing.
        if msg == "{\"STOP\"}":
            doStop = True
        else: #JSON messages
            message = json.loads(message.payload.decode(encoding='UTF-8'))
            theMessage = json.dumps(message)

if __name__ == "__main__":
  run(on_message)
  app.run(port=80)
  # http_server = WSGIServer(("localhost", 8008), app)
  # http_server.serve_forever()








