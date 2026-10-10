import random

NORMAL = "normal"
WARNING = "warning"
CRITICAL = "critical"
LEVELS = [NORMAL, WARNING, CRITICAL]

RULES = {
    "soil_moisture": {"name": "soil moisture", "normal": (45, 70), "warning": (30, 80)},
    "temperature": {"name": "air temperature", "normal": (22, 30), "warning": (18, 35)},
    "humidity": {"name": "humidity", "normal": (50, 70), "warning": (35, 80)},
    "light_intensity": {"name": "light level", "normal": (10000, 40000), "warning": (2000, float("inf"))},
    "soil_temperature": {"name": "soil temperature", "normal": (20, 28), "warning": (15, 32)},
}

FALLBACK_TIPS = {
    ("soil_moisture", "low"): "The soil is drier than usual. Check the top few centimetres and water slowly if it feels dry.",
    ("soil_moisture", "high"): "The soil is very wet. Hold off on watering and make sure the pot can drain.",
    ("temperature", "low"): "The air around the plant is cool. Keep it away from cold drafts or air conditioning.",
    ("temperature", "high"): "The air is hot today. The soil may dry out faster, so check it more often.",
    ("humidity", "low"): "The air is dry. Keeping the plant near other plants can help hold some moisture.",
    ("humidity", "high"): "Humidity is high. Give the plant some airflow so the leaves can dry.",
    ("light_intensity", "low"): "Light is low for a chili plant. Try moving it closer to a bright window.",
    ("light_intensity", "high"): "The light is very strong right now. A little afternoon shade may help.",
    ("soil_temperature", "low"): "The soil is cold. Keep the pot off cold floors and away from drafts.",
    ("soil_temperature", "high"): "The soil is warm. Avoid leaving the pot in hot direct sun for too long.",
}

DEFAULT_TIP = "Conditions look stable. Keep checking the soil before each watering."

FALLBACK_QUOTES = [
    "Small roots, steady care, and the patience of a quiet morning.",
    "A plant keeps no calendar, only the memory of how it was cared for.",
    "Green things grow in the space between watching and waiting.",
    "Every drop you give comes back slowly, as a leaf.",
]


def worse(a, b):
    return a if LEVELS.index(a) >= LEVELS.index(b) else b


def check_reading(reading):
    priority = NORMAL
    issues = []

    for field, rule in RULES.items():
        value = reading.get(field)
        if value is None:
            continue

        low, high = rule["normal"]
        if low <= value <= high:
            continue

        direction = "low" if value < low else "high"
        warn_low, warn_high = rule["warning"]
        level = WARNING if warn_low <= value <= warn_high else CRITICAL

        issues.append({
            "field": field,
            "level": level,
            "direction": direction,
            "text": f"{rule['name']} is {direction} ({level})",
        })
        priority = worse(priority, level)

    issues.sort(key=lambda issue: LEVELS.index(issue["level"]), reverse=True)
    return priority, issues


def fallback_care_tip(issues):
    if not issues:
        return DEFAULT_TIP
    first = issues[0]
    return FALLBACK_TIPS.get((first["field"], first["direction"]), DEFAULT_TIP)


def fallback_quote():
    return random.choice(FALLBACK_QUOTES)
