import logging
import time
from datetime import datetime, timezone

import requests

import config
from utils import utc_timestamp

log = logging.getLogger("weather")

WEATHER_URL = "https://api.openweathermap.org/data/2.5/weather"


def parse_weather(data):
    main = data.get("main") or {}
    conditions = data.get("weather") or [{}]
    rain = data.get("rain") or {}
    wind = data.get("wind") or {}
    clouds = data.get("clouds") or {}

    observed_at = None
    if data.get("dt"):
        observed_at = utc_timestamp(datetime.fromtimestamp(data["dt"], timezone.utc))

    return {
        "location": data.get("name") or "Unknown",
        "temperature": main.get("temp"),
        "feels_like": main.get("feels_like"),
        "humidity": main.get("humidity"),
        "condition": conditions[0].get("description"),
        "rain_1h_mm": rain.get("1h"),
        "cloud_cover": clouds.get("all"),
        "wind_speed": wind.get("speed"),
        "observed_at": observed_at,
    }


def fetch_weather():
    if not config.WEATHER_API_KEY:
        log.warning("WEATHER_API_KEY is not set, skipping weather")
        return None

    params = {
        "lat": config.WEATHER_LAT,
        "lon": config.WEATHER_LON,
        "appid": config.WEATHER_API_KEY,
        "units": "metric",
    }

    try:
        response = requests.get(WEATHER_URL, params=params, timeout=10)
    except requests.RequestException as e:
        log.error("Weather request failed: %s", type(e).__name__)
        return None

    if response.status_code != 200:
        log.error("Weather API returned status %s", response.status_code)
        return None

    try:
        weather = parse_weather(response.json())
    except ValueError:
        log.error("Weather API returned invalid JSON")
        return None

    weather["fetched_at"] = time.time()
    log.info("Weather updated for %s: %s, %s C", weather["location"], weather["condition"], weather["temperature"])
    return weather


def is_fresh(weather):
    if not weather:
        return False
    return time.time() - weather["fetched_at"] < config.WEATHER_MAX_AGE_MINUTES * 60


def weather_for_prompt(weather):
    return {key: value for key, value in weather.items() if key != "fetched_at"}
