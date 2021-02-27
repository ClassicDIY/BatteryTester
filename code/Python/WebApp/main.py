from flask.config import Config
from app import create_app
from config import Config
import os

app = create_app(Config)
ASSETS_DIR = os.path.dirname(os.path.abspath(__file__))
print("Running main {}".format(ASSETS_DIR))

