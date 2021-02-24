FROM tiangolo/uwsgi-nginx-flask:latest

# Set the working directory to /app
WORKDIR /app
#
# # Copy the current directory contents into the container at /app 
ADD requirements.txt /app
COPY ./app /app
# # Install the dependencies
RUN pip install -r requirements.txt

# run the command to start uWSGI
# CMD ["uwsgi", "app.ini"]