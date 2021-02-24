from flask import Blueprint, blueprints, render_template

settings = Blueprint("settings", __name__, static_folder="static", template_folder="templates")

@settings.route("/settings")
def home():
    return render_template("settings.html")