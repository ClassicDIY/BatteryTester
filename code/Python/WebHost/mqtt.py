from paho.mqtt import client as mqttclient
import json
import time
import sys
from random import randint, seed
from logger import log

argumentValues = { 'mqttHost':"192.168.86.25", 'mqttPort':"1883", 'mqttRoot':"BatteryTester", 'mqttUser':"Argon", 'mqttPassword':"volvo4"}

# --------------------------------------------------------------------------- # 
# Counters and status variables
# --------------------------------------------------------------------------- # 
infoPublished               = False
mqttConnected               = False
mqttClient                  = None


# --------------------------------------------------------------------------- # 
# MQTT On Connect function
# --------------------------------------------------------------------------- # 
def on_connect(client, userdata, flags, rc):
    global mqttConnected, mqttErrorCount, mqttClient
    if rc==0:
        log.debug("MQTT connected OK Returned code={}".format(rc))
        #subscribe to the commands
        try:
            topic = "{}/stat/#".format(argumentValues['mqttRoot'])
            client.subscribe(topic)
            log.debug("Subscribed to {}".format(topic))
            topic = "{}/cmnd/#".format(argumentValues['mqttRoot'])
            client.subscribe(topic)
            log.debug("Subscribed to {}".format(topic))
            topic = "{}/tele/#".format(argumentValues['mqttRoot'])
            client.subscribe(topic)
            log.debug("Subscribed to {}".format(topic))
            mqttPublish("", "ping")
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
# MQTT Publish the data
# --------------------------------------------------------------------------- # 
def mqttPublish(data, subtopic):
    global mqttConnected, mqttErrorCount

    topic = "{}/cmnd/{}".format(argumentValues['mqttRoot'], subtopic)
    log.debug("Publishing: {}".format(topic))
    
    try:
        mqttClient.publish(topic, data)
        return True
    except Exception as e:
        log.error("MQTT Publish Error Topic:{}".format(topic))
        log.exception(e, exc_info=True)
        mqttConnected = False
        return False

# --------------------------------------------------------------------------- # 
# Main
# --------------------------------------------------------------------------- # 
def run(on_stat, on_tele, on_cmnd):

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

    mqttClient.message_callback_add("{}/stat/#".format(argumentValues['mqttRoot']), on_stat)
    mqttClient.message_callback_add("{}/tele/#".format(argumentValues['mqttRoot']), on_tele)
    mqttClient.message_callback_add("{}/cmnd/#".format(argumentValues['mqttRoot']), on_cmnd)
    try:
        log.info("Connecting to MQTT {}:{}".format(argumentValues['mqttHost'], argumentValues['mqttPort']))
        mqttClient.connect(host=argumentValues['mqttHost'],port=int(argumentValues['mqttPort'])) 
    except Exception as e:
        log.error("Unable to connect to MQTT, exiting...")
        sys.exit(2)

    
    mqttClient.loop_start()