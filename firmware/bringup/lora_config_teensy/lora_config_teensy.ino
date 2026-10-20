#define M0_PIN 30  // öz bağladığın pinlərlə əvəz et
#define M1_PIN 31  // öz bağladığın pinlərlə əvəz et

byte configBytes[] = {0xC0, 0x00, 0x00, 0x00, 0x06, 0x44}; // eyni massiv!

void setLoRaMode(bool m0, bool m1) {
  digitalWrite(M0_PIN, m0 ? HIGH : LOW);
  digitalWrite(M1_PIN, m1 ? HIGH : LOW);
  delay(50);
}

void setup() {
  Serial.begin(9600);
  Serial7.begin(9600); // config rejimi həmişə 9600
  pinMode(M0_PIN, OUTPUT);
  pinMode(M1_PIN, OUTPUT);

  setLoRaMode(HIGH, HIGH);
  delay(500);

  Serial.println("Konfiqurasiya gonderilir...");
  Serial7.write(configBytes, sizeof(configBytes));
  delay(200);

  Serial.print("Cavab: ");
  while (Serial7.available()) {
    byte b = Serial7.read();
    Serial.print("0x");
    if (b < 0x10) Serial.print("0");
    Serial.print(b, HEX);
    Serial.print(" ");
  }
  Serial.println();

  setLoRaMode(LOW, LOW);
  Serial.println("Normal rejime qayidildi.");
}

void loop() {}