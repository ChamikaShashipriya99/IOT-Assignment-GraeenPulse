import logging
import os
import queue
import signal
import time
from collections import deque
from logging.handlers import RotatingFileHandler

import config
import database
import email_reader
import llm_service
import mqtt_client
import payloads
import plant_rules
import weather
from utils import utc_timestamp
from validator import decode_payload, validate_sensor_payload

log = logging.getLogger("main")


def setup_logging():
    os.makedirs(os.path.dirname(config.LOG_FILE), exist_ok=True)

    formatter = logging.Formatter("%(asctime)s UTC %(levelname)s [%(name)s] %(message)s")
    formatter.converter = time.gmtime

    file_handler = RotatingFileHandler(config.LOG_FILE, maxBytes=1_000_000, backupCount=3)
    console_handler = logging.StreamHandler()
    file_handler.setFormatter(formatter)
    console_handler.setFormatter(formatter)

    logging.basicConfig(level=logging.INFO, handlers=[file_handler, console_handler])
    logging.getLogger("httpx").setLevel(logging.WARNING)
    logging.getLogger("openai").setLevel(logging.WARNING)
    logging.getLogger("botocore").setLevel(logging.WARNING)


def stop_on_sigterm(signum, frame):
    raise KeyboardInterrupt


def new_state():
    return {
        "reading": None,
        "reading_time": 0,
        "priority": None,
        "issues": [],
        "weather": None,
        "last_care": 0,
        "last_care_priority": None,
        "last_weather": 0,
        "last_email": 0,
        "recent_sequences": {},
    }


def device_id(state):
    if state["reading"]:
        return state["reading"]["device_id"]
    return config.ALLOWED_DEVICES[0] if config.ALLOWED_DEVICES else "greenpulse_01"


def fresh_reading(state):
    if state["reading"] is None:
        return None
    if time.time() - state["reading_time"] > config.READING_MAX_AGE_MINUTES * 60:
        return None
    return state["reading"]


def is_duplicate(state, reading):
    seen = state["recent_sequences"].setdefault(reading["device_id"], deque(maxlen=20))
    if reading["sequence_number"] in seen:
        return True
    seen.append(reading["sequence_number"])
    return False


def handle_sensor_message(state, raw, table):
    data = decode_payload(raw)
    reading, errors = validate_sensor_payload(data, config.ALLOWED_DEVICES)

    if reading is None:
        log.warning("Rejected sensor message: %s", "; ".join(errors))
        return
    if errors:
        log.warning("Ignored bad fields from %s: %s", reading["device_id"], "; ".join(errors))

    if is_duplicate(state, reading):
        log.info("Skipped duplicate reading #%s", reading["sequence_number"])
        return

    timestamp = utc_timestamp()
    priority, issues = plant_rules.check_reading(reading)

    if table is not None:
        database.save_reading(table, reading, timestamp, priority)

    reading["timestamp"] = timestamp
    state["reading"] = reading
    state["reading_time"] = time.time()
    state["priority"] = priority
    state["issues"] = issues

    log.info(
        "Reading #%s from %s: soil=%s temp=%s hum=%s light=%s priority=%s",
        reading["sequence_number"], reading["device_id"], reading["soil_moisture"],
        reading["temperature"], reading["humidity"], reading["light_intensity"], priority,
    )


def handle_status_message(raw):
    data = decode_payload(raw)
    if data is None:
        text = raw.decode("utf-8", errors="replace") if isinstance(raw, bytes) else str(raw)
        log.info("Device status: %s", text[:200])
    else:
        log.info("Device status: %s", str(data)[:200])


def care_is_due(state, now):
    if fresh_reading(state) is None:
        return False
    if now - state["last_care"] >= config.CARE_INTERVAL_MINUTES * 60:
        return True
    priority_changed = state["priority"] != state["last_care_priority"]
    return priority_changed and now - state["last_care"] >= 60


def current_weather(state):
    if weather.is_fresh(state["weather"]):
        return state["weather"]
    return None


def update_care(client, llm, table, state):
    reading = state["reading"]
    earlier = None
    if table is not None:
        earlier = database.get_reading_from(table, reading["device_id"], 60)

    outdoor = current_weather(state)
    result = llm_service.generate_care(llm, reading, state["priority"], state["issues"], outdoor, earlier)
    payload = payloads.care_payload(reading, state["priority"], result, outdoor)

    mqtt_client.publish_json(client, config.TOPIC_CARE, payload)
    state["last_care_priority"] = state["priority"]


def update_weather(client, llm, state):
    new_weather = weather.fetch_weather()
    if new_weather:
        state["weather"] = new_weather

    outdoor = current_weather(state)
    result = llm_service.generate_weather_summary(llm, outdoor, fresh_reading(state))
    payload = payloads.weather_payload(device_id(state), outdoor, result)
    mqtt_client.publish_json(client, config.TOPIC_WEATHER, payload)


def update_notifications(client, llm, state):
    if not config.EMAIL_ENABLED:
        return
    email_result = email_reader.fetch_plant_emails()
    result = llm_service.summarize_notifications(llm, email_result)
    payload = payloads.notification_payload(device_id(state), email_result, result)
    mqtt_client.publish_json(client, config.TOPIC_NOTIFICATIONS, payload)


def run_jobs(client, llm, table, state):
    if not client.is_connected():
        return

    now = time.time()

    if now - state["last_weather"] >= config.WEATHER_INTERVAL_MINUTES * 60:
        state["last_weather"] = now
        update_weather(client, llm, state)

    if now - state["last_email"] >= config.EMAIL_INTERVAL_MINUTES * 60:
        state["last_email"] = now
        update_notifications(client, llm, state)

    if care_is_due(state, now):
        state["last_care"] = now
        update_care(client, llm, table, state)


def main():
    setup_logging()
    signal.signal(signal.SIGTERM, stop_on_sigterm)
    log.info("Starting GreenPulse backend")

    if not config.IOT_ENDPOINT:
        log.error("IOT_ENDPOINT is not set in .env")
        return

    for path in (config.ROOT_CA_PATH, config.CERT_PATH, config.KEY_PATH):
        if not os.path.exists(path):
            log.error("Certificate file not found: %s", path)
            return

    table = database.get_table() if config.SAVE_TO_DYNAMODB else None
    llm = llm_service.create_llm()

    messages = queue.Queue()
    client = mqtt_client.create_client(messages)
    mqtt_client.start(client)

    state = new_state()

    try:
        while True:
            try:
                topic, raw = messages.get(timeout=1)
            except queue.Empty:
                topic, raw = None, None

            try:
                if topic == config.TOPIC_SENSORS:
                    handle_sensor_message(state, raw, table)
                elif topic == config.TOPIC_STATUS:
                    handle_status_message(raw)
                run_jobs(client, llm, table, state)
            except Exception:
                log.exception("Error in main loop")
    except KeyboardInterrupt:
        log.info("Stopping backend")
    finally:
        mqtt_client.stop(client)


if __name__ == "__main__":
    main()
