# Firmware

All sketches build in the Arduino IDE. The Teensy sketches also need [Teensyduino](https://www.pjrc.com/teensy/td_download.html).

## `flight_computer/` — Teensy 4.1

The integrated flight firmware. It reads every sensor, builds the [telemetry packet](../docs/telemetry-format.md) and sends it over LoRa once per second.

**Libraries** (Arduino Library Manager): `Adafruit BNO08x`, `Adafruit BMP280 Library`, `TinyGPSPlus`. `SD` and `EEPROM` ship with Teensyduino.

Before flight:
1. Set `TEAM_ID` at the top of the file.
2. Measure the divider resistors with a multimeter and update `BATT_R1` / `BATT_R2`.
3. Calibrate the IMU with `bringup/bno086_calibration`, or send `s` over USB serial once accuracy reaches 3.

| Peripheral | Teensy pins |
|---|---|
| BNO086, BMP280 | `Wire` (SDA 18, SCL 19) |
| BN220 GPS | `Serial2` (RX2 7, TX2 8) |
| E30 LoRa | `Serial7` (RX7 28, TX7 29) |
| Battery divider | `A0` |
| SD card | built-in slot |

## `ground_station_receiver/` — Arduino Nano

On boot, the sketch puts the E30 into configuration mode (M0 = M1 = HIGH), writes the radio parameters and checks the module's echo. It then switches to transparent mode and forwards every received line to USB. The Qt ground station reads from that USB port.

| E30 pin | Nano pin |
|---|---|
| TXD | D2 |
| RXD | D3 |
| M0 | D4 |
| M1 | D5 |

## `bringup/`

Small sketches used to bring each part up on its own before integration:

| Sketch | Purpose |
|---|---|
| `bno086_calibration` | Live pitch/roll/yaw with the accuracy status; press `s` to save calibration to the sensor's flash |
| `bmp280_test` | Pressure, temperature and altitude readout |
| `ina219_test` | Current/power monitor test (evaluated, not used in the final build) |
| `sd_card_test` | Checks the Teensy SD slot (card must be FAT32) |
| `battery_voltage_lora` | Divider-based battery voltage sent over LoRa |
| `lora_config_teensy` | Configures the E30 from the Teensy using M0/M1 pins |
| `lora_config_teensy_serial7` | Earlier variant that sends the config bytes on `Serial7` |
| `lora_config_nano` | Configures the E30 from the Nano |
| `ground_station_rx_test` | Minimal Nano receiver that prints whatever arrives |
