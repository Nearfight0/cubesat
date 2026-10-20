# Ground Station — 3rd edition (Qt6 / C++17)

Desktop ground station for mission *Kripto*. It receives the 15-field telemetry packet from the LoRa receiver over USB serial, then displays, plots, maps and logs it.

![screenshot](../../docs/images/ground-station-v3.png)

## Architecture

| Class | Responsibility |
|---|---|
| `SerialManager` | Opens the COM port, splits the stream into lines, scans ports in the background with `QtConcurrent` |
| `TelemetryParser` | Validates and converts one line into a `TelemetryPacket`, with field-level error messages |
| `TelemetryPacket` | Value type for one frame, plus CSV serialisation |
| `CsvLogger` | Writes `<TEAM_ID>.csv` in the working directory |
| `SimulationManager` | Replays a recorded CSV through the same `packetReceived` signal as live serial (Idle → Loaded → Playing ⇄ Paused) |
| `ChartManager` | Rolling Qt Charts for altitude, descent speed, voltage, pressure and orientation |
| `MapView` | Offline GPS track drawn with `QPainter` |
| `CameraPanel` | Live view, snapshot and recording from a UVC capture device (analog VTX receiver), via QtMultimedia |
| `MainWindow` | Wires everything together: tabs, telemetry dock, command panel, status bar |

## Build

Requires **Qt 6** with `SerialPort`, `Charts`, `Multimedia` and `Concurrent`, plus CMake ≥ 3.21.

**Windows (Qt Online Installer, MinGW):**
```bash
cmake -B build -G Ninja -DCMAKE_PREFIX_PATH=C:/Qt/6.x.x/mingw_64
cmake --build build
windeployqt build/AerokosmikGroundStation.exe
```

**Linux (Debian/Ubuntu):**
```bash
sudo apt install cmake ninja-build qt6-base-dev qt6-serialport-dev qt6-charts-dev qt6-multimedia-dev
cmake -B build -G Ninja && cmake --build build
./build/AerokosmikGroundStation
```

## Try it without hardware

Open the **Simulation** tab, click **Browse…** and pick [`docs/sample-data/v3-bench-test.csv`](../../docs/sample-data/v3-bench-test.csv).

## Known limitations

- The **Send** (crypto word) and **Trigger Separation** controls only write to the command log. They don't transmit over serial yet.
- `prototypes/analog_camera_viewer.py` is the earlier OpenCV/Tkinter camera viewer that `CameraPanel` replaced. Its HUD figures (altitude, heading) are decorative placeholders, not telemetry.
