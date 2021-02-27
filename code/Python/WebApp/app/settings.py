from flask import Blueprint, request, redirect, render_template

settings = Blueprint("settings", __name__, static_folder="static", template_folder="templates")
@settings.route("/settings")
def setting_page():

    return render_template("settings.html")


