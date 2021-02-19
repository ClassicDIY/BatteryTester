from flask import Flask
from app.settings import settings


app = Flask(__name__)
app.register_blueprint(settings, url_prefix="")


from app import server
server.init()
