/*
  ============================================================================
  Mission: Kripto — CubeSat Flight Computer (Teensy 4.1) — INTEGRATED
  ============================================================================
  Sensors integrated in this version:
    - BNO086   : pitch / roll / yaw (SH-2 rotation vector, relative yaw)
    - BMP280   : pressure, relative altitude, descent speed
    - Battery  : voltage via resistor divider (20k/7k) on A0
    - GPS BN220: lat / lng / alt over Serial2 (RX2/TX2), NMEA via TinyGPS++.
                 TIME field uses GPS UTC once a fix is acquired; falls back
                 to a millis()-based elapsed HH:MM:SS counter before that.
    - SD card  : logs every telemetry packet as CSV (built-in Teensy 4.1
                 SD slot), filename <TEAM_ID>.csv, one row per packet.

  NOT integrated yet (placeholders in the telemetry packet):
    - Separation       -> SEPARATION = 0
    - Crypto message   -> CRYPTO_MSG = NULL

  NOTE: requires the TinyGPSPlus library (install via Library Manager:
  "TinyGPSPlus" by Mikal Hart).

  Telemetry is sent over E30 LoRa (Serial7, RX7/TX7) to the ground station,
  1 Hz, in the exact competition format:

  <TEAM_ID>,<PACKET_NO>,<TIME>,<PRESSURE>,<REL_ALTITUDE>,<DESCENT_SPEED>,
  <VOLTAGE>,<GPS_LAT>,<GPS_LNG>,<GPS_ALT>,<PITCH>,<ROLL>,<YAW>,
  <SEPARATION>,<CRYPTO_MSG>

  PACKET_NO persists across reboots using Teensy's EEPROM emulation.

  >>> REPLACE "00000" BELOW WITH YOUR ACTUAL TEAM ID BEFORE FLIGHT <<<
  ============================================================================
*/

#include <Wire.h>
#include <EEPROM.h>
#include <Adafruit_BNO08x.h>
#include <Adafruit_BMP280.h>
#include <TinyGPS++.h>
#include <SD.h>

// ---------------------------------------------------------------------------
// Config
// ---------------------------------------------------------------------------
#define TEAM_ID "00000"           // <-- put your real team ID here

#define BNO08X_RESET -1
#define BMP280_ADDR_PRIMARY   0x76
#define BMP280_ADDR_SECONDARY 0x77

#define BATT_PIN A0
#define BATT_R1  22.4f            // kOhm, battery side (measure with multimeter!)
#define BATT_R2  7.0f             // kOhm, GND side (measure with multimeter!)
#define BATT_VREF 3.3f
#define BATT_ADC_RES 4095
#define BATT_SAMPLES 50

#define EEPROM_ADDR_PACKET 0      // 4 bytes reserved for packet counter
#define TELEMETRY_INTERVAL_MS 1000

#define GPS_SERIAL Serial2        // BN220 wired to RX2/TX2
#define GPS_BAUD 9600             // BN220 default baud rate

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------
Adafruit_BNO08x bno08x(BNO08X_RESET);
sh2_SensorValue_t sensorValue;
Adafruit_BMP280 bmp;
TinyGPSPlus gps;

double gpsLatitude = 0.0;
double gpsLongitude = 0.0;
double gpsAltitude = 0.0;
bool gpsHasFix = false;
uint8_t gpsHour = 0, gpsMinute = 0, gpsSecond = 0;

float yawOffset = 0;
bool offsetSet = false;
int readingCount = 0;

float currentPitch = 0, currentRoll = 0, currentYaw = 0;
uint8_t currentAccuracy = 0;

float groundPressure = 0;      // hPa, calibrated at startup (altitude = 0 here)
float lastAltitude = 0;
float currentPressure = 0;     // hPa
float currentAltitude = 0;     // m, relative to ground
float descentSpeed = 0;        // m/s, positive = descending
unsigned long lastAltTime = 0;

float batteryVoltage = 0;

bool imuReady = false;
bool bmpReady = false;
bool sdReady = false;

File dataFile;
char csvFilename[24];

uint32_t packetNo = 0;

unsigned long lastTelemetryTime = 0;
unsigned long startMillis = 0;

// ---------------------------------------------------------------------------
// BNO086 helpers
// ---------------------------------------------------------------------------
void quaternionToEuler(float qr, float qi, float qj, float qk,
                        float &yaw, float &pitch, float &roll) {
  float t0 = 2.0 * (qr * qi + qj * qk);
  float t1 = 1.0 - 2.0 * (qi * qi + qj * qj);
  roll = atan2(t0, t1) * 180.0 / PI;

  float t2 = 2.0 * (qr * qj - qk * qi);
  t2 = t2 > 1.0 ? 1.0 : t2;
  t2 = t2 < -1.0 ? -1.0 : t2;
  pitch = asin(t2) * 180.0 / PI;

  float t3 = 2.0 * (qr * qk + qi * qj);
  float t4 = 1.0 - 2.0 * (qj * qj + qk * qk);
  yaw = atan2(t3, t4) * 180.0 / PI;
}

float normalizeAngle(float angle) {
  while (angle > 180) angle -= 360;
  while (angle < -180) angle += 360;
  return angle;
}

void setReports(void) {
  if (!bno08x.enableReport(SH2_ROTATION_VECTOR)) {
    Serial.println("Could not enable rotation vector");
  }
}

void updateIMU() {
  if (!imuReady) return;

  if (bno08x.wasReset()) {
    Serial.println("BNO086 was reset, re-enabling reports");
    setReports();
    offsetSet = false;
    readingCount = 0;
  }

  if (!bno08x.getSensorEvent(&sensorValue)) {
    return;
  }

  if (sensorValue.sensorId == SH2_ROTATION_VECTOR) {
    float yaw, pitch, roll;
    quaternionToEuler(
      sensorValue.un.rotationVector.real,
      sensorValue.un.rotationVector.i,
      sensorValue.un.rotationVector.j,
      sensorValue.un.rotationVector.k,
      yaw, pitch, roll
    );

    readingCount++;
    if (!offsetSet && readingCount > 10) {
      yawOffset = yaw;
      offsetSet = true;
      Serial.print(">>> Yaw offset set: ");
      Serial.println(yawOffset, 2);
    }

    currentPitch = pitch;
    currentRoll = roll;
    currentYaw = offsetSet ? normalizeAngle(yaw - yawOffset) : 0;
    currentAccuracy = sensorValue.status;
  }
}

// ---------------------------------------------------------------------------
// BMP280 helpers
// ---------------------------------------------------------------------------
void calibrateGroundPressure() {
  Serial.println("Calibrating ground-zero pressure (50 samples)...");
  float sum = 0;
  for (int i = 0; i < 50; i++) {
    sum += bmp.readPressure() / 100.0F; // hPa
    delay(20);
  }
  groundPressure = sum / 50.0F;
  Serial.print("Ground pressure: ");
  Serial.print(groundPressure, 2);
  Serial.println(" hPa");
  lastAltitude = 0;
  lastAltTime = millis();
}

void updateBMP280() {
  if (!bmpReady) return;

  currentPressure = bmp.readPressure() / 100.0F; // hPa
  currentAltitude = bmp.readAltitude(groundPressure); // m relative to ground

  unsigned long now = millis();
  float dt = (now - lastAltTime) / 1000.0f; // seconds
  if (dt > 0.05f) { // avoid divide-by-near-zero on fast loops
    float rate = (lastAltitude - currentAltitude) / dt; // + = descending
    descentSpeed = rate;
    lastAltitude = currentAltitude;
    lastAltTime = now;
  }
}

// ---------------------------------------------------------------------------
// Battery helpers
// ---------------------------------------------------------------------------
void updateBattery() {
  long sum = 0;
  for (int i = 0; i < BATT_SAMPLES; i++) {
    sum += analogRead(BATT_PIN);
    delayMicroseconds(100);
  }
  float avgReading = (float)sum / BATT_SAMPLES;
  float vAdc = avgReading * (BATT_VREF / BATT_ADC_RES);
  batteryVoltage = vAdc * ((BATT_R1 + BATT_R2) / BATT_R2);
}

// ---------------------------------------------------------------------------
// GPS (BN220) helpers
// ---------------------------------------------------------------------------
void updateGPS() {
  // Feed any waiting NMEA bytes to the parser (non-blocking)
  while (GPS_SERIAL.available()) {
    gps.encode(GPS_SERIAL.read());
  }

  if (gps.location.isValid() && gps.location.isUpdated()) {
    gpsLatitude = gps.location.lat();
    gpsLongitude = gps.location.lng();
    gpsHasFix = true;
  }

  if (gps.altitude.isValid()) {
    gpsAltitude = gps.altitude.meters();
  }

  if (gps.time.isValid() && gps.time.isUpdated()) {
    gpsHour = gps.time.hour();
    gpsMinute = gps.time.minute();
    gpsSecond = gps.time.second();
  }
}

// ---------------------------------------------------------------------------
// SD card logging (Teensy 4.1 built-in SD slot)
// ---------------------------------------------------------------------------
void initSDCard() {
  Serial.println("Initializing SD card...");
  if (!SD.begin(BUILTIN_SDCARD)) {
    Serial.println("!!! SD CARD NOT FOUND — telemetry will not be logged to SD !!!");
    sdReady = false;
    return;
  }

  snprintf(csvFilename, sizeof(csvFilename), "%s.csv", TEAM_ID);
  bool fileAlreadyExists = SD.exists(csvFilename);

  dataFile = SD.open(csvFilename, FILE_WRITE); // creates if missing, appends if exists
  if (!dataFile) {
    Serial.println("!!! Failed to open CSV file on SD card !!!");
    sdReady = false;
    return;
  }

  if (!fileAlreadyExists) {
    dataFile.println("TEAM_ID,PACKET_NO,TIME,PRESSURE,REL_ALTITUDE,DESCENT_SPEED,VOLTAGE,GPS_LAT,GPS_LNG,GPS_ALT,PITCH,ROLL,YAW,SEPARATION,CRYPTO_MSG");
    dataFile.flush();
  }

  sdReady = true;
  Serial.print("SD card ready, logging to ");
  Serial.println(csvFilename);
}

void logPacketToSD(const char* packet) {
  if (!sdReady) return;
  dataFile.println(packet);
  dataFile.flush(); // ensures data survives a sudden power loss / impact on landing
}

// ---------------------------------------------------------------------------
// EEPROM packet counter (persists across reboots)
// ---------------------------------------------------------------------------
void loadPacketNo() {
  EEPROM.get(EEPROM_ADDR_PACKET, packetNo);
  if (packetNo == 0xFFFFFFFF) { // blank/first-boot Teensy
    packetNo = 0;
  }
}

void savePacketNo() {
  EEPROM.put(EEPROM_ADDR_PACKET, packetNo); // only writes if value changed
}

// ---------------------------------------------------------------------------
// Telemetry
// ---------------------------------------------------------------------------
void sendTelemetry() {
  packetNo++;
  savePacketNo();

  // TIME: use GPS UTC once we've had at least one valid fix; otherwise
  // fall back to a millis()-based elapsed HH:MM:SS counter from boot.
  char timeStr[9];
  if (gpsHasFix) {
    snprintf(timeStr, sizeof(timeStr), "%02u:%02u:%02u", gpsHour, gpsMinute, gpsSecond);
  } else {
    unsigned long elapsedSec = (millis() - startMillis) / 1000;
    unsigned long h = (elapsedSec / 3600) % 24;
    unsigned long m = (elapsedSec / 60) % 60;
    unsigned long s = elapsedSec % 60;
    snprintf(timeStr, sizeof(timeStr), "%02lu:%02lu:%02lu", h, m, s);
  }

  // Placeholders — not yet integrated
  const int separation = 0;
  const char* cryptoMsg = "NULL";

  char packet[220];
  snprintf(packet, sizeof(packet),
    "%s,%lu,%s,%.2f,%.2f,%.2f,%.2f,%.6f,%.6f,%.2f,%.2f,%.2f,%.2f,%d,%s",
    TEAM_ID,
    (unsigned long)packetNo,
    timeStr,
    currentPressure,
    currentAltitude,
    descentSpeed,
    batteryVoltage,
    gpsLatitude, gpsLongitude, gpsAltitude,
    currentPitch, currentRoll, currentYaw,
    separation,
    cryptoMsg
  );

  Serial7.println(packet);   // -> LoRa -> ground station
  Serial.println(packet);    // -> USB debug
  logPacketToSD(packet);     // -> SD card CSV log
}

// ---------------------------------------------------------------------------
// Setup / Loop
// ---------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  unsigned long serialWaitStart = millis();
  while (!Serial && (millis() - serialWaitStart < 3000)) delay(10); // 3s timeout, no USB = no hang
  Serial7.begin(9600); // must match E30 UART baud rate
  GPS_SERIAL.begin(GPS_BAUD);

  Wire.begin();

  initSDCard();

  Serial.println("Initializing BNO086...");
  if (!bno08x.begin_I2C(0x4B)) {
    Serial.println("!!! BNO086 NOT FOUND — continuing without IMU data !!!");
    imuReady = false;
  } else {
    setReports();
    imuReady = true;
    Serial.println("BNO086 ready.");
  }

  Serial.println("Initializing BMP280...");
  if (bmp.begin(BMP280_ADDR_PRIMARY)) {
    bmpReady = true;
  } else {
    Serial.println("BMP280 not found at 0x76, trying 0x77...");
    if (bmp.begin(BMP280_ADDR_SECONDARY)) {
      bmpReady = true;
    } else {
      Serial.println("!!! BMP280 NOT FOUND — continuing without pressure/altitude data !!!");
      bmpReady = false;
    }
  }
  if (bmpReady) {
    bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                     Adafruit_BMP280::SAMPLING_X2,
                     Adafruit_BMP280::SAMPLING_X16,
                     Adafruit_BMP280::FILTER_X16,
                     Adafruit_BMP280::STANDBY_MS_1);
    calibrateGroundPressure();
    Serial.println("BMP280 ready.");
  }

  analogReadResolution(12);

  loadPacketNo();
  startMillis = millis();
  lastTelemetryTime = millis();

  Serial.println("=== Integrated telemetry started ===");
}

void loop() {
  updateIMU();
  updateBMP280();
  updateBattery();
  updateGPS();

  // Serial command: 's' saves current IMU calibration to flash
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 's' || c == 'S') {
      Serial.println(">>> Saving IMU calibration...");
      int status = sh2_saveDcdNow();
      Serial.println(status == SH2_OK ? ">>> Saved!" : ">>> Save failed, retry.");
    }
  }

  unsigned long now = millis();
  if (now - lastTelemetryTime >= TELEMETRY_INTERVAL_MS) {
    lastTelemetryTime = now;
    sendTelemetry();
  }
}
