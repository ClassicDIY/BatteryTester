from flask import (
    Flask,
    Response,
    request,
    url_for,
    redirect,
    render_template,
    jsonify,
    Config
)

from .settings import settings
from .firmware import firmware
from .my_mqtt import theQueue, testers, my_mqtt
from .logger import log

mqttClient = my_mqtt.instance()

def create_app(config_class=Config):
    # create and configure the app
    app = Flask(__name__)
    if app.config["ENV"] == "production":
        log.debug("************production************")
        app.config.from_object("config.ProductionConfig")
    else:
        log.debug("************DevelopmentConfig************")
        app.config.from_object("config.DevelopmentConfig")
    app.register_blueprint(settings, url_prefix="")
    app.register_blueprint(firmware, url_prefix="")
    print(app.config)
    mqttClient.init_app(app)
    

    @app.route("/")
    def render_index():
        return render_template(
            "index.html", operation="Monitor", cellCount=testers.value * 2
        )

    @app.route("/refresh")
    def render_refresh():
        testers.value = 0
        mqttClient.publish("ping", "")
        return redirect(url_for("render_index"))

    @app.route("/operation", methods=["POST"])
    def operation():
        current_operation = request.form.get("operation")
        mqttClient.publish("operation", current_operation)
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
