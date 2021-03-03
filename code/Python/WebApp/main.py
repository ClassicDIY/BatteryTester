from flask.config import Config
from app import create_app
import os

app = create_app(Config)
ASSETS_DIR = os.path.dirname(os.path.abspath(__file__))
print("Running main {} Host {}".format(ASSETS_DIR,  os.getenv("HostName", "localhost") ))

