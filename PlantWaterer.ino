/***************************************************
   PlantWaterer v1.1
   ESP32 + Blynk IoT
   Four-zone automatic plant watering system
****************************************************/

#define BLYNK_PRINT Serial

// Blynk template information and private credentials must be defined
// before BlynkSimpleEsp32.h is included.
#include "secrets.h"

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <time.h>

// -------------------------------------------------
// GENERAL CONFIGURATION
// -------------------------------------------------

constexpr uint8_t NUM_ZONES = 4;

// ADC1 pins are used because ADC2 conflicts with Wi-Fi on ESP32.
constexpr uint8_t MOISTURE_PINS[NUM_ZONES] = {32, 33, 34, 35};

// Four-channel relay module inputs.
constexpr uint8_t RELAY_PINS[NUM_ZONES] = {16, 17, 18, 19};

// Status LED.
constexpr uint8_t LED_PIN = 25;

// Software tank-level switch.
// Wire one side to GPIO 27 and the other side to GND.
constexpr uint8_t FLOAT_STATUS_PIN = 27;

// The relay module is active LOW.
constexpr uint8_t RELAY_ON = LOW;
constexpr uint8_t RELAY_OFF = HIGH;

// -------------------------------------------------
// SOIL-MOISTURE CALIBRATION
// -------------------------------------------------

// Replace these values with measurements from your own sensors.
//
// RAW_WET: reading while the sensor is in fully wet soil.
// RAW_DRY: reading while the sensor is in dry soil.
//
// The program converts these raw ADC readings to a 0-100% scale:
// 0% = dry, 100% = wet.
constexpr int RAW_WET = 1200;
constexpr int RAW_DRY = 3200;

// Desired minimum moisture percentage for each plant.
constexpr uint8_t TARGET_MOISTURE_PCT[NUM_ZONES] = {40, 40, 40, 40};

// Pump run time for each watering dose.
constexpr unsigned long DOSE_MS[NUM_ZONES] = {
  6000UL, 6000UL, 6000UL, 6000UL
};

// Minimum delay between automatic watering attempts for the same zone.
constexpr unsigned long WATERING_INTERVAL_MS =
  3UL * 60UL * 60UL * 1000UL;

// Sensor and control intervals.
constexpr unsigned long SENSOR_SAMPLE_INTERVAL_MS = 5000UL;
constexpr unsigned long AUTO_CHECK_INTERVAL_MS = 10000UL;
constexpr unsigned long STATUS_UPDATE_INTERVAL_MS = 2000UL;
constexpr unsigned long TIME_UPDATE_INTERVAL_MS = 15000UL;
constexpr unsigned long CONNECTION_CHECK_INTERVAL_MS = 10000UL;

// -------------------------------------------------
// TIME CONFIGURATION
// -------------------------------------------------

// Romania / Eastern European Time with automatic daylight-saving handling.
// Change this string if the system is used in another timezone.
constexpr char TIMEZONE_RULE[] =
  "EET-2EEST,M3.5.0/3,M10.5.0/4";

constexpr char NTP_SERVER_1[] = "pool.ntp.org";
constexpr char NTP_SERVER_2[] = "time.nist.gov";

// Automatic watering is allowed from 07:00 until 21:59.
constexpr uint8_t START_HOUR = 7;
constexpr uint8_t END_HOUR = 22;

// When false, automatic watering waits until the clock has synchronized.
// This avoids watering outside the permitted time window after a reboot.
constexpr bool ALLOW_AUTO_WATERING_WITHOUT_TIME = false;

// When true, dry plants may be watered as soon as the system starts.
// When false, every zone waits one full watering interval after startup.
constexpr bool ALLOW_FIRST_WATERING_IMMEDIATELY = true;

// -------------------------------------------------
// BLYNK VIRTUAL PINS
// -------------------------------------------------

#define VPIN_MOIST1        V1
#define VPIN_MOIST2        V2
#define VPIN_MOIST3        V3
#define VPIN_MOIST4        V4

#define VPIN_PUMP_ACTIVE   V7
#define VPIN_ERROR_MESSAGE V8
#define VPIN_TANK_STATUS   V10
#define VPIN_TIME_TO_WATER V11

// 0 = normal, 1 = watering, 2 = tank empty.
#define VPIN_LED_STATE     V30

#define VPIN_MANUAL1       V20
#define VPIN_MANUAL2       V21
#define VPIN_MANUAL3       V22
#define VPIN_MANUAL4       V23

// -------------------------------------------------
// GLOBAL STATE
// -------------------------------------------------

BlynkTimer timer;

enum TankState : uint8_t {
  TANK_OK = 0,
  TANK_EMPTY = 2
};

TankState tankState = TANK_OK;

unsigned long lastAutoWaterMs[NUM_ZONES] = {0, 0, 0, 0};
int lastMoistureRaw[NUM_ZONES] = {0, 0, 0, 0};
uint8_t lastMoisturePct[NUM_ZONES] = {0, 0, 0, 0};

bool isWatering = false;
int8_t currentZone = -1;
unsigned long waterStartMs = 0;
unsigned long activeWaterDurationMs = 0;

bool ledWatering = false;
bool ledTankEmpty = false;

// -------------------------------------------------
// HELPER FUNCTIONS
// -------------------------------------------------

bool timeReached(unsigned long now, unsigned long deadline) {
  return static_cast<long>(now - deadline) >= 0;
}

uint8_t moistureRawToPercent(int raw) {
  if (RAW_WET == RAW_DRY) {
    return 0;
  }

  long percentage = map(raw, RAW_WET, RAW_DRY, 100, 0);
  percentage = constrain(percentage, 0L, 100L);
  return static_cast<uint8_t>(percentage);
}

bool isClockSynchronized() {
  return time(nullptr) >= 100000;
}

bool isWithinWateringWindow() {
  if (!isClockSynchronized()) {
    return ALLOW_AUTO_WATERING_WITHOUT_TIME;
  }

  time_t now = time(nullptr);
  struct tm localTime;

  if (localtime_r(&now, &localTime) == nullptr) {
    return ALLOW_AUTO_WATERING_WITHOUT_TIME;
  }

  const int hour = localTime.tm_hour;

  // Standard daytime window, for example 07:00-22:00.
  if (START_HOUR < END_HOUR) {
    return hour >= START_HOUR && hour < END_HOUR;
  }

  // Also supports an overnight window, for example 22:00-07:00.
  return hour >= START_HOUR || hour < END_HOUR;
}

void updateTankState() {
  // INPUT_PULLUP means:
  // LOW  = switch connected to GND = water present
  // HIGH = switch open             = tank empty
  tankState =
    (digitalRead(FLOAT_STATUS_PIN) == LOW) ? TANK_OK : TANK_EMPTY;

  ledTankEmpty = (tankState == TANK_EMPTY);
}

void stopActivePump(const char* message = nullptr) {
  if (currentZone >= 0 && currentZone < NUM_ZONES) {
    digitalWrite(RELAY_PINS[currentZone], RELAY_OFF);
  }

  isWatering = false;
  currentZone = -1;
  waterStartMs = 0;
  activeWaterDurationMs = 0;
  ledWatering = false;

  if (Blynk.connected()) {
    Blynk.virtualWrite(VPIN_PUMP_ACTIVE, 0);

    if (message != nullptr) {
      Blynk.virtualWrite(VPIN_ERROR_MESSAGE, message);
    }
  }
}

void updateStatusLED() {
  const unsigned long now = millis();
  bool ledOn = false;

  if (ledWatering) {
    // Two short flashes per second.
    const unsigned long phase = now % 1000UL;
    ledOn = (phase < 150UL) || (phase >= 300UL && phase < 450UL);
  } else if (ledTankEmpty) {
    // Rapid flash when the tank is empty.
    ledOn = (now % 400UL) < 200UL;
  }

  digitalWrite(LED_PIN, ledOn ? HIGH : LOW);
}

void sampleMoistureSensors() {
  for (uint8_t zone = 0; zone < NUM_ZONES; zone++) {
    const int raw = analogRead(MOISTURE_PINS[zone]);
    lastMoistureRaw[zone] = raw;
    lastMoisturePct[zone] = moistureRawToPercent(raw);
  }
}

void sendMoistureToBlynk() {
  if (!Blynk.connected()) {
    return;
  }

  // Blynk receives human-friendly values from 0% to 100%.
  Blynk.virtualWrite(VPIN_MOIST1, lastMoisturePct[0]);
  Blynk.virtualWrite(VPIN_MOIST2, lastMoisturePct[1]);
  Blynk.virtualWrite(VPIN_MOIST3, lastMoisturePct[2]);
  Blynk.virtualWrite(VPIN_MOIST4, lastMoisturePct[3]);
}

void sendStatusToBlynk() {
  updateTankState();

  if (!Blynk.connected()) {
    return;
  }

  Blynk.virtualWrite(VPIN_TANK_STATUS, static_cast<int>(tankState));
  Blynk.virtualWrite(VPIN_PUMP_ACTIVE, isWatering ? 1 : 0);

  int ledState = 0;

  if (ledWatering) {
    ledState = 1;
  } else if (ledTankEmpty) {
    ledState = 2;
  }

  Blynk.virtualWrite(VPIN_LED_STATE, ledState);
}

void sendTimeToWaterBlynk() {
  if (!Blynk.connected()) {
    return;
  }

  const unsigned long now = millis();
  unsigned long minimumRemaining = WATERING_INTERVAL_MS;
  bool atLeastOneZoneReady = false;

  for (uint8_t zone = 0; zone < NUM_ZONES; zone++) {
    const unsigned long elapsed = now - lastAutoWaterMs[zone];

    if (elapsed >= WATERING_INTERVAL_MS) {
      atLeastOneZoneReady = true;
      minimumRemaining = 0;
      break;
    }

    const unsigned long remaining = WATERING_INTERVAL_MS - elapsed;

    if (remaining < minimumRemaining) {
      minimumRemaining = remaining;
    }
  }

  char buffer[32];

  if (atLeastOneZoneReady) {
    snprintf(buffer, sizeof(buffer), "Ready now");
  } else {
    unsigned long seconds = minimumRemaining / 1000UL;
    const unsigned long hours = seconds / 3600UL;
    seconds %= 3600UL;
    const unsigned long minutes = seconds / 60UL;
    seconds %= 60UL;

    snprintf(
      buffer,
      sizeof(buffer),
      "%02lu:%02lu:%02lu",
      hours,
      minutes,
      seconds
    );
  }

  Blynk.virtualWrite(VPIN_TIME_TO_WATER, buffer);
}

void servicePumpRuntime() {
  if (!isWatering) {
    return;
  }

  updateTankState();

  if (tankState != TANK_OK) {
    stopActivePump("Tank empty - pump stopped");
    return;
  }

  // Using elapsed time rather than an absolute deadline keeps millis()
  // rollover safe.
  if (millis() - waterStartMs >= activeWaterDurationMs) {
    stopActivePump();
  }
}

bool startWateringZone(
  uint8_t zone,
  unsigned long durationMs,
  bool isManual
) {
  if (zone >= NUM_ZONES || durationMs == 0) {
    return false;
  }

  if (isWatering) {
    if (Blynk.connected()) {
      Blynk.virtualWrite(
        VPIN_ERROR_MESSAGE,
        "Another pump is already active"
      );
    }

    return false;
  }

  updateTankState();

  if (tankState != TANK_OK) {
    if (Blynk.connected()) {
      Blynk.virtualWrite(
        VPIN_ERROR_MESSAGE,
        "Tank empty - watering blocked"
      );
    }

    return false;
  }

  // Manual watering ignores the time window but not tank protection.
  if (!isManual && !isWithinWateringWindow()) {
    if (Blynk.connected()) {
      Blynk.virtualWrite(
        VPIN_ERROR_MESSAGE,
        isClockSynchronized()
          ? "Outside watering window"
          : "Waiting for clock synchronization"
      );
    }

    return false;
  }

  currentZone = static_cast<int8_t>(zone);
  isWatering = true;
  ledWatering = true;
  waterStartMs = millis();
  activeWaterDurationMs = durationMs;

  digitalWrite(RELAY_PINS[zone], RELAY_ON);

  if (Blynk.connected()) {
    Blynk.virtualWrite(VPIN_PUMP_ACTIVE, 1);
    Blynk.virtualWrite(VPIN_ERROR_MESSAGE, "");
  }

  Serial.printf(
    "%s watering started for zone %u (%lu ms)\n",
    isManual ? "Manual" : "Automatic",
    zone + 1,
    durationMs
  );

  return true;
}

void automaticWateringCheck() {
  if (isWatering) {
    return;
  }

  updateTankState();

  if (tankState != TANK_OK || !isWithinWateringWindow()) {
    return;
  }

  const unsigned long now = millis();

  for (uint8_t zone = 0; zone < NUM_ZONES; zone++) {
    const unsigned long elapsed = now - lastAutoWaterMs[zone];

    if (elapsed < WATERING_INTERVAL_MS) {
      continue;
    }

    // Use the latest periodic reading. Refresh it here as well so the
    // watering decision is based on a current sample.
    const int raw = analogRead(MOISTURE_PINS[zone]);
    const uint8_t percentage = moistureRawToPercent(raw);

    lastMoistureRaw[zone] = raw;
    lastMoisturePct[zone] = percentage;

    Serial.printf(
      "Zone %u moisture: %u%%, raw: %d, target: %u%%\n",
      zone + 1,
      percentage,
      raw,
      TARGET_MOISTURE_PCT[zone]
    );

    if (percentage < TARGET_MOISTURE_PCT[zone]) {
      if (startWateringZone(zone, DOSE_MS[zone], false)) {
        // Record the interval only after the pump actually starts.
        lastAutoWaterMs[zone] = now;
      }

      // Only one zone may start during each check.
      break;
    }
  }
}

void maintainConnections() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Connecting to Wi-Fi...");
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    return;
  }

  if (!Blynk.connected()) {
    Serial.println("Connecting to Blynk...");
    Blynk.connect(1000);
  }
}

// -------------------------------------------------
// BLYNK CALLBACKS
// -------------------------------------------------

BLYNK_CONNECTED() {
  Blynk.syncVirtual(
    VPIN_MANUAL1,
    VPIN_MANUAL2,
    VPIN_MANUAL3,
    VPIN_MANUAL4
  );

  sendMoistureToBlynk();
  sendStatusToBlynk();
  sendTimeToWaterBlynk();
}

void handleManualWatering(uint8_t zone, int virtualPin, int value) {
  if (value != 0) {
    startWateringZone(zone, DOSE_MS[zone], true);
  }

  // Always return momentary Blynk buttons to their OFF state.
  if (Blynk.connected()) {
    Blynk.virtualWrite(virtualPin, 0);
  }
}

BLYNK_WRITE(VPIN_MANUAL1) {
  handleManualWatering(0, VPIN_MANUAL1, param.asInt());
}

BLYNK_WRITE(VPIN_MANUAL2) {
  handleManualWatering(1, VPIN_MANUAL2, param.asInt());
}

BLYNK_WRITE(VPIN_MANUAL3) {
  handleManualWatering(2, VPIN_MANUAL3, param.asInt());
}

BLYNK_WRITE(VPIN_MANUAL4) {
  handleManualWatering(3, VPIN_MANUAL4, param.asInt());
}

// -------------------------------------------------
// SETUP AND MAIN LOOP
// -------------------------------------------------

void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  pinMode(FLOAT_STATUS_PIN, INPUT_PULLUP);

  for (uint8_t zone = 0; zone < NUM_ZONES; zone++) {
    pinMode(RELAY_PINS[zone], OUTPUT);

    // Set every relay OFF before starting network services.
    digitalWrite(RELAY_PINS[zone], RELAY_OFF);
  }

  analogReadResolution(12);

  sampleMoistureSensors();
  updateTankState();

  const unsigned long now = millis();

  for (uint8_t zone = 0; zone < NUM_ZONES; zone++) {
    lastAutoWaterMs[zone] =
      ALLOW_FIRST_WATERING_IMMEDIATELY
        ? now - WATERING_INTERVAL_MS
        : now;
  }

  // Configure Wi-Fi and Blynk without blocking local watering logic.
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(false);

  Blynk.config(BLYNK_AUTH_TOKEN);

  maintainConnections();

  // Apply timezone rules and begin NTP synchronization.
  configTzTime(TIMEZONE_RULE, NTP_SERVER_1, NTP_SERVER_2);

  timer.setInterval(
    SENSOR_SAMPLE_INTERVAL_MS,
    []() {
      sampleMoistureSensors();
      sendMoistureToBlynk();
    }
  );

  timer.setInterval(AUTO_CHECK_INTERVAL_MS, automaticWateringCheck);
  timer.setInterval(STATUS_UPDATE_INTERVAL_MS, sendStatusToBlynk);
  timer.setInterval(TIME_UPDATE_INTERVAL_MS, sendTimeToWaterBlynk);
  timer.setInterval(CONNECTION_CHECK_INTERVAL_MS, maintainConnections);

  Serial.println("PlantWaterer setup complete.");
}

void loop() {
  if (Blynk.connected()) {
    Blynk.run();
  }

  timer.run();
  servicePumpRuntime();
  updateStatusLED();
}
