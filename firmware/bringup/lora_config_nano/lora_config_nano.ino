#include <SoftwareSerial.h>

SoftwareSerial loraSerial(2, 3); // RX, TX

#define M0_PIN 4
#define M1_PIN 5

byte configBytes[] = {0xC0, 0x00, 0x00, 0x19, 0x06, 0x44};

void setLoRaMode(bool m0, bool m1) {
  digitalWrite(M0_PIN, m0 ? HIGH : LOW);
  digitalWrite(M1_PIN, m1 ? HIGH : LOW);
  delay(50);
}

void setup() {
  Serial.begin(9600);
  loraSerial.begin(9600);

  pinMode(M0_PIN, OUTPUT);
  pinMode(M1_PIN, OUTPUT);

  // AT / config rejiminə keç
  setLoRaMode(HIGH, HIGH);
  delay(500);

  Serial.println("Konfiqurasiya göndərilir...");
  loraSerial.write(configBytes, sizeof(configBytes));
  delay(200);

  // Modulun cavabını oxu (adətən eyni bayt sırasını geri qaytarır - təsdiq üçün)
  delay(200);
  Serial.print("Cavab: ");
  while (loraSerial.available()) {
    byte b = loraSerial.read();
    Serial.print("0x");
    if (b < 0x10) Serial.print("0");
    Serial.print(b, HEX);
    Serial.print(" ");
  }
  Serial.println();

  // Konfiqurasiya bitdikdən sonra normal ötürmə rejiminə qayıt
  setLoRaMode(LOW, LOW);
  Serial.println("Normal rejimə qayıdıldı.");
}

void loop() {
  // boş
}