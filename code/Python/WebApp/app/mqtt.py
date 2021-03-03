from flask import current_app
from paho.mqtt import client as mqttclient
import json
import time
import sys
from random import randint, seed
from .logger import log

class mqtt(object):

    mqttClient                  = None
    mqttRoot                    = "Battery"

    _instance = None

    def on_connect(self, client, userdata, flags, rc):
        global mqttClient
        log.debug("on_connect")
        if rc==0:
            log.debug("MQTT connected OK Returned code={}".format(rc))
            #subscribe to the commands
            try:
                topic = "{}/stat/#".format(mqttRoot)
                client.subscribe(topic)
                log.debug("Subscribed to {}".format(topic))
                topic = "{}/cmnd/#".format(mqttRoot)
                client.subscribe(topic)
                log.debug("Subscribed to {}".format(topic))
                topic = "{}/tele/#".format(mqttRoot)
                client.subscribe(topic)
                log.debug("Subscribed to {}".format(topic))
                
            except Exception as e:
                log.error("MQTT Subscribe failed")
                log.exception(e, exc_info=True)
            self.mqttPublish("", "ping")
        else:
            log.error("MQTT Bad connection Returned code={}".format(rc))

    def on_disconnect(client, userdata, rc):
        global mqttClient
        log.debug("on_disconnect")
        #if disconnetion was unexpectred (not a result of a disconnect request) then log it.
        if rc!=mqttclient.MQTT_ERR_SUCCESS:
            log.error("on_disconnect: Disconnected. ReasonCode={}".format(rc))

    def mqttPublish(self, data, subtopic):
        topic = "{}/cmnd/{}".format(mqttRoot, subtopic)
        log.debug("Publishing: {}".format(topic))
        
        try:
            mqttClient.publish(topic, data)
            return True
        except Exception as e:
            log.error("MQTT Publish Error Topic:{}".format(topic))
            log.exception(e, exc_info=True)
            return False
        
    def disconnect(self):
        mqttClient.disconnect()
        
    #setup the MQTT Client for publishing and subscribing
    def init(self, on_stat = None, on_tele = None, on_cmnd = None):
        global mqttClient, mqttRoot

        clientId = current_app.config['MQTT_USER'] + "_mqttclient_" + str(randint(100, 999))
        mqttClient = mqttclient.Client(clientId) 
        mqttClient.username_pw_set(current_app.config['MQTT_USER'], password=current_app.config['MQTT_PASSWORD'])
        if on_stat is not None:
            mqttClient.on_connect = self.on_connect    
            mqttClient.on_disconnect = self.on_disconnect  
            mqttClient.message_callback_add("{}/stat/#".format(current_app.config['MQTT_ROOT']), on_stat)
            mqttClient.message_callback_add("{}/tele/#".format(current_app.config['MQTT_ROOT']), on_tele)
            mqttClient.message_callback_add("{}/cmnd/#".format(current_app.config['MQTT_ROOT']), on_cmnd)
        try:
            log.info("Connecting to MQTT {}:{}".format(current_app.config['MQTT_HOST'], current_app.config['MQTT_PORT']))
            mqttClient.connect(host=current_app.config['MQTT_HOST'],port=int(current_app.config['MQTT_PORT'])) 
        except Exception as e:
            log.error("Unable to connect to MQTT, exiting...")
            sys.exit(2)
        mqttRoot = current_app.config['MQTT_ROOT']
        mqttClient.loop_start()

    @classmethod
    def instance(cls, on_stat = None, on_tele = None, on_cmnd = None):
        if cls._instance is None:
            cls._instance = cls.__new__(cls)
            cls._instance.init(on_stat, on_tele, on_cmnd)
            # more init operation here
        return cls._instance
