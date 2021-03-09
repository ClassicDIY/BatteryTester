from flask import Flask
from flask_mqtt import Mqtt
from multiprocessing import Queue, Value
import json
import sys
from .logger import log
from .message import Message

# --------------------------------------------------------------------------- #
# Sharing Data Between Processes Using multiprocessing Queue
# --------------------------------------------------------------------------- #
mqtt = Mqtt()
theQueue = Queue()
testers = Value("i", 0)
mqttRoot = "BatteryTester"


class my_mqtt:

    _instance = None # singleton

    # --------------------------------------------------------------------------- #
    # setup the MQTT Client for publishing and subscribing
    # --------------------------------------------------------------------------- #

    def init_app(self, app: Flask) -> None:
        global mqttRoot
        mqtt.init_app(app)
        mqttRoot = app.config["MQTT_ROOT"]

    @mqtt.on_connect()
    def handle_connect(client, userdata, flags, rc):
        mqtt.subscribe("{}/stat/#".format(mqttRoot))
        mqtt.subscribe("{}/tele/#".format(mqttRoot))
        mqtt.subscribe("{}/cmnd/#".format(mqttRoot))

    def publish(self, subtopic, data):
        topic = "{}/cmnd/{}".format(mqttRoot, subtopic)
        log.debug("Publishing: {} ".format(topic))
        
        try:
            mqtt.publish(topic, data)
            return True
        except Exception as e:
            log.error("MQTT Publish Error Topic:{}".format(topic))
            log.exception(e, exc_info=True)
            return False
        
    # --------------------------------------------------------------------------- #
    # MQTT On Message
    # --------------------------------------------------------------------------- #

    @mqtt.on_topic("{}/stat/#".format(mqttRoot))
    def handle_stat(client, userdata, message):
        global theQueue
        msg = message.payload.decode(encoding="UTF-8")
        log.debug("Received STAT  {} : {}".format(message.topic, msg))
        if "monitor" in message.topic:
            theMessage = Message("monitor", json.dumps(json.loads(msg)))
            theQueue.put(theMessage)
        elif "mode" in message.topic:
            theMessage = Message("mode", json.dumps(json.loads(msg)))
            theQueue.put(theMessage)
        elif "result" in message.topic:
            theMessage = Message("result", json.dumps(json.loads(msg)))
            theQueue.put(theMessage)

    @mqtt.on_topic("{}/tele/#".format(mqttRoot))
    def handle_stat(client, userdata, message):
        global testers
        msg = message.payload.decode(encoding="UTF-8").upper()
        log.debug("Received TELE  {} : {}".format(message.topic, msg))
        if "ping" in message.topic:
            pl = json.loads(msg)
            tester = int(pl["TESTERNUMBER"])
            if tester > testers.value:
                testers.value = tester
                log.debug("Highest Tester number  {} ".format(testers.value))
            theMessage = Message("ping", json.dumps(pl))
            theQueue.put(theMessage)
        elif "LWT" in message.topic:
            num = message.topic.split('/')[2]
            data = {}
            data["TESTERNUMBER"] = int(num)
            data["STATE"] = msg
            theMessage = Message("LWT", json.dumps(data))
            theQueue.put(theMessage)

    @mqtt.on_topic("{}/cmnd/#".format(mqttRoot))
    def handle_cmnd(client, userdata, message):
        global theQueue
        msg = message.payload.decode(encoding="UTF-8")
        log.debug("Received CMND  {} : {}".format(message.topic, msg))
        if "operation" in message.topic:
            theMessage = Message("operation", msg)
            theQueue.put(theMessage)


    @classmethod
    def instance(cls):
        if cls._instance is None:
            cls._instance = cls.__new__(cls)
            # more init operation here
        return cls._instance
