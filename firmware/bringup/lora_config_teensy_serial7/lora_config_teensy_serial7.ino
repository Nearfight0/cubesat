// Teensy 4.1-də SoftwareSerial lazım deyil, birbaşa Serial7 (TX7 / RX7) istifadə edirik.

// C0 (Save) | 00 00 (Address) | 1A (9600 / 2.4k) | 06 (Channel 6 = 166 MHz) | 44 (20dBm, FEC ON, Transparent)
byte configBytes[] = {0xC0, 0x00, 0x00, 0x1A, 0x06, 0x44};

void setup() {
  Serial.begin(9600);   // Kompüterlə əlaqə üçün (Serial Monitor)
  Serial7.begin(9600);  // E30 modulu üçün (TX7 / RX7)
  
  // Teensy-də Serial portun açılması üçün qısa gözləmə əlavə etmək faydalıdır
  while (!Serial && millis() < 3000); 
  
  delay(1000);
  Serial.println("E30 Modulu Serial7 (TX7/RX7) uzerinden konfiqurasiya edilir...");

  // Konfiqurasiya əmrini E30 moduluna göndəririk
  Serial7.write(configBytes, sizeof(configBytes));
  
  delay(500);

  // Moduldan gələn cavabı oxuyub Serial Monitor-a yazdırırıq
  Serial.print("Moduldan gelen cavab: ");
  while (Serial7.available()) {
    byte b = Serial7.read();
    Serial.print("0x");
    if (b < 16) Serial.print("0");
    Serial.print(b, HEX);
    Serial.print(" ");
  }
  Serial.println();
  Serial.println("Ayarlama tamamlandi! Normal data rejimine kecmek ucun M0=0, M1=0 edin.");
}

void loop() {
}