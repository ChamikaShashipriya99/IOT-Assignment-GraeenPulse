import email
import html
import imaplib
import logging
import re
from datetime import date, timedelta
from email.header import decode_header, make_header
from email.utils import parseaddr

import config
from utils import short_text

log = logging.getLogger("email")

KEYWORD_PATTERN = re.compile(
    r"\b(plant|water|garden|weather|rain|storm|flood|fertili|soil|seed|greenpulse|chili|chilli|nursery|pest|compost)"
)


def decode_text(value):
    if not value:
        return ""
    try:
        return str(make_header(decode_header(value)))
    except Exception:
        return str(value)


def is_relevant(subject):
    return KEYWORD_PATTERN.search(subject.lower()) is not None


def clean_text(text, limit=500):
    text = re.sub(r"\S+@\S+", "[email]", text)
    text = re.sub(r"https?://\S+|www\.\S+", "[link]", text)
    return short_text(text, limit)


def decode_part(part):
    payload = part.get_payload(decode=True)
    if payload is None:
        return ""
    charset = part.get_content_charset() or "utf-8"
    try:
        return payload.decode(charset, errors="replace")
    except LookupError:
        return payload.decode("utf-8", errors="replace")


def html_to_text(text):
    text = re.sub(r"(?is)<(style|script).*?</\1>", " ", text)
    text = re.sub(r"<[^>]+>", " ", text)
    return html.unescape(text)


def get_body(msg):
    html_text = None
    for part in msg.walk():
        if part.get_content_maintype() == "multipart":
            continue
        if part.get_content_disposition() == "attachment":
            continue
        if part.get_content_type() == "text/plain":
            return decode_part(part)
        if part.get_content_type() == "text/html" and html_text is None:
            html_text = html_to_text(decode_part(part))
    return html_text or ""


def fetch_plant_emails():
    if not config.EMAIL_ENABLED:
        return None
    if not config.EMAIL_USER or not config.EMAIL_APP_PASSWORD:
        log.warning("Email login is not set, skipping notifications")
        return None

    since = (date.today() - timedelta(days=config.EMAIL_DAYS)).strftime("%d-%b-%Y")
    mail = None

    try:
        mail = imaplib.IMAP4_SSL(config.EMAIL_HOST, timeout=20)
        mail.login(config.EMAIL_USER, config.EMAIL_APP_PASSWORD)
        mail.select("INBOX", readonly=True)

        status, data = mail.search(None, "SINCE", since)
        if status != "OK":
            log.error("Email search failed")
            return None

        ids = data[0].split()[-config.EMAIL_MAX_CHECK:]
        messages = []

        for msg_id in reversed(ids):
            status, header_data = mail.fetch(msg_id, "(BODY.PEEK[HEADER.FIELDS (SUBJECT FROM DATE)])")
            if status != "OK" or not header_data or header_data[0] is None:
                continue

            header = email.message_from_bytes(header_data[0][1])
            subject = decode_text(header.get("Subject"))
            if not is_relevant(subject):
                continue

            status, full_data = mail.fetch(msg_id, "(BODY.PEEK[])")
            if status != "OK" or not full_data or full_data[0] is None:
                continue

            msg = email.message_from_bytes(full_data[0][1])
            sender = parseaddr(decode_text(header.get("From")))[0] or "Unknown sender"

            messages.append({
                "subject": clean_text(subject, 120),
                "from": clean_text(sender, 60),
                "date": header.get("Date", ""),
                "text": clean_text(get_body(msg)),
            })

            if len(messages) >= config.EMAIL_MAX_RELEVANT:
                break

        log.info("Checked %d emails, %d look plant related", len(ids), len(messages))
        return {"checked": len(ids), "messages": messages}

    except imaplib.IMAP4.error as e:
        log.error("Email login or read failed: %s", e)
        return None
    except OSError as e:
        log.error("Could not reach the email server: %s", type(e).__name__)
        return None
    finally:
        if mail is not None:
            try:
                mail.logout()
            except Exception:
                pass
