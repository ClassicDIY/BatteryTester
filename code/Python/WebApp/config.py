class Config(object):
    DEBUG = False
    TESTING = False
    FIRMWARE_UPLOADS = "uploads"
    SECRET_KEY=b'_5#y2L"F4Q8z\n\xec]/'
    MQTT_BROKER_URL = "192.168.86.25"
    MQTT_BROKER_PORT = 1883
    MQTT_ROOT = "BatteryTester"
    MQTT_USERNAME = "Argon"
    MQTT_PASSWORD = "volvo4"

class ProductionConfig(Config):
    pass

class DevelopmentConfig(Config):
    DEBUG = True

class TestingConfig(Config):
    TESTING = True