import json
import math

SENSOR_RANGES = {
    "soil_moisture": (0, 100),
    "temperature": (-40, 80),
    "humidity": (0, 100),
    "light_intensity": (0, 65535),
    "soil_temperature": (-55, 125),
}

SENSOR_UNITS = {
    "soil_moisture": "%",
    "temperature": "C",
    "humidity": "%",
    "light_intensity": "lux",
    "soil_temperature": "C",
}


def decode_payload(raw):
    try:
        if isinstance(raw, bytes):
            raw = raw.decode("utf-8")
        return json.loads(raw)
    except (UnicodeDecodeError, ValueError):
        return None


def is_number(value):
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        return False
    return math.isfinite(value)


def validate_sensor_payload(data, allowed_devices):
    if not isinstance(data, dict):
        return None, ["payload is not a JSON object"]

    device_id = data.get("device_id")
    if not isinstance(device_id, str) or device_id.strip() == "":
        return None, ["missing device_id"]
    if allowed_devices and device_id not in allowed_devices:
        return None, ["unknown device_id: " + device_id]

    seq = data.get("sequence_number")
    if isinstance(seq, bool) or not isinstance(seq, int) or seq < 0:
        return None, ["invalid sequence_number"]

    errors = []
    reading = {"device_id": device_id, "sequence_number": seq}

    for field, (low, high) in SENSOR_RANGES.items():
        value = data.get(field)
        reading[field] = None
        if value is None:
            continue
        if not is_number(value):
            errors.append(field + " is not a number")
        elif value < low or value > high:
            errors.append(f"{field} out of range: {value}")
        else:
            reading[field] = round(float(value), 2)

    if data.get("soil_temperature_ok") is False:
        reading["soil_temperature"] = None

    if all(reading[field] is None for field in SENSOR_RANGES):
        errors.append("no valid sensor values")
        return None, errors

    return reading, errors


def missing_sensors(reading):
    return [field for field in SENSOR_RANGES if reading.get(field) is None]
