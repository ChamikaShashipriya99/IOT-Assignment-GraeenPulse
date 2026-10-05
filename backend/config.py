import os

from dotenv import load_dotenv

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
load_dotenv(os.path.join(BASE_DIR, ".env"))


def get_bool(name, default):
    value = os.getenv(name)
    if value is None:
        return default
    return value.strip().lower() in ("1", "true", "yes", "on")


def get_int(name, default):
    value = os.getenv(name)
    if value is None or value.strip() == "":
        return default
    return int(value)


def get_float(name, default):
    value = os.getenv(name)
    if value is None or value.strip() == "":
        return default
    return float(value)


def local_path(name, default):
    path = os.getenv(name, default)
    if not os.path.isabs(path):
        path = os.path.join(BASE_DIR, path)
    return path


AWS_REGION = os.getenv("AWS_REGION", "us-east-1")
IOT_ENDPOINT = os.getenv("IOT_ENDPOINT", "")
IOT_PORT = get_int("IOT_PORT", 8883)
IOT_CLIENT_ID = os.getenv("IOT_CLIENT_ID", "greenpulse_backend")
ROOT_CA_PATH = local_path("ROOT_CA_PATH", "certs/AmazonRootCA1.pem")
CERT_PATH = local_path("CERT_PATH", "certs/backend-certificate.pem.crt")
KEY_PATH = local_path("KEY_PATH", "certs/backend-private.pem.key")

TOPIC_SENSORS = "greenpulse/sensors"
TOPIC_STATUS = "greenpulse/status"
TOPIC_CARE = "greenpulse/ai/care"
TOPIC_WEATHER = "greenpulse/ai/weather"
TOPIC_NOTIFICATIONS = "greenpulse/ai/notifications"

ALLOWED_DEVICES = [d.strip() for d in os.getenv("ALLOWED_DEVICES", "greenpulse_01").split(",") if d.strip()]
PLANT_TYPE = os.getenv("PLANT_TYPE", "chili plant")

SAVE_TO_DYNAMODB = get_bool("SAVE_TO_DYNAMODB", True)
DYNAMODB_TABLE = os.getenv("DYNAMODB_TABLE", "greenpulse_sensor_data")
DATA_RETENTION_DAYS = get_int("DATA_RETENTION_DAYS", 30)

OPENAI_API_KEY = os.getenv("OPENAI_API_KEY", "")
OPENAI_MODEL = os.getenv("OPENAI_MODEL", "gpt-4.1-mini")

WEATHER_API_KEY = os.getenv("WEATHER_API_KEY", "")
WEATHER_LAT = get_float("WEATHER_LAT", 6.9147)
WEATHER_LON = get_float("WEATHER_LON", 79.9729)
WEATHER_MAX_AGE_MINUTES = get_int("WEATHER_MAX_AGE_MINUTES", 60)

EMAIL_ENABLED = get_bool("EMAIL_ENABLED", True)
EMAIL_HOST = os.getenv("EMAIL_HOST", "imap.gmail.com")
EMAIL_USER = os.getenv("EMAIL_USER", "")
EMAIL_APP_PASSWORD = os.getenv("EMAIL_APP_PASSWORD", "")
EMAIL_DAYS = get_int("EMAIL_DAYS", 3)
EMAIL_MAX_CHECK = get_int("EMAIL_MAX_CHECK", 30)
EMAIL_MAX_RELEVANT = get_int("EMAIL_MAX_RELEVANT", 5)

CARE_INTERVAL_MINUTES = get_int("CARE_INTERVAL_MINUTES", 5)
WEATHER_INTERVAL_MINUTES = get_int("WEATHER_INTERVAL_MINUTES", 15)
EMAIL_INTERVAL_MINUTES = get_int("EMAIL_INTERVAL_MINUTES", 15)
READING_MAX_AGE_MINUTES = get_int("READING_MAX_AGE_MINUTES", 5)

LOG_FILE = local_path("LOG_FILE", "logs/backend.log")
