from flask.helpers import url_for
from gevent import monkey; monkey.patch_all()
from flask import Flask, Response, redirect, url_for, render_template, stream_with_context
from gevent.pywsgi import WSGIServer
from paho.mqtt import client as mqttclient
import logging
import os
import sys
import json
import time
from random import randint, seed

# --------------------------------------------------------------------------- # 
# Counters and status variables
# --------------------------------------------------------------------------- # 
infoPublished               = False
mqttConnected               = False
mqttClient                  = None
theMessage = "5645646454"

# --------------------------------------------------------------------------- # 
# configure the logging
# --------------------------------------------------------------------------- # 
log = logging.getLogger('battery_tester')
if not log.handlers:
    handler = logging.StreamHandler(sys.stdout)
    formatter = logging.Formatter('%(asctime)s:%(levelname)s:%(name)s:%(message)s')
    handler.setFormatter(formatter)
    log.addHandler(handler) 
    log.setLevel(os.environ.get("LOGLEVEL", "DEBUG"))


argumentValues = { 'classicName':"1", 'mqttHost':"192.168.86.25", 'mqttPort':"1883", 'mqttRoot':"BatteryTester", 'mqttUser':"Argon", 'mqttPassword':"volvo4"}

# --------------------------------------------------------------------------- # 
# MQTT On Connect function
# --------------------------------------------------------------------------- # 
def on_connect(client, userdata, flags, rc):
    global mqttConnected, mqttErrorCount, mqttClient
    if rc==0:
        log.debug("MQTT connected OK Returned code={}".format(rc))
        #subscribe to the commands
        try:
            topic = "{}/stat/{}/#".format(argumentValues['mqttRoot'], argumentValues['classicName'])
            client.subscribe(topic)
            log.debug("Subscribed to {}".format(topic))
            
            #publish that we are Online
            # will_topic = "{}{}/tele/LWT".format(argumentValues['mqttRoot'], argumentValues['classicName')
            # mqttClient.publish(will_topic, "Online",  qos=0, retain=False)
        except Exception as e:
            log.error("MQTT Subscribe failed")
            log.exception(e, exc_info=True)

        mqttConnected = True
        mqttErrorCount = 0
    else:
        mqttConnected = False
        log.error("MQTT Bad connection Returned code={}".format(rc))

# --------------------------------------------------------------------------- # 
# MQTT On Disconnect
# --------------------------------------------------------------------------- # 
def on_disconnect(client, userdata, rc):
    global mqttConnected, mqttClient
    mqttConnected = False
    #if disconnetion was unexpectred (not a result of a disconnect request) then log it.
    if rc!=mqttclient.MQTT_ERR_SUCCESS:
        log.debug("on_disconnect: Disconnected. ReasonCode={}".format(rc))

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
        # log.debug("Received MQTT message {}".format(msg))

        #if we get a WAKE or INFO, reset the counters, re-puplish the INFO and stop snoozing.
        if msg == "{\"STOP\"}":
            doStop = True
        else: #JSON messages
            message = json.loads(message.payload.decode(encoding='UTF-8'))
            theMessage = json.dumps(message)
            log.debug(theMessage)
            
           
# --------------------------------------------------------------------------- # 
# MQTT Publish the data
# --------------------------------------------------------------------------- # 
def mqttPublish(client, data, subtopic):
    global mqttConnected, mqttErrorCount

    topic = "{}{}/stat/{}".format(argumentValues['mqttRoot'], argumentValues['classicName'], subtopic)
    log.debug("Publishing: {}".format(topic))
    
    try:
        client.publish(topic,data)
        return True
    except Exception as e:
        log.error("MQTT Publish Error Topic:{}".format(topic))
        log.exception(e, exc_info=True)
        mqttConnected = False
        return False


app = Flask(__name__)
counter = 100


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
    # while True:

      # with open("color.txt", "r") as f:
      #   color = f.read()
      #   print("next")
      # if(color != "white"):
        # print(counter)
      # counter = theMessage
      # _data = json.dumps(theMessage)
    yield f"id: 1\ndata: {theMessage}\nevent: online\n\n"
      # time.sleep(0.5)
  return Response(respond_to_client(), mimetype='text/event-stream')
  

# --------------------------------------------------------------------------- # 
# Main
# --------------------------------------------------------------------------- # 
def run(argv):

    global doStop, mqttClient

    log.info("BatteryTester starting up...")

    mqttErrorCount = 0

    #setup the MQTT Client for publishing and subscribing
    clientId = argumentValues['mqttUser'] + "_mqttclient_" + str(randint(100, 999))
    log.info("Connecting with clientId=" + clientId)
    mqttClient = mqttclient.Client(clientId) 
    mqttClient.username_pw_set(argumentValues['mqttUser'], password=argumentValues['mqttPassword'])
    mqttClient.on_connect = on_connect    
    mqttClient.on_disconnect = on_disconnect  
    mqttClient.on_message = on_message

    #Set Last Will 
    will_topic = "{}{}/tele/LWT".format(argumentValues['mqttRoot'], argumentValues['classicName'])
    mqttClient.will_set(will_topic, payload="Offline", qos=0, retain=False)

    try:
        log.info("Connecting to MQTT {}:{}".format(argumentValues['mqttHost'], argumentValues['mqttPort']))
        mqttClient.connect(host=argumentValues['mqttHost'],port=int(argumentValues['mqttPort'])) 
    except Exception as e:
        log.error("Unable to connect to MQTT, exiting...")
        sys.exit(2)

    
    mqttClient.loop_start()


    # #define the stop for the function
    # periodic_stop = threading.Event()
    # # start calling periodic now and every 
    # periodic(periodic_stop)

    # log.debug("Starting main loop...")
    # while not doStop:
    #     try:            
    #         time.sleep(MAIN_LOOP_SLEEP_SECS)
    #         #check to see if shutdown received
    #         if modbusErrorCount > MODBUS_MAX_ERROR_COUNT:
    #             log.error("MODBUS error count exceeded, exiting...")
    #             doStop = True
            
    #         if not mqttConnected:
    #             if (mqttErrorCount > MQTT_MAX_ERROR_COUNT):
    #                 log.error("MQTT Error count exceeded, disconnected, exiting...")
    #                 doStop = True

    #     except KeyboardInterrupt:
    #         log.error("Got Keyboard Interuption, exiting...")
    #         doStop = True
    #     except Exception as e:
    #         log.error("Caught other exception...")
    #         log.exception(e, exc_info=True)
    
    # log.info("Exited the main loop, stopping other loops")
    # log.info("Stopping periodic async...")
    # periodic_stop.set()

    # log.info("Stopping MQTT loop...")
    # mqttClient.loop_stop()

    # log.info("Exiting classic_mqtt")
if __name__ == "__main__":
  run(sys.argv[1:])
  app.run(port=80, debug=True)
  # http_server = WSGIServer(("localhost", 8008), app)
  # http_server.serve_forever()








