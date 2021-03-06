from flask import (
    Blueprint,
    request,
    redirect,
    render_template,
    url_for,
    flash,
    send_file,
    jsonify,
    current_app
)
from .my_mqtt import testers, my_mqtt
from werkzeug.utils import secure_filename
import os
import json

mqttClient = my_mqtt.instance()

firmware = Blueprint(
    "firmware", __name__, static_folder="static", template_folder="templates"
)

@firmware.route("/update_firmware")
def firmware_page():
    return render_template("firmware.html", devices = testers.value)


@firmware.route("/publishOta", methods=["POST"])
def operation():
    data = {}
    data["ServerUrl"] = "http://" + os.getenv("HostName", "localhost") + "/firmware"
    json_data = json.dumps(data)
    mqttClient.publish("5/flash", json_data)
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
        image = request.files["image"]
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