from datetime import datetime, timezone


def utc_timestamp(dt=None):
    if dt is None:
        dt = datetime.now(timezone.utc)
    return dt.strftime("%Y-%m-%dT%H:%M:%S.%f")[:-3] + "Z"


def short_text(text, limit):
    text = " ".join(str(text).split())
    if len(text) <= limit:
        return text
    return text[:limit - 3].rstrip() + "..."
