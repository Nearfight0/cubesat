const float R1 = 22.4;  // kOhm - hansi divider secdinse, deqiq deyeri yaz
const float R2 = 7.24;   // kOhm - multimetrle olculmus deyer
const float VREF = 3.3;
const int ADC_RESOLUTION = 4095; // 12-bit

const int BATT_PIN = A0;
const int NUM_SAMPLES = 50;

void setup() {
  Serial.begin(9600);      // USB debug
  Serial7.begin(9600);     // E30 LoRa
  analogReadResolution(12);
}

float readBatteryVoltage() {
  long sum = 0;
  for (int i = 0; i < NUM_SAMPLES; i++) {
    sum += analogRead(BATT_PIN);
    delayMicroseconds(100);
  }
  float avgReading = (float)sum / NUM_SAMPLES;
  float vAdc = avgReading * (VREF / ADC_RESOLUTION);
  float vBattery = vAdc * ((R1 + R2) / R2);
  return vBattery;
}

void loop() {
  float voltage = 2 * readBatteryVoltage();

  // LoRa'ya gonder
  Serial7.print("BATT:");
  Serial7.println(voltage, 3);

  // Debug ucun USB'ye de yaz
  Serial.print("Gonderildi -> BATT: ");
  Serial.println(voltage, 3);

  delay(1000);
}