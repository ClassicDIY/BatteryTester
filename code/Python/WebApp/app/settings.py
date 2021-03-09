from flask import Blueprint, request, redirect, render_template, Response, url_for, jsonify
import json
from .my_mqtt import theQueue, my_mqtt
from .logger import log

mqttClient = my_mqtt.instance()

settings = Blueprint("settings", __name__, static_folder="static", template_folder="templates")
@settings.route("/settings")
def setting_page():
    mqttClient.publish("ping", "")
    return render_template("settings.html")


@settings.route("/listen")
def listen():
    def respond_to_client():
        global theQueue
        while not theQueue.empty():
            theMessage = theQueue.get()
            yield f"data: {theMessage.data}\nevent: {theMessage.topic}\n\n"

    return Response(respond_to_client(), mimetype="text/event-stream")


@settings.route("/configure", methods=["POST"])
def configure_page():
    data = {}
    data["LowCutoff"] = request.form.get("LowCutoff")
    data["ThermalShutdownTemperature"] = int(request.form.get("ThermalShutdownTemperature")) * 10
    data["StorageVoltage"] = request.form.get("StorageVoltage")
    data["StabilizeDuration"] = request.form.get("StabilizeDuration")
    data["ChargeCurrent"] = request.form.get("ChargeCurrent")
    data["ChargeDischargeCycleCount"] = request.form.get("ChargeDischargeCycleCount")
    json_data = json.dumps(data)
    mqttClient.publish("config", json_data)
    return redirect(url_for("settings.setting_page"))
