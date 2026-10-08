import argparse
import json
import random
import time

import paho.mqtt.client as mqtt

import config


def make_reading(seq, scenario):
    reading = {
        "device_id": "greenpulse_01",
        "sequence_number": seq,
        "soil_moisture": round(random.uniform(48, 65), 1),
        "temperature": round(random.uniform(24, 29), 1),
        "humidity": round(random.uniform(52, 68), 1),
        "light_intensity": round(random.uniform(12000, 30000), 1),
        "soil_temperature": round(random.uniform(21, 26), 1),
        "soil_temperature_ok": True,
    }

    if scenario == "dry":
        reading["soil_moisture"] = round(random.uniform(18, 28), 1)
    elif scenario == "hot":
        reading["temperature"] = round(random.uniform(36, 39), 1)
        reading["humidity"] = round(random.uniform(30, 34), 1)
    elif scenario == "dark":
        reading["light_intensity"] = round(random.uniform(300, 1500), 1)
    elif scenario == "invalid":
        reading["soil_moisture"] = "wet"
        reading["temperature"] = 150
        reading["humidity"] = None
        reading["light_intensity"] = -5
        reading["soil_temperature"] = None
        reading["soil_temperature_ok"] = False

    return reading


def main():
    parser = argparse.ArgumentParser(description="Send fake GreenPulse sensor readings to AWS IoT Core")
    parser.add_argument("--count", type=int, default=5)
    parser.add_argument("--interval", type=float, default=3)
    parser.add_argument("--scenario", choices=["normal", "dry", "hot", "dark", "invalid"], default="normal")
    args = parser.parse_args()

    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id="greenpulse_tester")
    client.tls_set(ca_certs=config.ROOT_CA_PATH, certfile=config.CERT_PATH, keyfile=config.KEY_PATH)
    client.connect(config.IOT_ENDPOINT, config.IOT_PORT, keepalive=30)
    client.loop_start()

    for _ in range(20):
        if client.is_connected():
            break
        time.sleep(0.5)
    if not client.is_connected():
        print("Could not connect to AWS IoT Core, check the endpoint, certificates and policy")
        client.loop_stop()
        return

    seq = random.randint(100000, 900000)
    for i in range(args.count):
        reading = make_reading(seq + i, args.scenario)
        info = client.publish(config.TOPIC_SENSORS, json.dumps(reading), qos=1)
        info.wait_for_publish(timeout=5)
        print("Sent:", json.dumps(reading))
        time.sleep(args.interval)

    client.loop_stop()
    client.disconnect()


if __name__ == "__main__":
    main()
