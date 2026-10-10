import json
import logging

import paho.mqtt.client as mqtt

import config

log = logging.getLogger("mqtt")

MAX_PAYLOAD_BYTES = 900


def create_client(message_queue):
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id=config.IOT_CLIENT_ID)
    client.tls_set(
        ca_certs=config.ROOT_CA_PATH,
        certfile=config.CERT_PATH,
        keyfile=config.KEY_PATH,
    )
    client.user_data_set(message_queue)
    client.on_connect = on_connect
    client.on_connect_fail = on_connect_fail
    client.on_disconnect = on_disconnect
    client.on_message = on_message
    client.reconnect_delay_set(min_delay=1, max_delay=60)
    return client


def start(client):
    client.connect_async(config.IOT_ENDPOINT, config.IOT_PORT, keepalive=60)
    client.loop_start()


def stop(client):
    client.loop_stop()
    client.disconnect()


def on_connect(client, userdata, flags, reason_code, properties):
    if reason_code.is_failure:
        log.error("AWS IoT Core refused the connection: %s", reason_code)
        return
    log.info("Connected to AWS IoT Core as %s", config.IOT_CLIENT_ID)
    client.subscribe([(config.TOPIC_SENSORS, 1), (config.TOPIC_STATUS, 1)])


def on_connect_fail(client, userdata):
    log.error("Could not reach AWS IoT Core at %s:%s, retrying", config.IOT_ENDPOINT, config.IOT_PORT)


def on_disconnect(client, userdata, flags, reason_code, properties):
    if reason_code.is_failure:
        log.warning("Lost connection to AWS IoT Core: %s", reason_code)
    else:
        log.info("Disconnected from AWS IoT Core")


def on_message(client, userdata, msg):
    userdata.put((msg.topic, msg.payload))


def publish_json(client, topic, data):
    payload = json.dumps(data)

    if len(payload) > MAX_PAYLOAD_BYTES and "llm_input" in data:
        data = {key: value for key, value in data.items() if key != "llm_input"}
        payload = json.dumps(data)
        log.warning("Payload for %s was too big, sent without llm_input", topic)

    if len(payload) > MAX_PAYLOAD_BYTES:
        log.warning("Payload for %s is %d bytes, the ESP32 may drop it", topic, len(payload))

    result = client.publish(topic, payload, qos=1)
    if result.rc != mqtt.MQTT_ERR_SUCCESS:
        log.error("Publish to %s failed: %s", topic, mqtt.error_string(result.rc))
        return False

    log.info("Published to %s (%d bytes)", topic, len(payload))
    return True
