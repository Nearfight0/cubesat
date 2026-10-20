// BNO08x test - I2C address 0x4B, Rotation Vector -> Pitch/Roll/Yaw (relative yaw)
#include <Adafruit_BNO08x.h>

#define BNO08X_CS 10
#define BNO08X_INT 9
#define BNO08X_RESET -1

Adafruit_BNO08x  bno08x(BNO08X_RESET);
sh2_SensorValue_t sensorValue;

float yawOffset = 0;
bool offsetSet = false;
int readingCount = 0;

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

// -180/+180 araligina normalize eder (offset cikarinca tasma olabilir)
float normalizeAngle(float angle) {
  while (angle > 180) angle -= 360;
  while (angle < -180) angle += 360;
  return angle;
}

void setup(void) {
  Serial.begin(115200);
  while (!Serial) delay(10);

  Serial.println("Adafruit BNO08x test!");

  if (!bno08x.begin_I2C(0x4B)) {
    Serial.println("Failed to find BNO08x chip");
    while (1) { delay(10); }
  }
  Serial.println("BNO08x Found!");

  setReports();
  Serial.println("Reading events");
  delay(100);
}

void setReports(void) {
  Serial.println("Setting desired reports");
  if (! bno08x.enableReport(SH2_ROTATION_VECTOR)) {
    Serial.println("Could not enable rotation vector");
  }
}

void loop() {
  delay(10);

  if (bno08x.wasReset()) {
    Serial.print("sensor was reset ");
    setReports();
    offsetSet = false;   // resetlenince offset'i yeniden al
    readingCount = 0;
  }

  if (! bno08x.getSensorEvent(&sensorValue)) {
    return;
  }

  switch (sensorValue.sensorId) {

    case SH2_ROTATION_VECTOR: {
      float yaw, pitch, roll;
      quaternionToEuler(
        sensorValue.un.rotationVector.real,
        sensorValue.un.rotationVector.i,
        sensorValue.un.rotationVector.j,
        sensorValue.un.rotationVector.k,
        yaw, pitch, roll
      );

      // Ilk birkac okumayi atla (sensor henuz stabil olmayabilir), sonra offset'i sabitle
      readingCount++;
      if (!offsetSet && readingCount > 10) {
        yawOffset = yaw;
        offsetSet = true;
        Serial.print(">>> Yaw offset ayarlandi: ");
        Serial.println(yawOffset, 2);
      }

      float relativeYaw = offsetSet ? normalizeAngle(yaw - yawOffset) : 0;

      Serial.print("Roll: ");
      Serial.print(roll, 2);
      Serial.print(" Pitch: ");
      Serial.print(pitch, 2);
      Serial.print(" Yaw(relative): ");
      Serial.print(relativeYaw, 2);
      Serial.print(" | accuracy(status): ");
      Serial.println(sensorValue.status);
      break;
    }
  }

  if (Serial.available()) {
    char c = Serial.read();
    if (c == 's' || c == 'S') {
      Serial.println(">>> Kalibrasyon kaydediliyor...");
      int status = sh2_saveDcdNow();
      if (status == SH2_OK) {
        Serial.println(">>> KAYDEDILDI! Artik kalici.");
      } else {
        Serial.println(">>> Kayit basarisiz oldu, tekrar dene.");
      }
    }
  }
}