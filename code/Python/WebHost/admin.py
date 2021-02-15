from flask import Blueprint, blueprints, render_template

admin = Blueprint("admin", __name__, static_folder="static", template_folder="templates")

@admin.route("/home")
@admin.route("/")
def home():
    return render_template("home.html")