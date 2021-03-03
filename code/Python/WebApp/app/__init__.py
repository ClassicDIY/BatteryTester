from flask import (
    Flask,
    Response,
    request,
    url_for,
    redirect,
    render_template,
    jsonify,
    session,
    Config,
    g
)
from multiprocessing import Queue, Value
import json
from .settings import settings
from .firmware import firmware
from .mqtt import mqtt
from .logger import log
from .message import Message

# --------------------------------------------------------------------------- #
# Sharing Data Between Processes Using multiprocessing Queue
# --------------------------------------------------------------------------- #
theQueue = Queue()
testers = Value("i", 0)


def create_app(config_class=Config):
    # create and configure the app
    app = Flask(__name__)
    if app.config["ENV"] == "production":
        app.config.from_object("config.ProductionConfig")
    else:
        app.config.from_object("config.DevelopmentConfig")
    app.register_blueprint(settings, url_prefix="")
    app.register_blueprint(firmware, url_prefix="")

    print(app.config)
    with app.app_context():
        init_mqtt()
    # @app.teardown_appcontext
    # def teardown_mqtt(exception):
    #     log.debug("************teardown_mqtt************")
    #     mqtt = g.pop('mqtt', None)
    #     if mqtt is not None:
    #         mqtt.disconnect()
    @app.route("/")
    def render_index():
        session["testers"] = testers.value
        return render_template(
            "index.html", operation="Monitor", cellCount=testers.value * 2
        )

    @app.route("/refresh")
    def render_refresh():
        testers.value = 0
        con = mqtt.instance()
        con.mqttPublish("", "ping")
        return redirect(url_for("render_index"))

    @app.route("/operation", methods=["POST"])
    def operation():
        current_operation = request.form.get("operation")
        con = mqtt.instance()
        con.mqttPublish(current_operation, "operation")
        return jsonify(status="success")

    @app.route("/listen")
    def listen():
        def respond_to_client():
            global theQueue
            while not theQueue.empty():
                theMessage = theQueue.get()
                yield f"data: {theMessage.data}\nevent: {theMessage.topic}\n\n"

        return Response(respond_to_client(), mimetype="text/event-stream")

    return app


# --------------------------------------------------------------------------- #
# MQTT On Message
# --------------------------------------------------------------------------- #


def on_stat(client, userdata, message):

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


def on_tele(client, userdata, message):
    global testers
    msg = message.payload.decode(encoding="UTF-8").upper()
    log.debug("Received TELE  {} : {}".format(message.topic, msg))
    if "ping" in message.topic:
        pl = json.loads(msg)
        tester = int(pl["TESTERNUMBER"])
        if tester > testers.value:
            testers.value = tester
        log.debug("cellCount at on_tele  {} ".format(testers.value))


def on_cmnd(client, userdata, message):
    global theQueue
    msg = message.payload.decode(encoding="UTF-8")
    log.debug("Received CMND  {} : {}".format(message.topic, msg))
    if "operation" in message.topic:
        theMessage = Message("operation", msg)
        theQueue.put(theMessage)


def init_mqtt():
    testers.value = 0
    con = mqtt.instance(on_stat, on_tele, on_cmnd)
    log.info("************BatteryTester init_mqtt()*********")

