#include <Wire.h>
#include <Adafruit_INA219.h>

Adafruit_INA219 ina219(0x40);

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }

  Wire.begin();  // Teensy 4.1 default I2C (pin 18/19)

  if (!ina219.begin()) {
    Serial.println("INA219 tapılmadı! Qoşulmanı yoxlayın.");
    while (1) { delay(10); }
  }

  // 32V, 2A rejimi — geniş diapazon, orta dəqiqlik
  // Əgər cərəyan 2A-dan aşağı qalırsa, daha yaxşı dəqiqlik üçün
  // setCalibration_16V_400mA() istifadə edin
  ina219.setCalibration_32V_2A();

  Serial.println("INA219 hazırdır.");
}

void loop() {
  float shuntvoltage = ina219.getShuntVoltage_mV();
  float busvoltage   = ina219.getBusVoltage_V();
  float current_mA   = ina219.getCurrent_mA();
  float power_mW      = ina219.getPower_mW();
  float loadvoltage  = busvoltage + (shuntvoltage / 1000);

  Serial.print("Bus Voltajı:   "); Serial.print(busvoltage);   Serial.println(" V");
  Serial.print("Shunt Voltajı: "); Serial.print(shuntvoltage); Serial.println(" mV");
  Serial.print("Yük Voltajı:   "); Serial.print(loadvoltage);  Serial.println(" V");
  Serial.print("Cərəyan:       "); Serial.print(current_mA);  Serial.println(" mA");
  Serial.print("Güc:           "); Serial.print(power_mW);    Serial.println(" mW");
  Serial.println("---");

  delay(1000);
}