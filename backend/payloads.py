from utils import utc_timestamp


def weather_short(weather):
    if not weather:
        return None
    text = weather.get("condition") or "unknown"
    if weather.get("temperature") is not None:
        text += f", {weather['temperature']:.1f} C"
    return text


def care_payload(reading, priority, result, weather=None):
    return {
        "device_id": reading["device_id"],
        "timestamp": utc_timestamp(),
        "priority": priority,
        "care_tip": result["care_tip"],
        "literary_quote": result["literary_quote"],
        "source": result["source"],
        "llm_input": {
            "sequence_number": reading["sequence_number"],
            "reading_time": reading.get("timestamp"),
            "soil_moisture": reading.get("soil_moisture"),
            "temperature": reading.get("temperature"),
            "humidity": reading.get("humidity"),
            "light_intensity": reading.get("light_intensity"),
            "soil_temperature": reading.get("soil_temperature"),
            "weather": weather_short(weather),
        },
    }


def weather_payload(device_id, weather, result):
    if not weather or not result:
        return {
            "device_id": device_id,
            "timestamp": utc_timestamp(),
            "available": False,
            "summary": "Weather data is not available right now.",
            "source": "none",
        }

    return {
        "device_id": device_id,
        "timestamp": utc_timestamp(),
        "available": True,
        "location": weather["location"],
        "temperature": weather["temperature"],
        "humidity": weather["humidity"],
        "condition": weather["condition"],
        "rain_1h_mm": weather["rain_1h_mm"],
        "observed_at": weather["observed_at"],
        "summary": result["summary"],
        "plant_advice": result["plant_advice"],
        "source": result["source"],
    }


def notification_payload(device_id, email_result, result):
    if email_result is None or result is None:
        return {
            "device_id": device_id,
            "timestamp": utc_timestamp(),
            "available": False,
            "summary": "Email notifications could not be checked right now.",
            "source": "none",
        }

    return {
        "device_id": device_id,
        "timestamp": utc_timestamp(),
        "available": True,
        "emails_checked": email_result["checked"],
        "relevant_count": result["relevant_count"],
        "summary": result["summary"],
        "source": result["source"],
    }
