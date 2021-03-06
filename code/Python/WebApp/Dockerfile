FROM python:3.7.2-stretch
WORKDIR /BatteryTester
ENV STATIC_URL /static
ENV STATIC_PATH /BatteryTester/app/static
ADD requirements.txt /BatteryTester
ADD main.py /BatteryTester
ADD config.py /BatteryTester
# ADD uwsgi.ini /BatteryTester
COPY ./app /BatteryTester/app
# # Install the dependencies
RUN pip install -r requirements.txt
ENTRYPOINT FLASK_APP=main.py flask run --host=0.0.0.0 --no-reload
# run the command to start uWSGI
# CMD ["uwsgi", "uwsgi.ini"]