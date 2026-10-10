import logging
import time
from datetime import datetime, timedelta, timezone
from decimal import Decimal

import boto3
from boto3.dynamodb.conditions import Key

import config
from utils import utc_timestamp
from validator import SENSOR_RANGES

log = logging.getLogger("database")


def get_table():
    dynamodb = boto3.resource("dynamodb", region_name=config.AWS_REGION)
    return dynamodb.Table(config.DYNAMODB_TABLE)


def build_item(reading, timestamp, priority):
    item = {
        "device_id": reading["device_id"],
        "timestamp": timestamp,
        "sequence_number": reading["sequence_number"],
        "priority": priority,
        "expires_at": int(time.time()) + config.DATA_RETENTION_DAYS * 24 * 60 * 60,
    }
    for field in SENSOR_RANGES:
        if reading.get(field) is not None:
            item[field] = Decimal(str(reading[field]))
    return item


def save_reading(table, reading, timestamp, priority):
    try:
        table.put_item(Item=build_item(reading, timestamp, priority))
        return True
    except Exception as e:
        log.error("Could not save reading to DynamoDB: %s", e)
        return False


def get_reading_from(table, device_id, minutes_ago):
    since = utc_timestamp(datetime.now(timezone.utc) - timedelta(minutes=minutes_ago))
    try:
        response = table.query(
            KeyConditionExpression=Key("device_id").eq(device_id) & Key("timestamp").gte(since),
            ScanIndexForward=True,
            Limit=1,
        )
    except Exception as e:
        log.error("Could not read history from DynamoDB: %s", e)
        return None

    items = response.get("Items", [])
    if not items:
        return None

    old = {"timestamp": items[0]["timestamp"]}
    for field in SENSOR_RANGES:
        value = items[0].get(field)
        old[field] = float(value) if value is not None else None
    return old
