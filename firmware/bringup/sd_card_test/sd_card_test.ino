/*
  Minimal SD card test for Teensy 4.1 built-in SD slot.
  Upload this alone (nothing else) to check if the SD card is detected
  at all, independent of the rest of the CubeSat firmware.
*/

#include <SD.h>

void setup() {
  Serial.begin(115200);
  unsigned long t0 = millis();
  while (!Serial && millis() - t0 < 3000) delay(10);

  Serial.println("Testing SD card...");

  if (!SD.begin(BUILTIN_SDCARD)) {
    Serial.println("SD.begin() FAILED — card not detected.");
    Serial.println("Check: card seated properly, formatted FAT32 (not exFAT), card itself working in another device.");
    return;
  }

  Serial.println("SD.begin() OK — card detected!");

  File testFile = SD.open("test.txt", FILE_WRITE);
  if (testFile) {
    testFile.println("Hello from Teensy 4.1!");
    testFile.close();
    Serial.println("Wrote test.txt successfully.");
  } else {
    Serial.println("Failed to open test.txt for writing.");
  }

  File readBack = SD.open("test.txt");
  if (readBack) {
    Serial.println("Reading test.txt back:");
    while (readBack.available()) {
      Serial.write(readBack.read());
    }
    readBack.close();
  }
}

void loop() {
  // nothing
}
