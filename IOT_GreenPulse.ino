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

// ========== SAMPLE SOIL CALIBRATION ==========
// DEMONSTRATION VALUES ONLY.
// Replace with actual dry and watered-and-drained readings.
const int SOIL_DRY_RAW = 4095;
const int SOIL_WET_RAW = 1500;
const bool SOIL_DEMO_CALIBRATION = true;

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
const int OLED_SCREEN_COUNT = 5;
int oledScreen = 0;
unsigned long lastOLEDScreenChange = 0;

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
  return SOIL_DRY_RAW >= 0 && SOIL_DRY_RAW <= 4095 &&
         SOIL_WET_RAW >= 0 && SOIL_WET_RAW <= 4095 &&
         SOIL_DRY_RAW != SOIL_WET_RAW;
}

float soilPercentage(int raw) {
  if (!soilCalibrationReady()) {
    return NAN;
  }

  float value = 100.0f * (raw - SOIL_DRY_RAW) /
                (SOIL_WET_RAW - SOIL_DRY_RAW);

  return constrain(value, 0.0f, 100.0f);
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
void serviceNetwork() {
  unsigned long now = millis();
  bool wifiConnected = WiFi.status() == WL_CONNECTED;

  if (!wifiConnected) {
    if (!wifiWasConnected) {
      showStartupStatus("Connecting WiFi...");
    }

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

    showStartupStatus("WiFi OK");

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

    showStartupStatus("Connecting MQTT...");
    updateNetworkLED();
    Serial.println("Connecting to AWS MQTT...");

    // This call can briefly block during a connection attempt.
    if (mqtt.connect(MQTT_CLIENT_ID)) {
      mqttEverConnected = true;
      showStartupStatus("MQTT OK");
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

  updateNetworkLED();
}

// ========== READ AND DISPLAY SENSORS ==========
void updateSensors() {
  bool soilTempOK = soilTempValid &&
    millis() - soilTempLastGood <= SOIL_TEMP_MAX_AGE_MS;
  int soilRaw = readSoilRaw();
  bool soilCalibrated = soilCalibrationReady();
  float soilPercent = soilPercentage(soilRaw);

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
    Serial.print(SOIL_DEMO_CALIBRATION ? "% DEMO" : "% relative");
  } else {
    Serial.print("Not calibrated");
  }

  Serial.print(" | Soil temp: ");
  if (soilTempOK) Serial.print(soilTemperatureC, 1);
  else Serial.print("ERROR");
  Serial.print(" C | ");
  Serial.print(message);
  Serial.print(" | ");
  Serial.println(networkText());

  // JSON and Serial output continue even if the OLED is unavailable.
  if (oledReady) {
    if (millis() - lastOLEDScreenChange >= OLED_SCREEN_INTERVAL_MS) {
      lastOLEDScreenChange = millis();
      oledScreen = (oledScreen + 1) % OLED_SCREEN_COUNT;
    }

    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("GreenPulse");
    display.setCursor(82, 0);
    display.print(networkText());

    if (oledScreen == 0) {
      display.setCursor(0, 12);
      display.print("Temp: ");
      if (dhtOK) { display.print(temperature, 1); display.print("C"); }
      else display.print("--");

      display.setCursor(0, 22);
      display.print("Hum: ");
      if (dhtOK) { display.print(humidity, 1); display.print("%"); }
      else display.print("--");

      display.setCursor(0, 32);
      display.print("Light: ");
      if (lightOK) { display.print(lux, 0); display.print(" lx"); }
      else display.print("--");

      display.setCursor(0, 52);
      display.print("Status: ");
      display.print(message);
    } else if (oledScreen == 1) {
      display.setCursor(0, 12);
      display.print("Soil: ");
      if (soilCalibrated) {
        display.print(soilPercent, 0);
        display.print("%");
      } else {
        display.print(soilRaw);
      }

      display.setCursor(0, 22);
      display.print("Soil T: ");
      if (soilTempOK) { display.print(soilTemperatureC, 1); display.print("C"); }
      else display.print("ERR");

      display.setCursor(0, 32);
      display.print("Air: ");
      if (dhtOK) { display.print(temperature, 1); display.print("C"); }
      else display.print("--");

      display.setCursor(0, 52);
      display.print("State: ");
      if (overallStatus == CRITICAL) display.print("CRIT");
      else if (overallStatus == WARNING) display.print("WARN");
      else display.print("OK");
    } else if (oledScreen == 2) {
      display.setCursor(0, 12);
      display.print("Light: ");
      if (lightOK) { display.print(lux, 0); display.print(" lx"); }
      else display.print("--");

      display.setCursor(0, 22);
      display.print("Soil: ");
      if (soilCalibrated) {
        display.print(soilPercent, 0);
        display.print("%");
      } else {
        display.print("CAL");
      }

      display.setCursor(0, 32);
      display.print("Plant: ");
      display.print(message);

      display.setCursor(0, 52);
      display.print("Mood: ");
      if (overallStatus == CRITICAL) display.print("Need help");
      else if (overallStatus == WARNING) display.print("Watch");
      else display.print("Good");
    } else if (oledScreen == 3) {
      display.setCursor(0, 12);
      display.print("WiFi: ");
      display.print(WiFi.status() == WL_CONNECTED ? "ON" : "OFF");

      display.setCursor(0, 26);
      display.print("MQTT: ");
      display.print(mqtt.connected() ? "ON" : "OFF");

      display.setCursor(0, 40);
      display.print("RSSI: ");
      if (WiFi.status() == WL_CONNECTED) {
        display.print(WiFi.RSSI());
      } else {
        display.print("--");
      }

      display.setCursor(0, 52);
      display.print("IP: ");
      if (WiFi.status() == WL_CONNECTED) {
        display.print(WiFi.localIP());
      } else {
        display.print("--");
      }
    } else {
      display.setCursor(0, 12);
      display.print("ALERT");
      display.setCursor(0, 24);
      display.print(message);

      display.setCursor(0, 36);
      if (dhtOK) { display.print("T:"); display.print(temperature, 1); }
      else { display.print("T:ERR"); }

      display.setCursor(48, 36);
      if (dhtOK) { display.print("H:"); display.print(humidity, 1); }
      else { display.print("H:ERR"); }

      display.setCursor(0, 52);
      if (lightOK) { display.print("L:"); display.print(lux, 0); }
      else { display.print("L:ERR"); }

      display.setCursor(48, 52);
      if (soilCalibrated) { display.print("S:"); display.print(soilPercent, 0); }
      else { display.print("S:ERR"); }
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

  char jsonString[512];
  if (doc.overflowed() || measureJson(doc) >= sizeof(jsonString)) {
    Serial.println("Sensor JSON exceeds buffer capacity; skipping payload.");
    return;
  }
  serializeJson(doc, jsonString, sizeof(jsonString));

  Serial.println("\n--- Generated JSON Payload ---");
  Serial.println(jsonString);
  Serial.println("------------------------------\n");

  if (mqtt.connected() && !mqtt.publish("greenpulse/sensors", jsonString)) {
    Serial.println("Failed to publish sensor payload.");
  }
}

// ========== SETUP ==========
void setup() {
  Serial.begin(115200);

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
  serviceSoilTemperature();
  serviceNetwork();
  serviceSoilTemperature();

  // Timed sensor updates keep the connection LED responsive.
  if (millis() - lastSensorRead >= 2500UL) {
    lastSensorRead = millis();
    updateSensors();
  }

  delay(5);
}