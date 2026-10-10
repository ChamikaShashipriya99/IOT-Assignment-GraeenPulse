import json
import logging
import re

from langchain_core.prompts import ChatPromptTemplate

import config
import plant_rules
from utils import short_text
from weather import weather_for_prompt

log = logging.getLogger("llm")

CARE_TIP_LIMIT = 250
QUOTE_LIMIT = 150
WEATHER_SUMMARY_LIMIT = 250
WEATHER_ADVICE_LIMIT = 200
NOTIFICATION_LIMIT = 300

HEALTH_CLAIM_PATTERN = re.compile(
    r"\b(disease\w*|diagnos\w*|infect\w*|fung\w*|virus\w*|bacteri\w*|rot|rotting|pesticide\w*|insecticide\w*)\b"
)

CARE_SYSTEM = """You are the GreenPulse plant-care assistant for a {plant_type}.
You get the latest sensor readings from the plant's pot, the result of a simple threshold check, and the outdoor weather if it is available.

Rules:
- Use only the values you are given. Never make up a reading. A null value means that sensor is not available, so do not describe it.
- Do not diagnose diseases, pests or the health of the plant. Only talk about the growing conditions.
- care_tip: one or two short sentences (max 250 characters) with a practical thing the user can do now.
- literary_quote: one original literary-style line (max 150 characters) inspired by the current conditions. Do not quote real authors.

Reply with JSON only, in this format:
{{"care_tip": "...", "literary_quote": "..."}}"""

WEATHER_SYSTEM = """You are the GreenPulse weather assistant for a {plant_type} that is kept at home.
You get the current outdoor weather from a weather API and the latest indoor sensor readings if they are available.

Rules:
- summary: describe the actual outdoor weather in one or two sentences (max 250 characters). Use only the given weather values.
- plant_advice: one sentence (max 200 characters) on how this weather could affect watering or care of the plant.
- Keep outdoor weather and indoor readings separate. Do not make up rain, alerts or forecasts that are not in the data. A null value means it was not reported.

Reply with JSON only, in this format:
{{"summary": "...", "plant_advice": "..."}}"""

NOTIFICATION_SYSTEM = """You are the GreenPulse notification assistant for a {plant_type} owner.
You get a few recent emails that matched plant, watering, gardening or weather keywords.

Rules:
- Summarise only what is written in the emails (max 300 characters). Do not make up messages, dates or tasks.
- Some emails may only match a keyword by accident. Ignore emails that are not really about plants, gardening, watering or weather.
- Do not include email addresses, links or personal details.
- relevant_count: the number of emails that were really about plants, gardening, watering or weather.

Reply with JSON only, in this format:
{{"summary": "...", "relevant_count": 0}}"""

CARE_PROMPT = ChatPromptTemplate.from_messages([("system", CARE_SYSTEM), ("human", "{input}")])
WEATHER_PROMPT = ChatPromptTemplate.from_messages([("system", WEATHER_SYSTEM), ("human", "{input}")])
NOTIFICATION_PROMPT = ChatPromptTemplate.from_messages([("system", NOTIFICATION_SYSTEM), ("human", "{input}")])


def create_llm():
    if not config.OPENAI_API_KEY:
        log.warning("OPENAI_API_KEY is not set, fallback messages will be used")
        return None

    from langchain_openai import ChatOpenAI

    return ChatOpenAI(
        model=config.OPENAI_MODEL,
        api_key=config.OPENAI_API_KEY,
        temperature=0.7,
        timeout=20,
        max_retries=0,
    )


def reply_text(reply):
    content = reply.content
    if isinstance(content, list):
        parts = []
        for part in content:
            if isinstance(part, dict):
                parts.append(part.get("text", ""))
            else:
                parts.append(str(part))
        content = "".join(parts)
    return content


def parse_json_reply(text):
    start = text.find("{")
    end = text.rfind("}")
    if start == -1 or end <= start:
        return None
    try:
        return json.loads(text[start:end + 1])
    except ValueError:
        return None


def check_text(value):
    if not isinstance(value, str) or not value.strip():
        return "missing or empty"
    if HEALTH_CLAIM_PATTERN.search(value.lower()):
        return "has a plant health claim"
    return None


def check_fields(result, fields):
    if not isinstance(result, dict):
        return "reply is not a JSON object"
    for field in fields:
        problem = check_text(result.get(field))
        if problem:
            return field + " " + problem
    return None


def ask(llm, prompt, data, check):
    if llm is None:
        return None

    chain = prompt | llm
    for attempt in range(2):
        try:
            reply = chain.invoke({"plant_type": config.PLANT_TYPE, "input": json.dumps(data, indent=2)})
        except Exception as e:
            log.error("LLM call failed (attempt %d): %s", attempt + 1, type(e).__name__)
            continue

        result = parse_json_reply(reply_text(reply))
        problem = check(result)
        if problem is None:
            return result
        log.warning("LLM reply rejected (attempt %d): %s", attempt + 1, problem)

    return None


def readings_for_prompt(reading):
    return {
        "soil_moisture_percent": reading.get("soil_moisture"),
        "air_temperature_c": reading.get("temperature"),
        "humidity_percent": reading.get("humidity"),
        "light_lux": reading.get("light_intensity"),
        "soil_temperature_c": reading.get("soil_temperature"),
    }


def care_input(reading, priority, issues, weather, earlier):
    data = {
        "plant": config.PLANT_TYPE,
        "readings": readings_for_prompt(reading),
        "reading_time": reading.get("timestamp"),
        "priority": priority,
        "issues": [issue["text"] for issue in issues] or ["all available readings are in the normal range"],
        "outdoor_weather": weather_for_prompt(weather) if weather else "not available",
    }
    if earlier and earlier.get("soil_moisture") is not None:
        data["earlier_reading"] = {
            "time": earlier["timestamp"],
            "soil_moisture_percent": earlier["soil_moisture"],
        }
    return data


def generate_care(llm, reading, priority, issues, weather=None, earlier=None):
    data = care_input(reading, priority, issues, weather, earlier)
    result = ask(llm, CARE_PROMPT, data, lambda r: check_fields(r, ["care_tip", "literary_quote"]))

    if result:
        return {
            "care_tip": short_text(result["care_tip"], CARE_TIP_LIMIT),
            "literary_quote": short_text(result["literary_quote"], QUOTE_LIMIT),
            "source": "llm",
        }

    return {
        "care_tip": plant_rules.fallback_care_tip(issues),
        "literary_quote": plant_rules.fallback_quote(),
        "source": "fallback",
    }


def fallback_weather_summary(weather):
    parts = []
    if weather.get("condition"):
        parts.append(weather["condition"].capitalize())
    if weather.get("temperature") is not None:
        parts.append(f"{weather['temperature']:.1f} C")
    if weather.get("humidity") is not None:
        parts.append(f"humidity {weather['humidity']}%")
    text = ", ".join(parts) if parts else "Weather details are limited"
    return f"{text} in {weather.get('location', 'your area')}."


def generate_weather_summary(llm, weather, reading=None):
    if not weather:
        return None

    data = {
        "plant": config.PLANT_TYPE,
        "outdoor_weather": weather_for_prompt(weather),
        "indoor_readings": readings_for_prompt(reading) if reading else "not available",
    }
    result = ask(llm, WEATHER_PROMPT, data, lambda r: check_fields(r, ["summary", "plant_advice"]))

    if result:
        return {
            "summary": short_text(result["summary"], WEATHER_SUMMARY_LIMIT),
            "plant_advice": short_text(result["plant_advice"], WEATHER_ADVICE_LIMIT),
            "source": "llm",
        }

    return {
        "summary": fallback_weather_summary(weather),
        "plant_advice": "Check the soil in the pot before watering, indoor conditions can be different from outside.",
        "source": "fallback",
    }


def check_notification(result, email_count):
    problem = check_fields(result, ["summary"])
    if problem:
        return problem
    count = result.get("relevant_count")
    if isinstance(count, bool) or not isinstance(count, int) or count < 0 or count > email_count:
        return "relevant_count does not match the emails given"
    return None


def summarize_notifications(llm, email_result):
    if email_result is None:
        return None

    messages = email_result["messages"]
    if not messages:
        return {
            "summary": f"No plant, watering or weather emails in the last {config.EMAIL_DAYS} days.",
            "relevant_count": 0,
            "source": "rules",
        }

    data = {"emails": messages}
    result = ask(llm, NOTIFICATION_PROMPT, data, lambda r: check_notification(r, len(messages)))

    if result:
        return {
            "summary": short_text(result["summary"], NOTIFICATION_LIMIT),
            "relevant_count": result["relevant_count"],
            "source": "llm",
        }

    return {
        "summary": f"{len(messages)} email(s) mention plants, watering or weather. Open your inbox to read them.",
        "relevant_count": len(messages),
        "source": "fallback",
    }
