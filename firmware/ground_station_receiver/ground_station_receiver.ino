/*
  ============================================================================
  Ground Station — Arduino Nano + E30 LoRa (SoftwareSerial D2/D3)
  ============================================================================
  1) Modulu konfiqurasiya edir (Teensy 4.1 / CubeSat terefi ile EYNI ayarlar:
     9600 UART, 1.2kbps air rate, max guc, channel 6, transparent mode)
  2) Sonra receive-only rejimde islayir: her telemetriya setrini (CSV)
     oldugu kimi Serial Monitor-a yazir.

  Wiring:
    E30 LoRa TXD -> Nano D2
    E30 LoRa RXD -> Nano D3
    E30 LoRa M0  -> Nano D4
    E30 LoRa M1  -> Nano D5
    Shared GND, ayri stabil qidalanma (VCC)
  ============================================================================
*/

#include <SoftwareSerial.h>

#define M0_PIN 4
#define M1_PIN 5

SoftwareSerial loraSerial(2, 3); // RX=D2 (modul TXD-dən), TX=D3 (modul RXD-yə)

// Ayni bayt massivi - Teensy/CubeSat terefinde istifade etdiyiniz kimi.
// 0xC2 = DAIMI yazma (EEPROM). Movcud ayarlariniza uygun deyisin.
byte configBytes[] = {0xC2, 0x00, 0x00, 0x19, 0x06, 0x44};
//                     baslq ADDH  ADDL  SPED  CHAN  OPTION

void setup() {
  Serial.begin(9600);

  pinMode(M0_PIN, OUTPUT);
  pinMode(M1_PIN, OUTPUT);

  loraSerial.begin(9600); // Modulun UART surati (SPED bitlerine uygun)

  configureModule();

  Serial.println("Ground station hazir, telemetriya gozlenilir...");
}

void loop() {
  if (loraSerial.available()) {
    String incoming = loraSerial.readStringUntil('\n');
    incoming.trim();
    if (incoming.length() > 0) {
      Serial.println(incoming);
    }
  }
}

// ---------------- Konfiqurasiya funksiyasi ----------------

void configureModule() {
  // Konfiqurasiya rejimine kecir
  digitalWrite(M0_PIN, HIGH);
  digitalWrite(M1_PIN, HIGH);
  delay(50);

  while (loraSerial.available()) loraSerial.read(); // bufferi temizle

  for (int i = 0; i < 6; i++) {
    loraSerial.write(configBytes[i]);
  }

  // Modulun cavabini gozle (6 bayt geri qaytarmalidir)
  unsigned long start = millis();
  int idx = 0;
  byte resp[6];
  while (idx < 6 && (millis() - start) < 1000) {
    if (loraSerial.available()) {
      resp[idx++] = loraSerial.read();
    }
  }

  Serial.print("Konfiqurasiya ");
  if (idx == 6) {
    Serial.println("teyid edildi:");
    for (int i = 0; i < 6; i++) {
      Serial.print("0x");
      if (resp[i] < 0x10) Serial.print("0");
      Serial.print(resp[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
  } else {
    Serial.println("teyid alinmadi! Baglantilari (RX/TX capraz, M0/M1) yoxlayin.");
  }

  // Normal (transparent) is rejimine qayit
  digitalWrite(M0_PIN, LOW);
  digitalWrite(M1_PIN, LOW);
  delay(50);
}
