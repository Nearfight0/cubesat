#include <Wire.h>
#include <Adafruit_BMP280.h>

Adafruit_BMP280 bmp; // I2C

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);

  Wire.begin(); // Teensy 4.1 default I2C pins (18/19 on Wire)

  // BMP280 I2C address is usually 0x76 or 0x77
  if (!bmp.begin(0x76)) {
    Serial.println("BMP280 not found at 0x76, trying 0x77...");
    if (!bmp.begin(0x77)) {
      Serial.println("BMP280 not found! Check wiring/address.");
      while (1) delay(10);
    }
  }

  Serial.println("BMP280 found!");

  // Recommended settings for reasonably fast + low-noise readings
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     // operating mode
                   Adafruit_BMP280::SAMPLING_X2,     // temp oversampling
                   Adafruit_BMP280::SAMPLING_X16,    // pressure oversampling
                   Adafruit_BMP280::FILTER_X16,      // IIR filter
                   Adafruit_BMP280::STANDBY_MS_1);   // standby time
}

void loop() {
  float temp = bmp.readTemperature();       // °C
  float pressure = bmp.readPressure();      // Pa
  float altitude = bmp.readAltitude(1013.25); // meters, using standard sea-level pressure

  Serial.print("Temp: ");
  Serial.print(temp);
  Serial.print(" °C  |  Pressure: ");
  Serial.print(pressure / 100.0F); // hPa
  Serial.print(" hPa  |  Altitude: ");
  Serial.print(altitude);
  Serial.println(" m");

  delay(200); // adjust based on your standby setting
}