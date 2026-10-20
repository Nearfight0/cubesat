#include <SoftwareSerial.h>

SoftwareSerial loraSerial(2, 3); // RX=D2, TX=D3

void setup() {
  Serial.begin(9600);      // USB debug monitor
  loraSerial.begin(9600);  // E30 ilə eyni baud rate olmalıdır
  Serial.println("Ground station hazir, mesaj gozlenilir...");
}

void loop() {
  if (loraSerial.available()) {
    String incoming = loraSerial.readStringUntil('\n');
    Serial.print("Alindi: ");
    Serial.println(incoming);
  }
}