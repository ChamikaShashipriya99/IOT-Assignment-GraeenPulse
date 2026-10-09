# GreenPulse Cloud Backend

Member 3 part: cloud backend and intelligent processing (Methsuka S A I).

The backend runs on a cloud server (AWS EC2), connects to AWS IoT Core over MQTT with TLS, saves the ESP32 sensor readings in DynamoDB, and uses an LLM (LangChain + OpenAI) to make care tips, quotes, a weather summary and an email notification summary. Results are published back to MQTT for the Node-RED dashboard and the ESP32.

## Flow

```
ESP32 --greenpulse/sensors--> AWS IoT Core --> backend
                                                 |-- validate JSON (validator.py)
                                                 |-- normal / warning / critical check (plant_rules.py)
                                                 |-- save to DynamoDB (database.py)
                                                 |-- OpenWeatherMap (weather.py)
                                                 |-- Gmail over IMAP, read only (email_reader.py)
                                                 |-- LangChain + OpenAI (llm_service.py)
backend --greenpulse/ai/care, ai/weather, ai/notifications--> AWS IoT Core --> Node-RED, ESP32
```

The MQTT callback only puts messages in a queue. The main loop in `main.py` takes them out, so slow LLM or API calls never block the MQTT connection.

## Files

| File | Purpose |
| --- | --- |
| `main.py` | Starts everything, handles messages, runs the timed jobs |
| `config.py` | Reads settings from `.env` |
| `mqtt_client.py` | AWS IoT Core connection, subscribe and publish |
| `validator.py` | Checks incoming sensor JSON |
| `plant_rules.py` | Threshold check (same chili plant values as the firmware) and fallback messages |
| `database.py` | DynamoDB save and history lookup |
| `create_table.py` | Creates the DynamoDB table and turns on TTL |
| `weather.py` | OpenWeatherMap current weather |
| `email_reader.py` | Reads plant related emails over IMAP |
| `llm_service.py` | Prompts, LLM calls and output checks |
| `payloads.py` | JSON payloads that get published |
| `send_test_reading.py` | Sends fake readings to test without the ESP32 |
| `aws/` | IoT and IAM policies |
| `greenpulse-backend.service` | systemd service for EC2 |
| `Dockerfile`, `docker-compose.yml` | Run the backend in Docker |
| `tests/` | pytest tests |

## MQTT topics

| Topic | Backend | QoS |
| --- | --- | --- |
| `greenpulse/sensors` | subscribes | 1 |
| `greenpulse/status` | subscribes (plain text or JSON) | 1 |
| `greenpulse/ai/care` | publishes every 5 min, or after 1 min if the priority changes | 1 |
| `greenpulse/ai/weather` | publishes every 15 min | 1 |
| `greenpulse/ai/notifications` | publishes every 15 min | 1 |

Care messages are only made when the latest reading is less than 5 minutes old, so old data is not shown as current.

All published payloads are kept under 900 bytes because the ESP32 MQTT buffer is 1024 bytes.

## Payloads

Input from the ESP32 (`greenpulse/sensors`). `soil_moisture` is missing while the sensor is not calibrated, and `soil_temperature` can be `null`.

```json
{"device_id": "greenpulse_01", "sequence_number": 125, "soil_moisture": 41.2, "temperature": 29.1,
 "humidity": 64.0, "light_intensity": 8200.0, "soil_temperature": 26.5, "soil_temperature_ok": true}
```

`greenpulse/ai/care` (`llm_input` is what was given to the LLM, for the dashboard):

```json
{"device_id": "greenpulse_01", "timestamp": "2026-10-09T16:30:00.123Z", "priority": "warning",
 "care_tip": "The soil is getting dry. Check the top layer and water slowly if it feels dry.",
 "literary_quote": "Dry earth waits patiently for a gentle hand.",
 "source": "llm",
 "llm_input": {"sequence_number": 125, "reading_time": "2026-10-09T16:29:58.410Z", "soil_moisture": 41.2,
               "temperature": 29.1, "humidity": 64.0, "light_intensity": 8200.0, "soil_temperature": 26.5,
               "weather": "light rain, 29.4 C"}}
```

`greenpulse/ai/weather`:

```json
{"device_id": "greenpulse_01", "timestamp": "2026-10-09T16:30:00.456Z", "available": true,
 "location": "Malabe", "temperature": 29.4, "humidity": 78, "condition": "light rain", "rain_1h_mm": 0.6,
 "observed_at": "2026-10-09T16:20:00.000Z",
 "summary": "Light rain and 29.4 C in Malabe.",
 "plant_advice": "Rain outside will not water a pot indoors, so still check the soil.",
 "source": "llm"}
```

`greenpulse/ai/notifications`:

```json
{"device_id": "greenpulse_01", "timestamp": "2026-10-09T16:30:01.002Z", "available": true,
 "emails_checked": 30, "relevant_count": 1,
 "summary": "The Garden Club reminds you to water the chili plants on Saturday.",
 "source": "llm"}
```

`source` is `llm` when the LLM answered, `fallback` when a rule based message was used instead, `rules` when no LLM call was needed (no relevant emails), and `none` when the data was not available. When weather or email cannot be read, `available` is `false` and only a short message is sent.

## DynamoDB table

Table `greenpulse_sensor_data`, on-demand billing.

| Field | Type | Notes |
| --- | --- | --- |
| `device_id` | String | partition key |
| `timestamp` | String | sort key, ISO 8601 UTC with milliseconds (backend time) |
| `sequence_number` | Number | from the ESP32 |
| `soil_moisture`, `temperature`, `humidity`, `light_intensity`, `soil_temperature` | Number | only stored when valid |
| `priority` | String | normal, warning or critical |
| `expires_at` | Number | TTL, readings are removed after 30 days |

Duplicate messages are skipped using the last 20 sequence numbers per device.

## Setup

### 1. AWS IoT Core

1. Create a thing called `greenpulse_backend` and a certificate for it.
2. Download the certificate, private key and `AmazonRootCA1.pem` into `backend/certs/`.
3. Create a policy from `aws/backend-iot-policy.json` (replace `ACCOUNT_ID`) and attach it to the certificate.
4. Copy the endpoint from AWS IoT Core > Settings.

### 2. DynamoDB

From a computer with AWS credentials that can create tables:

```bash
python create_table.py
```

### 3. EC2 server

1. Launch an Ubuntu EC2 instance (t2.micro or t3.micro is enough).
2. Create an IAM role with `aws/ec2-dynamodb-policy.json` and attach it to the instance. This way no AWS keys are stored on the server.
3. On the server:

```bash
sudo apt update && sudo apt install -y python3-venv git
git clone https://github.com/ChamikaShashipriya99/IOT-Assignment-GraeenPulse.git
cd IOT-Assignment-GraeenPulse/backend
python3 -m venv venv
venv/bin/pip install -r requirements.txt
cp .env.example .env
nano .env
```

4. Copy the files from `certs/` to the server (for example with `scp`).
5. Run it once to check: `venv/bin/python main.py`
6. Keep it running after logout or reboot:

```bash
sudo cp greenpulse-backend.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable --now greenpulse-backend
journalctl -u greenpulse-backend -f
```

Logs are also written to `logs/backend.log` (UTC timestamps).

### 3b. Run with Docker (instead of venv and systemd)

The image only has the code. `.env` and `certs/` are never copied into it, they are given to the container when it starts.

On the EC2 server (steps 1, 2 and 4 above are the same):

```bash
sudo apt update && sudo apt install -y docker.io docker-compose-v2 git
sudo usermod -aG docker ubuntu
```

Log out and in again, then:

```bash
cd IOT-Assignment-GraeenPulse/backend
cp .env.example .env
nano .env
docker compose up -d --build
docker compose logs -f
```

`restart: unless-stopped` starts the container again after a crash or a reboot. Stop it with `docker compose down`.

The container uses the host network, so it can get DynamoDB access from the EC2 IAM role and no AWS keys are needed. When running it on a laptop with Docker Desktop instead, either set `SAVE_TO_DYNAMODB=false` or add `AWS_ACCESS_KEY_ID` and `AWS_SECRET_ACCESS_KEY` for an IAM user with the same policy to `.env`.

Run the tests inside the container:

```bash
docker compose run --rm backend python -m pytest
```

### 4. Keys in `.env`

| Setting | Where to get it |
| --- | --- |
| `IOT_ENDPOINT` | AWS IoT Core > Settings |
| `OPENAI_API_KEY` | OpenAI platform account. `OPENAI_MODEL` can be changed to any chat model on the account |
| `WEATHER_API_KEY` | openweathermap.org (free plan). `WEATHER_LAT` and `WEATHER_LON` default to SLIIT Malabe |
| `EMAIL_USER`, `EMAIL_APP_PASSWORD` | A separate Gmail account made for the project, with 2-Step Verification on and an app password |

## Testing

Unit tests (no AWS, OpenAI, weather or email account needed, the LLM is replaced with a fake model):

```bash
pip install -r requirements.txt
python -m pytest
```

| Proposal test | Covered by |
| --- | --- |
| T09 cloud backend, valid and invalid payloads | `tests/test_validator.py`, `tests/test_main.py`, `send_test_reading.py --scenario invalid` |
| T10 data storage | `tests/test_main.py`, then check items in the DynamoDB console |
| T11 LLM processing | `tests/test_llm_service.py` |
| T12 weather API | `tests/test_weather.py` |
| T13 email integration | `tests/test_email_reader.py` |

Testing with AWS IoT Core but without the ESP32:

1. Attach `aws/tester-iot-policy.json` to the backend certificate (detach it again after testing).
2. Start the backend, then in another terminal:

```bash
python send_test_reading.py --scenario dry --count 3
python send_test_reading.py --scenario invalid --count 1
```

Scenarios: `normal`, `dry`, `hot`, `dark`, `invalid`.

3. Watch `greenpulse/ai/#` in the AWS IoT Core MQTT test client.

## When something fails

| Problem | What the backend does |
| --- | --- |
| Invalid sensor JSON or unknown device | Message is rejected and logged |
| One bad sensor value | That value is dropped, the rest is used |
| MQTT connection lost | paho reconnects automatically and subscribes again. Jobs wait until it is connected |
| DynamoDB error | Logged, processing continues |
| LLM error, bad JSON, missing field or a plant disease claim | Retries once, then publishes a rule based message with `source: fallback` |
| Weather API down | Uses the last data only while it is under 60 minutes old, otherwise publishes `available: false` |
| Email login fails | Publishes `available: false` |

## Security and privacy

- `.env` and `certs/` are in `.gitignore`. Keys and certificates never go to GitHub or into MQTT messages.
- The backend certificate can only connect as `greenpulse_backend`, read the sensor and status topics and publish to the three AI topics.
- The EC2 server uses an IAM role that only allows `PutItem` and `Query` on the one table.
- Email is opened read only (`EXAMINE` and `BODY.PEEK`), so nothing is marked as read or changed.
- For other emails only the subject line is checked. The body is read only when the subject matches plant, watering, gardening or weather words, at most 5 emails. Email addresses and links are removed before the text goes to the LLM, and only the summary is published.
- The weather API key is not written to the log even when a request fails.
