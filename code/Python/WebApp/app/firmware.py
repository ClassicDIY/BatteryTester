from flask import (
    Blueprint,
    request,
    Response,
    redirect,
    render_template,
    url_for,
    flash,
    send_file,
    jsonify,
    current_app
)
from .my_mqtt import testers, theQueue, my_mqtt
from werkzeug.utils import secure_filename
import os
import json
from .logger import log

mqttClient = my_mqtt.instance()

firmware = Blueprint(
    "firmware", __name__, static_folder="static", template_folder="templates"
)

@firmware.route("/update_firmware")
def firmware_page():
    mqttClient.publish("ping", "")
    return render_template("firmware.html", devices = testers.value)


@firmware.route("/publishOta", methods=["POST"])
def operation():
    tester=request.form['data']
    data = {}
    data["ServerUrl"] = "http://" + os.getenv("HostName", "localhost") + "/firmware"
    json_data = json.dumps(data)
    mqttClient.publish(tester + "/flash", json_data)
    return jsonify(status="success")


def allowed_file(filename):
    if not "." in filename:
        return False
    ext = filename.rsplit(".", 1)[1]
    if ext.upper() in ["BIN"]:
        return True
    else:
        return False


@firmware.route("/upload-firmware", methods=["GET", "POST"])
def upload_firmware():
    if request.method == "POST":
        # check if the post request has the file part
        if "image" not in request.files:
            flash("No file part")
        image = request.files["firmware"]
        # if user does not select file, browser also submit an empty part without filename
        if image.filename == "":
            flash("No selected file")
        if image and allowed_file(image.filename):
            filename = secure_filename(image.filename)
            image.save(
                os.path.join(
                    current_app.root_path,
                    current_app.config["FIRMWARE_UPLOADS"],
                    image.filename,
                )
            )
            print("Image Saved")
    return redirect(url_for("firmware.firmware_page"))


@firmware.route("/firmware")
def download_firmware():
    firmware = os.path.join(
        current_app.root_path, current_app.config["FIRMWARE_UPLOADS"], "firmware.bin"
    )
    print("Firmware: {}".format(firmware))
    if os.path.exists(firmware):
        return send_file(
            firmware, mimetype="application/octet-stream", as_attachment=True
        )
    else:
        firmware.errorhandler(404)


@firmware.route("/listen")
def listen():
    def respond_to_client():
        global theQueue
        while not theQueue.empty():
            theMessage = theQueue.get()
            yield f"data: {theMessage.data}\nevent: {theMessage.topic}\n\n"

    return Response(respond_to_client(), mimetype="text/event-stream")
