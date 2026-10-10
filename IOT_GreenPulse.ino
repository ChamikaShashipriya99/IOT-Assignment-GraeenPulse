#include <Arduino.h>
#include <esp_timer.h>
#include <driver/gpio.h>
#include <Wire.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <time.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <BH1750.h>
#include <ArduinoJson.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <LittleFS.h>

// ========== WIFI SETTINGS ==========
const char* WIFI_SSID = "Chamika";
const char* WIFI_PASSWORD = "1234567890";

// ========== AWS MQTT SETTINGS ==========
// දැන් මෙය true ලෙස වෙනස් කර ඇත.
const bool ENABLE_MQTT = true;

// Hostname only: no https:// prefix.
const char* MQTT_HOST = "a1c256s0jz89b5-ats.iot.us-east-1.amazonaws.com";
const int MQTT_PORT = 8883;
const char* MQTT_CLIENT_ID = "greenpulse_01";

// ========== CERTIFICATES ==========
const char* ROOT_CA = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF\n" \
"ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6\n" \
"b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL\n" \
"MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv\n" \
"b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj\n" \
"ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM\n" \
"9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw\n" \
"IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6\n" \
"VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L\n" \
"93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm\n" \
"jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC\n" \
"AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA\n" \
"A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI\n" \
"U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs\n" \
"N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv\n" \
"o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU\n" \
"5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy\n" \
"rqXRfboQnoZsG4q5WTP468SQvvG5\n" \
"-----END CERTIFICATE-----\n" ;

const char* DEVICE_CERT = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDWTCCAkGgAwIBAgIURAieTFQVr3QiahV5IFkPAAu/4mAwDQYJKoZIhvcNAQEL\n" \
"BQAwTTFLMEkGA1UECwxCQW1hem9uIFdlYiBTZXJ2aWNlcyBPPUFtYXpvbi5jb20g\n" \
"SW5jLiBMPVNlYXR0bGUgU1Q9V2FzaGluZ3RvbiBDPVVTMB4XDTI2MTAwMjA1MjQx\n" \
"NVoXDTQ5MTIzMTIzNTk1OVowHjEcMBoGA1UEAwwTQVdTIElvVCBDZXJ0aWZpY2F0\n" \
"ZTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBANBpRSE5TZWH/+jc3tb4\n" \
"pKDijH3563wWWQP7FJYfJwdUXXoy5jHTfwZRTmCJWprkg3o8vLwRYP5zXpoeVocp\n" \
"V5kNfNRO3UCWSnYE/yU4XkxjLvP0exc5a0mXlc6x2AfYFX481vK12bSYmuhIrFI5\n" \
"4/DTZczXrcS6Lsdg878FyYtV4v/oLlAAl2PSH1ywtzhGIMQVxtkDuzOYskCwnNIU\n" \
"h6j4clzaONKYfUQZi/kSwbHvfwmHsR0xgzQGTGFyFUryN1s6M4IxwfX1ZLCoxdZY\n" \
"rU0adjU6E8UPsINZCbIKWebMfW9qwFJ/pxXQjKPbR+RjxjvVRgRw4Uhyy43teoJk\n" \
"/pMCAwEAAaNgMF4wHwYDVR0jBBgwFoAUoRLlRit4kFdN4T72m3rh+cXynSIwHQYD\n" \
"VR0OBBYEFM1XNM2p45dwlacZfgs5jFmaDnvHMAwGA1UdEwEB/wQCMAAwDgYDVR0P\n" \
"AQH/BAQDAgeAMA0GCSqGSIb3DQEBCwUAA4IBAQAwBtcu67rF1jQ19KSdwbaK1HR0\n" \
"LTMizH6sU3IUGXBAT/UR5Bub+O0VkF2cpSiIalVE865oWe3JxY/9eGvRZ1Y1W/3+\n" \
"EiU/s5NHQ8F2JbpKsd1ciAS4xT60nDdU3OuwRShuZsyy/Opg5riBaE0lyuFVipKV\n" \
"kdJg/1anpCNjIB1rv67/BPZh/7ImC3S+PJbBDEOfzzuisxofj2Ub0G63+LCmKPNH\n" \
"NRR150MJHzJ02TU92z4HEuq8we1jjHE8mk1jeJXX95mA8CG9jgWuWXevuTk18nNv\n" \
"ZGY6AHFSBB7WSvBEU1CDnWxELbVsAUadBdVnFnhvqyRB3C0IZRqlLM5SeLPq\n" \
"-----END CERTIFICATE-----\n";

const char* PRIVATE_KEY = \
"-----BEGIN RSA PRIVATE KEY-----\n" \
"MIIEpAIBAAKCAQEA0GlFITlNlYf/6Nze1vikoOKMffnrfBZZA/sUlh8nB1RdejLm\n" \
"MdN/BlFOYIlamuSDejy8vBFg/nNemh5WhylXmQ181E7dQJZKdgT/JTheTGMu8/R7\n" \
"FzlrSZeVzrHYB9gVfjzW8rXZtJia6EisUjnj8NNlzNetxLoux2DzvwXJi1Xi/+gu\n" \
"UACXY9IfXLC3OEYgxBXG2QO7M5iyQLCc0hSHqPhyXNo40ph9RBmL+RLBse9/CYex\n" \
"HTGDNAZMYXIVSvI3WzozgjHB9fVksKjF1litTRp2NToTxQ+wg1kJsgpZ5sx9b2rA\n" \
"Un+nFdCMo9tH5GPGO9VGBHDhSHLLje16gmT+kwIDAQABAoIBACiwVMHOyTnnamwR\n" \
"KyR3ONT8RgeWUoZQaqKfU36XqGwC6Zkg9NZHf2ZBW3b3egwBjzW/Q+3HytMCvTaD\n" \
"s8sBpEuKWZOtrf6dGP5/rTycC7UILDOOyLVZDXw5rxLCn/WwF0olIKYUIOPkY2H0\n" \
"RkhravgqnZZniVJ+MwHUCOKDg7zC9q/KvAiW4f+cYXwrxRgA0KEhaFQv5i0B8KCb\n" \
"8E9pdrXlfWV8avu5NZKq4Fv0BbulUurzJa6R8ZA+2lycp4NIibex6ttIfif5fIjZ\n" \
"ulcg0XKVejYaKu2sUq/HQP8hoSenMJF1QvxmVQJoErrZS5XdQYwb2jf09jgP07xi\n" \
"baWi8LECgYEA65U+sgHf4i3Ss8jQCVi/akYa3ScrB+/I0ZkCuarFis13izP25Ukk\n" \
"LuubxAJ3p2GL36dEja2dEUzrq97unOtfqo6kCSDleqypsd8cD66BohWpc/L/XUz3\n" \
"YkAwggyso0fQlGk+kTIYhbjNNxnzscoszFnh7q7HxmpAhdjVQ4YD0qkCgYEA4nkt\n" \
"5pmosHBu14I+5ATSlwA68t3MkXk7BtbELXm+yfhpKvd473R7doD6aMT/tVdZyZhI\n" \
"NTA+G8kLP7PkfFDs/DzwEdOTW4E+pKqRta2mLFdqWa5nbfvmwlh84ydsoiraDXcj\n" \
"3zjdtV3Kq1VxklFW5nzc8iOOsO0UzglS/0NGiNsCgYEAgEhRzo7Uwg4fyUSVfDF2\n" \
"ckFgiYK1nOnGmdPPNxk13qKJ8SRH0o0kheRIetC5JU0p5Izp+JhMikovnvSTTKGj\n" \
"A3YO/uWJ8GYrNa9/UU6+SmuvTXNJ+R1bLIY1o+uQ6ozFmLtClcAIuHBsVw/rsqmc\n" \
"AV8Ed3t3T6U2I2xynQVR+ZkCgYATdlZ7erkEcKUV4BuHfOKcF2j5dhYeakyoScyS\n" \
"G+RJdw+SobHC4j+571uVpVhUPS16JZwO9hZgTOaqkssP4+R5yMLYxVXkhzba782V\n" \
"z1Z4eQqqqlI5IWyzRud0ne64APa8MYDjrf9flq/UCYO5D0RoilJyfJvGmJkxG5wG\n" \
"U8G8AQKBgQDR9565mAIfQqEzsK2jdic0BlhhFs9AhNPFPgOuxKH57FwiBwPaEsz6\n" \
"P1Fni43BgZY7EhIGZAh+ut4B16850pBQL3+W1Gk1JsjlfxkesU2eti41QV+wBOQ9\n" \
"TXyC8jNES1Ef1Oa2hj9I4m2nYJZfwJJd7aJMifoW5MGgRdL9oKznkA==\n" \
"-----END RSA PRIVATE KEY-----\n";

// ========== SENSOR PINS ==========
#define DHT_PIN 4
#define DHT_TYPE DHT22

const int SOIL_PIN = 34;
const int SOIL_TEMP_PIN = 32;  // DS18B20 DQ, with 4.7k to 3V3
const uint8_t LIGHT_ADDRESS = 0x23;

// RGB 1: plant conditions
const int PLANT_R = 25;
const int PLANT_G = 26;
const int PLANT_B = 27;

// RGB 2: connectivity
const int NET_R = 18;
const int NET_G = 19;
const int NET_B = 23;

// ========== SOIL CALIBRATION ==========
// Calibrated raw ADC endpoints: dry soil and watered-and-drained soil.
const int SOIL_DRY_RAW = 4000;
const int SOIL_WET_RAW = 1500;

// Real-world thresholds for a chili plant.
const float SOIL_CRITICAL_BELOW = 30.0;
const float SOIL_WARNING_BELOW = 45.0;
const float SOIL_WARNING_OVERWET = 80.0;

const float TEMP_NORMAL_MIN = 22.0;
const float TEMP_NORMAL_MAX = 30.0;
const float TEMP_WARNING_MIN = 18.0;
const float TEMP_WARNING_MAX = 35.0;

const float HUM_NORMAL_MIN = 50.0;
const float HUM_NORMAL_MAX = 70.0;
const float HUM_WARNING_MIN = 35.0;
const float HUM_WARNING_MAX = 80.0;

const float SOIL_TEMP_NORMAL_MIN = 20.0;
const float SOIL_TEMP_NORMAL_MAX = 28.0;
const float SOIL_TEMP_WARNING_MIN = 15.0;
const float SOIL_TEMP_WARNING_MAX = 32.0;

const float LIGHT_VERY_LOW_CRITICAL = 2000.0;
const float LIGHT_LOW_WARNING = 5000.0;
const float LIGHT_NORMAL_MIN = 10000.0;
const float LIGHT_NORMAL_MAX = 40000.0;

const int NORMAL = 0;
const int WARNING = 1;
const int CRITICAL = 2;

// ========== AUTOMATIC WATERING ==========
// GPIO33 drives module TRIG/PWM; power the pump from the external 5 V supply.
const int PUMP_PIN = 33;
const int PUMP_ON_LEVEL = HIGH;   // Swap ON/OFF levels if the module trigger is active-low.
const int PUMP_OFF_LEVEL = LOW;
const unsigned long PUMP_RUN_MS = 20000UL;
const unsigned long PUMP_SOAK_MS = 60000UL;
const int DRY_CONFIRMATION_SAMPLES = 3;
const int MAX_PULSES_PER_DRY_EPISODE = 3;

int latestSoilRaw = 0;
float latestSoilPercent = NAN;
bool latestSoilSampleReady = false;
bool newSoilSample = false;
bool pumpRunning = false;
bool pumpTimerReady = false;
int dryConfirmationCount = 0;
int pulsesThisDryEpisode = 0;
unsigned long pumpStartedAt = 0;
unsigned long pumpStoppedAt = 0;
const char* wateringStatus = "WAIT SOIL SAMPLE";
esp_timer_handle_t pumpSafetyTimer = nullptr;
portMUX_TYPE pumpMux = portMUX_INITIALIZER_UNLOCKED;
int64_t pumpDeadlineUs = 0;

void pumpSafetyCallback(void*) {
  portENTER_CRITICAL(&pumpMux);
  if (pumpDeadlineUs != 0 && esp_timer_get_time() >= pumpDeadlineUs) {
    gpio_set_level((gpio_num_t)PUMP_PIN, PUMP_OFF_LEVEL);
    pumpDeadlineUs = 0;
  }
  portEXIT_CRITICAL(&pumpMux);
}

// ========== DEVICES ==========
DHT dht(DHT_PIN, DHT_TYPE);
OneWire soilTempWire(SOIL_TEMP_PIN);
DallasTemperature soilTempSensor(&soilTempWire);
DeviceAddress soilTempAddress;
bool soilTempFound = false;
bool soilTempPending = false;
bool soilTempValid = false;
float soilTemperatureC = NAN;
unsigned long soilTempRequestedAt = 0;
unsigned long soilTempLastAttempt = 0;
unsigned long soilTempLastGood = 0;
const unsigned long SOIL_TEMP_INTERVAL_MS = 2500UL;
const unsigned long SOIL_TEMP_CONVERSION_MS = 800UL;
const unsigned long SOIL_TEMP_MAX_AGE_MS = 6000UL;
BH1750 lightMeter;
Adafruit_SSD1306 display(128, 64, &Wire, -1);

WiFiClientSecure secureClient;
PubSubClient mqtt(secureClient);

bool oledReady = false;
bool lightReady = false;
bool mqttConfigured = false;

bool wifiWasConnected = false;
bool wifiEverConnected = false;
bool mqttAttempted = false;
bool mqttEverConnected = false;

unsigned long wifiStartedAt = 0;
unsigned long lastWiFiRetry = 0;
unsigned long lastMQTTAttempt = 0;
unsigned long lastSensorRead = 0;
unsigned long lightStartedAt = 0;
unsigned long sequenceNumber = 0;
const unsigned long OLED_SCREEN_INTERVAL_MS = 4000UL;
const int OLED_SCREEN_COUNT = 3;
int oledScreen = 0;
unsigned long lastOLEDScreenChange = 0;
const char* OFFLINE_QUEUE_PATH = "/offline_queue.jsonl";
const size_t OFFLINE_QUEUE_MAX_BYTES = 64 * 1024;
const size_t OFFLINE_BATCH_CAPACITY = 4096;
const unsigned long OFFLINE_BATCH_FLUSH_INTERVAL_MS = 15000UL;
const unsigned long OFFLINE_REPLAY_INTERVAL_MS = 250UL;
bool offlineQueueReady = false;
bool offlineQueuePending = false;
size_t offlineReplayOffset = 0;
char offlineBatch[OFFLINE_BATCH_CAPACITY];
size_t offlineBatchLength = 0;
unsigned long lastOfflineFlush = 0;
unsigned long lastOfflineReplay = 0;

// ========== LED HELPERS ==========
// Both modules must be common-cathode.
void setPlantRGB(bool r, bool g, bool b) {
  digitalWrite(PLANT_R, r ? HIGH : LOW);
  digitalWrite(PLANT_G, g ? HIGH : LOW);
  digitalWrite(PLANT_B, b ? HIGH : LOW);
}

void setNetworkRGB(bool r, bool g, bool b) {
  digitalWrite(NET_R, r ? HIGH : LOW);
  digitalWrite(NET_G, g ? HIGH : LOW);
  digitalWrite(NET_B, b ? HIGH : LOW);
}

void updatePlantLED(int status) {
  if (status == CRITICAL) {
    setPlantRGB(true, false, false);
  } else if (status == WARNING) {
    setPlantRGB(true, true, false);
  } else {
    setPlantRGB(false, true, false);
  }
}

// ========== I2C / LIGHT SENSOR ==========
bool deviceResponds(uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

bool startLightSensor() {
  if (!deviceResponds(LIGHT_ADDRESS)) {
    return false;
  }

  bool ok = lightMeter.begin(
    BH1750::CONTINUOUS_HIGH_RES_MODE,
    LIGHT_ADDRESS,
    &Wire
  );

  if (ok) {
    lightStartedAt = millis();
  }

  return ok;
}

// ========== DS18B20 SOIL TEMPERATURE ==========
// One externally powered, three-wire DS18B20 on GPIO32.
// Conversion runs asynchronously so its 750 ms wait does not block the loop.
void startSoilTemperatureConversion() {
  soilTempLastAttempt = millis();
  if (!soilTempFound) {
    soilTempSensor.begin();
    soilTempFound = soilTempSensor.getAddress(soilTempAddress, 0) &&
                    soilTempAddress[0] == 0x28;
    if (!soilTempFound) {
      soilTempValid = false;
      soilTemperatureC = NAN;
      return;
    }
    soilTempSensor.setResolution(soilTempAddress, 12);
    soilTempSensor.setWaitForConversion(false);
  }
  if (!soilTempSensor.requestTemperaturesByAddress(soilTempAddress)) {
    soilTempFound = false;
    soilTempValid = false;
    soilTemperatureC = NAN;
    return;
  }
  soilTempRequestedAt = millis();
  soilTempPending = true;
}

void serviceSoilTemperature() {
  unsigned long now = millis();
  if (soilTempPending &&
      now - soilTempRequestedAt >= SOIL_TEMP_CONVERSION_MS) {
    float value = soilTempSensor.getTempC(soilTempAddress);
    soilTempPending = false;
    // -127 and newer library error codes are outside the allowed range.
    // For this plant monitor, reject 85 C: it may be the power-up default.
    soilTempValid = !isnan(value) && value >= -55.0f &&
                    value <= 125.0f && value != 85.0f;
    soilTemperatureC = soilTempValid ? value : NAN;
    if (soilTempValid) {
      soilTempLastGood = millis();
    } else {
      soilTempFound = false;  // Rediscover on the next attempt.
    }
  }
  if (!soilTempPending &&
      millis() - soilTempLastAttempt >= SOIL_TEMP_INTERVAL_MS) {
    startSoilTemperatureConversion();
  }
}

// ========== SOIL ==========
int readSoilRaw() {
  long sum = 0;

  for (int i = 0; i < 10; i++) {
    sum += analogRead(SOIL_PIN);
    delay(2);
  }

  return sum / 10;
}

bool soilCalibrationReady() {
  return SOIL_DRY_RAW > 20 && SOIL_DRY_RAW < 4075 &&
         SOIL_WET_RAW > 20 && SOIL_WET_RAW < 4075 &&
         abs(SOIL_DRY_RAW - SOIL_WET_RAW) >= 300;
}

float soilPercentage(int raw) {
  if (!soilCalibrationReady()) {
    return NAN;
  }

  float value = 100.0f * (raw - SOIL_DRY_RAW) /
                (SOIL_WET_RAW - SOIL_DRY_RAW);

  return constrain(value, 0.0f, 100.0f);
}

bool soilRawIsValid(int raw) {
  if (raw <= 20 || raw > 4095) return false;
  if (raw < 4075) return true;

  // Accept readings near the calibrated dry endpoint at the ADC high rail.
  // For this calibration, 4095 maps to 0% rather than being discarded.
  if (SOIL_DRY_RAW <= SOIL_WET_RAW) return false;
  int dryRailTolerance = abs(SOIL_DRY_RAW - SOIL_WET_RAW) / 20;
  if (dryRailTolerance < 100) dryRailTolerance = 100;
  return raw >= SOIL_DRY_RAW &&
         raw - SOIL_DRY_RAW <= dryRailTolerance;
}

void startPumpPulse() {
  if (!pumpTimerReady || pumpRunning) return;

  pumpStartedAt = millis();
  pumpRunning = true;
  pulsesThisDryEpisode++;
  portENTER_CRITICAL(&pumpMux);
  pumpDeadlineUs = esp_timer_get_time() + (int64_t)PUMP_RUN_MS * 1000;
  gpio_set_level((gpio_num_t)PUMP_PIN, PUMP_ON_LEVEL);
  portEXIT_CRITICAL(&pumpMux);

  wateringStatus = "PUMP ON 20s";
  Serial.println("Watering: pump ON for 20 seconds.");
}

void stopPumpPulse() {
  portENTER_CRITICAL(&pumpMux);
  pumpDeadlineUs = 0;
  gpio_set_level((gpio_num_t)PUMP_PIN, PUMP_OFF_LEVEL);
  portEXIT_CRITICAL(&pumpMux);

  pumpRunning = false;
  pumpStoppedAt = millis();
  wateringStatus = "SOAK 60s";
  Serial.println("Watering: pump OFF; waiting 60 seconds before recheck.");
}

void serviceAutomaticWatering() {
  if (pumpRunning && millis() - pumpStartedAt >= PUMP_RUN_MS) {
    stopPumpPulse();
  }

  if (!newSoilSample) return;
  newSoilSample = false;

  if (pumpRunning) {
    wateringStatus = "PUMP ON 20s";
    return;
  }
  if (!pumpTimerReady) {
    wateringStatus = "PUMP TIMER ERROR";
    return;
  }
  if (!soilCalibrationReady()) {
    dryConfirmationCount = 0;
    wateringStatus = "CALIBRATE SOIL";
    return;
  }
  if (!latestSoilSampleReady || !soilRawIsValid(latestSoilRaw)) {
    dryConfirmationCount = 0;
    wateringStatus = "SOIL SENSOR ERROR";
    return;
  }
  if (latestSoilPercent >= SOIL_CRITICAL_BELOW) {
    dryConfirmationCount = 0;
    pulsesThisDryEpisode = 0;
    wateringStatus = "SOIL OK";
    return;
  }
  if (pulsesThisDryEpisode >= MAX_PULSES_PER_DRY_EPISODE) {
    wateringStatus = "PULSE LIMIT";
    return;
  }
  if (pumpStoppedAt != 0 && millis() - pumpStoppedAt < PUMP_SOAK_MS) {
    wateringStatus = "SOAK 60s";
    return;
  }

  dryConfirmationCount++;
  if (dryConfirmationCount < DRY_CONFIRMATION_SAMPLES) {
    wateringStatus = "CONFIRMING DRY";
    return;
  }

  dryConfirmationCount = 0;
  startPumpPulse();
}

int evaluateSoil(float moisture) {
  if (moisture < SOIL_CRITICAL_BELOW || moisture > SOIL_WARNING_OVERWET) return CRITICAL;
  if (moisture < SOIL_WARNING_BELOW || moisture > 70.0f) return WARNING;
  return NORMAL;
}

int evaluateAir(float temperature, float humidity) {
  if (temperature < TEMP_WARNING_MIN ||
      temperature > TEMP_WARNING_MAX ||
      humidity < HUM_WARNING_MIN ||
      humidity > HUM_WARNING_MAX) {
    return CRITICAL;
  }

  if (temperature < TEMP_NORMAL_MIN ||
      temperature > TEMP_NORMAL_MAX ||
      humidity < HUM_NORMAL_MIN ||
      humidity > HUM_NORMAL_MAX) {
    return WARNING;
  }

  return NORMAL;
}

int evaluateSoilTemperature(float soilTempC) {
  if (isnan(soilTempC)) return CRITICAL;
  if (soilTempC < SOIL_TEMP_WARNING_MIN ||
      soilTempC > SOIL_TEMP_WARNING_MAX) {
    return CRITICAL;
  }

  if (soilTempC < SOIL_TEMP_NORMAL_MIN ||
      soilTempC > SOIL_TEMP_NORMAL_MAX) {
    return WARNING;
  }

  return NORMAL;
}

int evaluateLight(float lux) {
  if (lux < LIGHT_VERY_LOW_CRITICAL) return CRITICAL;
  if (lux < LIGHT_LOW_WARNING) return WARNING;
  if (lux < LIGHT_NORMAL_MIN) return WARNING;
  if (lux <= LIGHT_NORMAL_MAX) return NORMAL;
  return WARNING; // Very bright; monitor only.
}

// ========== CONNECTION STATUS ==========
const char* networkText() {
  if (WiFi.status() != WL_CONNECTED) {
    return "W:OFF";
  }

  if (!mqttConfigured) {
    return "W:ON";
  }

  return mqtt.connected() ? "M:ON" : "M:OFF";
}

void showStartupStatus(const char* statusText) {
  if (!oledReady) {
    return;
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setTextWrap(false);
  display.setCursor(0, 0);
  display.println("GreenPulse");
  display.setCursor(0, 24);
  display.println(statusText);
  display.display();
}

void updateNetworkLED() {
  if (WiFi.status() != WL_CONNECTED) {
    bool initialAttempt =
      !wifiEverConnected &&
      millis() - wifiStartedAt < 30000UL;

    if (initialAttempt) {
      bool blink = (millis() / 500UL) % 2;
      setNetworkRGB(false, false, blink);
    } else {
      setNetworkRGB(true, false, false);
    }

    return;
  }

  if (!mqttConfigured) {
    setNetworkRGB(false, false, true);
  } else if (mqtt.connected()) {
    setNetworkRGB(false, true, false);
  } else if (mqttAttempted || mqttEverConnected) {
    setNetworkRGB(true, true, false);
  } else {
    setNetworkRGB(false, false, true);
  }
}

// Print incoming AI messages using the supplied payload length.
void onMQTTMessage(char* topic, byte* payload, unsigned int length) {
  Serial.print("MQTT received [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.write(payload, length);
  Serial.println();
}

bool flushOfflineBatch() {
  if (!offlineQueueReady || offlineBatchLength == 0) {
    return offlineBatchLength == 0;
  }

  File queueFile = LittleFS.open(OFFLINE_QUEUE_PATH, FILE_APPEND);
  if (!queueFile) {
    Serial.println("Could not open offline queue for writing.");
    return false;
  }

  size_t queueSize = queueFile.size();
  if (queueSize + offlineBatchLength > OFFLINE_QUEUE_MAX_BYTES) {
    queueFile.close();
    offlineBatchLength = 0;
    Serial.println("Offline queue full; dropping newest buffered readings.");
    return false;
  }

  size_t bytesWritten = queueFile.write(
    reinterpret_cast<const uint8_t*>(offlineBatch), offlineBatchLength
  );
  queueFile.close();

  if (bytesWritten != offlineBatchLength) {
    offlineBatchLength = 0;
    Serial.println("Offline queue write incomplete; buffered readings lost.");
    return false;
  }

  offlineBatchLength = 0;
  offlineQueuePending = true;
  lastOfflineFlush = millis();
  return true;
}

bool queueOfflinePayload(const char* payload) {
  if (!offlineQueueReady) {
    return false;
  }

  size_t payloadLength = strlen(payload);
  size_t recordLength = payloadLength + 1;
  if (recordLength > OFFLINE_BATCH_CAPACITY) {
    Serial.println("Sensor payload too large for offline queue.");
    return false;
  }

  if (offlineBatchLength + recordLength > OFFLINE_BATCH_CAPACITY) {
    if (!flushOfflineBatch()) {
      return false;
    }
  }

  memcpy(offlineBatch + offlineBatchLength, payload, payloadLength);
  offlineBatchLength += payloadLength;
  offlineBatch[offlineBatchLength++] = '\n';
  return true;
}

void serviceOfflineQueue() {
  if (!offlineQueueReady || !mqtt.connected()) {
    return;
  }

  if (offlineBatchLength > 0 &&
      (!offlineQueuePending ||
       millis() - lastOfflineFlush >= OFFLINE_BATCH_FLUSH_INTERVAL_MS)) {
    if (!flushOfflineBatch()) {
      return;
    }
  }

  if (!offlineQueuePending ||
      millis() - lastOfflineReplay < OFFLINE_REPLAY_INTERVAL_MS) {
    return;
  }
  lastOfflineReplay = millis();

  File queueFile = LittleFS.open(OFFLINE_QUEUE_PATH, FILE_READ);
  if (!queueFile) {
    Serial.println("Could not open offline queue for replay.");
    return;
  }

  size_t queueSize = queueFile.size();
  if (offlineReplayOffset >= queueSize) {
    queueFile.close();
    LittleFS.remove(OFFLINE_QUEUE_PATH);
    offlineQueuePending = false;
    offlineReplayOffset = 0;
    Serial.println("Offline queue replay complete.");
    return;
  }

  if (!queueFile.seek(offlineReplayOffset)) {
    queueFile.close();
    Serial.println("Could not seek in offline queue.");
    return;
  }

  String queuedPayload = queueFile.readStringUntil('\n');
  size_t nextOffset = queueFile.position();
  queueFile.close();

  if (queuedPayload.length() == 0 || queuedPayload.length() >= 512) {
    Serial.println("Invalid offline queue record; skipping it.");
    offlineReplayOffset = nextOffset;
    return;
  }

  StaticJsonDocument<512> queuedDocument;
  if (deserializeJson(queuedDocument, queuedPayload) ||
      !queuedDocument.is<JsonObject>()) {
    Serial.println("Malformed offline JSON record; skipping it.");
    offlineReplayOffset = nextOffset;
    return;
  }

  if (!mqtt.publish("greenpulse/sensors", queuedPayload.c_str())) {
    Serial.println("Offline queue publish failed; will retry.");
    return;
  }

  offlineReplayOffset = nextOffset;
  Serial.println("Replayed one offline sensor reading.");
}

void serviceNetwork() {
  unsigned long now = millis();
  bool wifiConnected = WiFi.status() == WL_CONNECTED;

  if (!wifiConnected) {
    if (wifiWasConnected) {
      Serial.println("Wi-Fi connection lost.");
      secureClient.stop();
      wifiWasConnected = false;
    }

    if (now - lastWiFiRetry >= 15000UL) {
      lastWiFiRetry = now;
      WiFi.reconnect();
    }

    updateNetworkLED();
    return;
  }

  if (!wifiWasConnected) {
    wifiWasConnected = true;
    wifiEverConnected = true;

    Serial.print("Wi-Fi connected. IP: ");
    Serial.println(WiFi.localIP());

    if (mqttConfigured) {
      // TLS certificate checks require a valid clock.
      configTime(0, 0, "pool.ntp.org", "time.nist.gov");
      Serial.println("Waiting for network time for TLS.");
    }
  }

  if (!mqttConfigured) {
    updateNetworkLED();
    return;
  }

  if (mqtt.connected()) {
    mqtt.loop();
  }

  // Wait for time synchronization before TLS.
  bool clockReady = time(nullptr) > 1700000000;

  if (!mqtt.connected() && clockReady &&
      (!mqttAttempted ||
       now - lastMQTTAttempt >= 15000UL)) {

    lastMQTTAttempt = now;
    mqttAttempted = true;

    updateNetworkLED();
    Serial.println("Connecting to AWS MQTT...");

    // This call can briefly block during a connection attempt.
    if (mqtt.connect(MQTT_CLIENT_ID)) {
      mqttEverConnected = true;
      Serial.println("MQTT broker connected.");
      if (!mqtt.subscribe("greenpulse/ai/care")) {
        Serial.println("Failed to send care subscription.");
      }
      if (!mqtt.subscribe("greenpulse/ai/weather")) {
        Serial.println("Failed to send weather subscription.");
      }
      if (!mqtt.publish("greenpulse/status", "Device Online")) {
        Serial.println("Failed to publish online status.");
      }
    } else {
      Serial.print("MQTT connection failed. State: ");
      Serial.println(mqtt.state());
    }
  }

  serviceOfflineQueue();
  updateNetworkLED();
}

// ========== READ AND DISPLAY SENSORS ==========
void updateSensors() {
  bool soilTempOK = soilTempValid &&
    millis() - soilTempLastGood <= SOIL_TEMP_MAX_AGE_MS;
  int soilRaw = readSoilRaw();
  bool soilCalibrated = soilCalibrationReady();
  float soilPercent = soilPercentage(soilRaw);
  latestSoilRaw = soilRaw;
  latestSoilPercent = soilPercent;
  latestSoilSampleReady = soilCalibrated;
  newSoilSample = true;
  serviceAutomaticWatering();

  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  bool dhtOK =
    !isnan(humidity) && !isnan(temperature) &&
    humidity >= 0 && humidity <= 100 &&
    temperature >= -40 && temperature <= 80;

  if (!lightReady) {
    lightReady = startLightSensor();
  }

  float lux = -1;

  // Allow a measurement after (re)initialization.
  if (lightReady && millis() - lightStartedAt >= 200UL) {
    lux = lightMeter.readLightLevel();

    if (isnan(lux) || lux < 0) {
      lightReady = false;
    }
  }

  bool lightOK = !isnan(lux) && lux >= 0;

  int overallStatus = NORMAL;
  const char* message = "NORMAL";

  if (!dhtOK || !lightOK) {
    overallStatus = CRITICAL;

    if (!dhtOK && !lightOK) message = "SENSOR ERROR";
    else if (!dhtOK) message = "DHT22 ERROR";
    else message = "BH1750 ERROR";
  } else {
    int airStatus = evaluateAir(temperature, humidity);

    if (!soilCalibrated) {
      overallStatus = airStatus == CRITICAL ? CRITICAL : WARNING;
      message = airStatus == CRITICAL
                  ? "AIR CRITICAL" : "SOIL: CALIBRATE";
    } else {
      int soilStatus = evaluateSoil(soilPercent);
      overallStatus = soilStatus > airStatus
                        ? soilStatus : airStatus;

      if (soilStatus == CRITICAL && airStatus == CRITICAL) {
        message = "AIR + SOIL CRITICAL";
      } else if (soilStatus == CRITICAL) {
        message = "SOIL VERY DRY";
      } else if (airStatus == CRITICAL) {
        message = "AIR CRITICAL";
      } else if (soilStatus == WARNING && airStatus == WARNING) {
        message = "AIR + SOIL WARNING";
      } else if (soilStatus == WARNING) {
        message = "SOIL DRY";
      } else if (airStatus == WARNING) {
        message = "AIR WARNING";
      }
    }
  }

  // Check the soil temperature against chili-plant thresholds.
  int soilTempStatus = soilTempOK ? evaluateSoilTemperature(soilTemperatureC) : CRITICAL;
  if (soilTempStatus == CRITICAL) {
    overallStatus = CRITICAL;
    message = (!dhtOK || !lightOK) ? "SENSOR ERROR" : "SOIL TEMP ERROR";
  } else if (soilTempStatus == WARNING) {
    if (overallStatus < WARNING) {
      overallStatus = WARNING;
      message = "SOIL TEMP WARNING";
    }
  }

  if (lightOK) {
    int lightStatus = evaluateLight(lux);
    if (lightStatus == CRITICAL) {
      overallStatus = CRITICAL;
      message = "VERY LOW LIGHT";
    } else if (lux < LIGHT_LOW_WARNING) {
      if (overallStatus < WARNING) {
        overallStatus = WARNING;
        message = "LOW LIGHT";
      }
    } else if (lux > LIGHT_NORMAL_MAX) {
      if (overallStatus < WARNING) {
        overallStatus = WARNING;
        message = "VERY HIGH LIGHT";
      }
    } else if (lux < LIGHT_NORMAL_MIN) {
      if (overallStatus < WARNING) {
        overallStatus = WARNING;
        message = "LIGHT WARNING";
      }
    }
  }

  const char* systemMessages[8];
  int systemMessageCount = 0;

  auto addSystemMessage = [&](const char* text) {
    if (systemMessageCount < 8) {
      systemMessages[systemMessageCount++] = text;
    }
  };

  if (!dhtOK) {
    addSystemMessage("CRIT: DHT SENSOR");
  } else {
    if (temperature < TEMP_WARNING_MIN || temperature > TEMP_WARNING_MAX) {
      addSystemMessage("CRIT: AIR TEMP");
    } else if (temperature < TEMP_NORMAL_MIN || temperature > TEMP_NORMAL_MAX) {
      addSystemMessage("WARN: AIR TEMP");
    }

    if (humidity < HUM_WARNING_MIN || humidity > HUM_WARNING_MAX) {
      addSystemMessage("CRIT: HUMIDITY");
    } else if (humidity < HUM_NORMAL_MIN || humidity > HUM_NORMAL_MAX) {
      addSystemMessage("WARN: HUMIDITY");
    }
  }

  if (!soilCalibrated) {
    addSystemMessage("WARN: SOIL CALIBRATE");
  } else if (soilPercent < SOIL_CRITICAL_BELOW) {
    addSystemMessage("CRIT: SOIL TOO DRY");
  } else if (soilPercent > SOIL_WARNING_OVERWET) {
    addSystemMessage("CRIT: SOIL TOO WET");
  } else if (soilPercent < SOIL_WARNING_BELOW) {
    addSystemMessage("WARN: SOIL DRY");
  } else if (soilPercent > 70.0f) {
    addSystemMessage("WARN: SOIL WET");
  }

  if (!soilTempOK) {
    addSystemMessage("CRIT: SOIL TEMP ERR");
  } else if (soilTemperatureC < SOIL_TEMP_WARNING_MIN ||
             soilTemperatureC > SOIL_TEMP_WARNING_MAX) {
    addSystemMessage("CRIT: SOIL TEMP");
  } else if (soilTemperatureC < SOIL_TEMP_NORMAL_MIN ||
             soilTemperatureC > SOIL_TEMP_NORMAL_MAX) {
    addSystemMessage("WARN: SOIL TEMP");
  }

  if (!lightOK) {
    addSystemMessage("CRIT: LIGHT SENSOR");
  } else if (lux < LIGHT_VERY_LOW_CRITICAL) {
    addSystemMessage("CRIT: VERY LOW LIGHT");
  } else if (lux < LIGHT_NORMAL_MIN) {
    addSystemMessage("WARN: LOW LIGHT");
  } else if (lux > LIGHT_NORMAL_MAX) {
    addSystemMessage("WARN: HIGH LIGHT");
  }

  if (systemMessageCount == 0) {
    addSystemMessage("All readings normal");
  }

  updatePlantLED(overallStatus);

  Serial.print("Temp: ");
  if (dhtOK) Serial.print(temperature, 1);
  else Serial.print("ERROR");

  Serial.print(" C | Humidity: ");
  if (dhtOK) Serial.print(humidity, 1);
  else Serial.print("ERROR");

  Serial.print(" % | Light: ");
  if (lightOK) Serial.print(lux, 1);
  else Serial.print("ERROR");

  Serial.print(" lx | Soil raw: ");
  Serial.print(soilRaw);
  Serial.print(" | Soil: ");

  if (soilCalibrated) {
    Serial.print(soilPercent, 1);
    Serial.print("% relative");
  } else {
    Serial.print("Not calibrated");
  }

  Serial.print(" | Soil temp: ");
  if (soilTempOK) Serial.print(soilTemperatureC, 1);
  else Serial.print("ERROR");
  Serial.print(" C | ");
  Serial.print(message);
  Serial.print(" | ");
  Serial.print(networkText());
  Serial.print(" | Watering: ");
  Serial.println(wateringStatus);

  // JSON and Serial output continue even if the OLED is unavailable.
  if (oledReady) {
    if (millis() - lastOLEDScreenChange >= OLED_SCREEN_INTERVAL_MS) {
      lastOLEDScreenChange = millis();
      oledScreen = (oledScreen + 1) % OLED_SCREEN_COUNT;
    }

    display.clearDisplay();
    display.setTextSize(1);
    const char* screenTitle;
    if (oledScreen == 0) {
      screenTitle = "Network";
    } else if (oledScreen == 1) {
      screenTitle = "Sensor readings";
    } else {
      if (overallStatus == CRITICAL) screenTitle = "System: CRITICAL";
      else if (overallStatus == WARNING) screenTitle = "System: WARNING";
      else screenTitle = "System: NORMAL";
    }

    display.fillRect(0, 0, 128, 10, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
    int titleX = (128 - (strlen(screenTitle) * 6)) / 2;
    display.setCursor(titleX, 1);
    display.print(screenTitle);
    display.setTextColor(SSD1306_WHITE);

    if (oledScreen == 0) {
      display.setCursor(0, 12);
      display.print("WiFi: ");
      display.print(WiFi.status() == WL_CONNECTED ? "ON" : "OFF");

      display.setCursor(0, 26);
      display.print("MQTT: ");
      if (!mqttConfigured) display.print("DISABLED");
      else display.print(mqtt.connected() ? "ON" : "OFF");

      display.setCursor(0, 40);
      display.print("IP: ");
      if (WiFi.status() == WL_CONNECTED) display.print(WiFi.localIP());
      else display.print("--");

      display.setCursor(0, 54);
      display.print("RSSI: ");
      if (WiFi.status() == WL_CONNECTED) display.print(WiFi.RSSI());
      else display.print("--");
    } else if (oledScreen == 1) {
      display.setCursor(0, 12);
      display.print("Air temp: ");
      if (dhtOK) { display.print(temperature, 1); display.print("C"); }
      else display.print("ERR");

      display.setCursor(0, 22);
      display.print("Humidity: ");
      if (dhtOK) { display.print(humidity, 0); display.print("%"); }
      else display.print("ERR");

      display.setCursor(0, 32);
      display.print("Soil: ");
      if (soilCalibrated) {
        display.print(soilPercent, 0);
        display.print("% R:");
        display.print(soilRaw);
      } else {
        display.print("CAL ");
        display.print(soilRaw);
      }

      display.setCursor(0, 42);
      display.print("Soil temp: ");
      if (soilTempOK) { display.print(soilTemperatureC, 1); display.print("C"); }
      else display.print("ERR");

      display.setCursor(0, 52);
      display.print("Light: ");
      if (lightOK) { display.print(lux, 0); display.print(" lx"); }
      else display.print("ERR");
    } else {
      int firstMessage = 0;
      if (systemMessageCount > 5) {
        firstMessage = (millis() / 2000UL) % (systemMessageCount - 4);
      }

      for (int row = 0; row < 5; row++) {
        int messageIndex = firstMessage + row;
        if (messageIndex >= systemMessageCount) break;
        display.setCursor(0, 12 + row * 10);
        display.print(systemMessages[messageIndex]);
      }
    }

    display.display();
  }

  // ==========================================
  // BUILD AND PUBLISH THE JSON PAYLOAD
  // ==========================================
  
  StaticJsonDocument<512> doc;
  doc["device_id"] = MQTT_CLIENT_ID;
  doc["sequence_number"] = ++sequenceNumber;

  if (soilCalibrated) {
    doc["soil_moisture"] = soilPercent;
  }
  if (dhtOK) {
    doc["temperature"] = temperature;
    doc["humidity"] = humidity;
  }
  if (lightOK) {
    doc["light_intensity"] = lux;
  }

  // A missing temperature is null, never a false zero or -127 reading.
  if (soilTempOK) doc["soil_temperature"] = soilTemperatureC;
  else doc["soil_temperature"] = nullptr;
  doc["soil_temperature_ok"] = soilTempOK;
  doc["soil_calibrated"] = soilCalibrated;
  doc["watering_auto"] = soilCalibrated && pumpTimerReady;
  doc["watering_status"] = wateringStatus;
  doc["pump_command_on"] = pumpRunning;

  char jsonString[512];
  if (doc.overflowed() || measureJson(doc) >= sizeof(jsonString)) {
    Serial.println("Sensor JSON exceeds buffer capacity; skipping payload.");
    return;
  }
  serializeJson(doc, jsonString, sizeof(jsonString));

  Serial.println("\n--- Generated JSON Payload ---");
  Serial.println(jsonString);
  Serial.println("------------------------------\n");

  if (offlineQueuePending || offlineBatchLength > 0 || !mqtt.connected()) {
    if (!queueOfflinePayload(jsonString)) {
      Serial.println("Could not save sensor payload to offline queue.");
    }
  } else if (!mqtt.publish("greenpulse/sensors", jsonString)) {
    Serial.println("Failed to publish sensor payload; queueing it.");
    if (!queueOfflinePayload(jsonString)) {
      Serial.println("Could not save failed payload to offline queue.");
    }
  }
}

// ========== SETUP ==========
void setup() {
  Serial.begin(115200);

  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, PUMP_OFF_LEVEL);
  esp_timer_create_args_t pumpTimerArgs = {};
  pumpTimerArgs.callback = pumpSafetyCallback;
  pumpTimerArgs.name = "pump_cutoff";
  if (esp_timer_create(&pumpTimerArgs, &pumpSafetyTimer) != ESP_OK) {
    wateringStatus = "PUMP TIMER ERROR";
  } else if (esp_timer_start_periodic(pumpSafetyTimer, 10000) != ESP_OK) {
    esp_timer_delete(pumpSafetyTimer);
    pumpSafetyTimer = nullptr;
    wateringStatus = "PUMP TIMER ERROR";
  } else {
    pumpTimerReady = true;
  }

  Serial.println("Automatic watering: GPIO33; critical soil moisture; 20-second pump pulse.");
  Serial.println("Enter measured soil calibration values before automatic watering can run.");

  offlineQueueReady = LittleFS.begin(true);
  if (offlineQueueReady) {
    if (LittleFS.exists(OFFLINE_QUEUE_PATH)) {
      File queueFile = LittleFS.open(OFFLINE_QUEUE_PATH, FILE_READ);
      if (queueFile) {
        offlineQueuePending = queueFile.size() > 0;
        queueFile.close();
      }
    }
    Serial.println("Offline queue storage ready.");
  } else {
    Serial.println("LittleFS unavailable; offline readings will not be saved.");
  }
  lastOfflineFlush = millis();

  pinMode(PLANT_R, OUTPUT);
  pinMode(PLANT_G, OUTPUT);
  pinMode(PLANT_B, OUTPUT);
  pinMode(NET_R, OUTPUT);
  pinMode(NET_G, OUTPUT);
  pinMode(NET_B, OUTPUT);

  setPlantRGB(false, false, false);
  setNetworkRGB(false, false, false);

  pinMode(SOIL_PIN, INPUT);
  analogReadResolution(12);
  analogSetPinAttenuation(SOIL_PIN, ADC_11db);

  Wire.begin(21, 22);
  delay(200);

  uint8_t oledAddress = 0;
  if (deviceResponds(0x3C)) oledAddress = 0x3C;
  else if (deviceResponds(0x3D)) oledAddress = 0x3D;

  if (oledAddress != 0) {
    oledReady = display.begin(
      SSD1306_SWITCHCAPVCC, oledAddress, false, false
    );
  }

  if (oledReady) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setTextWrap(false);
    display.setCursor(0, 0);
    display.println("GreenPulse");
    display.setCursor(0, 24);
    display.println("Connecting WiFi...");
    display.display();
    lastOLEDScreenChange = millis();
  }

  dht.begin();
  startSoilTemperatureConversion();
  lightReady = startLightSensor();

  mqttConfigured =
    ENABLE_MQTT &&
    strlen(MQTT_HOST) > 0 &&
    strstr(ROOT_CA, "-----BEGIN CERTIFICATE-----") != nullptr &&
    strstr(DEVICE_CERT, "-----BEGIN CERTIFICATE-----") != nullptr &&
    strstr(PRIVATE_KEY, "PRIVATE KEY-----") != nullptr;

  if (mqttConfigured) {
    secureClient.setCACert(ROOT_CA);
    secureClient.setCertificate(DEVICE_CERT);
    secureClient.setPrivateKey(PRIVATE_KEY);
    secureClient.setHandshakeTimeout(5);

    // Allow room for the sensor JSON plus the MQTT topic and packet header.
    if (!mqtt.setBufferSize(1024)) {
      mqttConfigured = false;
      Serial.println("MQTT buffer allocation failed. Wi-Fi-only mode.");
    }
    mqtt.setCallback(onMQTTMessage);
    mqtt.setServer(MQTT_HOST, MQTT_PORT);
    mqtt.setKeepAlive(20);
    mqtt.setSocketTimeout(3);
  } else {
    Serial.println(
      "MQTT disabled or settings incomplete. Wi-Fi-only mode."
    );
  }

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  wifiStartedAt = millis();
  lastWiFiRetry = millis();
  lastSensorRead = millis();

  Serial.println("Connecting to Wi-Fi...");
}

// ========== LOOP ==========
void loop() {
  serviceAutomaticWatering();
  serviceSoilTemperature();
  serviceNetwork();
  serviceSoilTemperature();

  if (offlineBatchLength > 0 &&
      millis() - lastOfflineFlush >= OFFLINE_BATCH_FLUSH_INTERVAL_MS) {
    flushOfflineBatch();
  }

  // Timed sensor updates keep the connection LED responsive.
  if (millis() - lastSensorRead >= 2500UL) {
    lastSensorRead = millis();
    updateSensors();
    serviceAutomaticWatering();
  }

  delay(5);
}