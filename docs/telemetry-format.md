# Telemetry format (3rd edition)

One ASCII line per packet, comma-separated, terminated by `\r\n`, sent at **1 Hz**.
The same line goes to the LoRa radio, the USB debug port and the SD card log.

```
<TEAM_ID>,<PACKET_NO>,<TIME>,<PRESSURE>,<REL_ALTITUDE>,<DESCENT_SPEED>,<VOLTAGE>,
<GPS_LAT>,<GPS_LNG>,<GPS_ALT>,<PITCH>,<ROLL>,<YAW>,<SEPARATION>,<CRYPTO_MSG>
```

| # | Field | Unit / format | Source |
|---|---|---|---|
| 0 | `TEAM_ID` | 5-character string | compile-time constant |
| 1 | `PACKET_NO` | integer, persists across reboots | EEPROM |
| 2 | `TIME` | `HH:MM:SS`, GPS UTC after first fix, otherwise time since boot | BN220 / `millis()` |
| 3 | `PRESSURE` | hPa, 2 decimals | BMP280 |
| 4 | `REL_ALTITUDE` | m above the launch point | BMP280, ground-zero calibrated at boot |
| 5 | `DESCENT_SPEED` | m/s, positive = descending | Δaltitude / Δt |
| 6 | `VOLTAGE` | V | resistor divider on `A0` |
| 7 | `GPS_LAT` | decimal degrees, 6 decimals | BN220 |
| 8 | `GPS_LNG` | decimal degrees, 6 decimals | BN220 |
| 9 | `GPS_ALT` | m MSL | BN220 |
| 10 | `PITCH` | degrees | BNO086 |
| 11 | `ROLL` | degrees | BNO086 |
| 12 | `YAW` | degrees, relative to the start-up heading, −180…180 | BNO086 |
| 13 | `SEPARATION` | `0` / `1` | placeholder, always `0` |
| 14 | `CRYPTO_MSG` | string | placeholder, always `NULL` |

Example from `sample-data/v3-bench-test.csv`:

```
00000,4921,00:00:01,1004.6000,-0.2100,0.6100,5.0700,0.00000000,0.00000000,0.0000,0.6100,1.2300,-0.0300,0,NULL
```

(This bench test ran without a GPS fix and on bench power, so GPS is zero and the voltage is below the battery range.)

## 2nd edition

The 2nd edition used JSON objects, one per line, for example:

```json
{"T":24.82,"H":69.06,"P":1021.77,"GAS":66.64,"ALT":-70.78,"Y":-0.64,"X":0.49,"Z":-0.00,"SERVO":"CLOSED","LAT":0.0,"LON":0.0}
```
