# Ground Station — 2nd edition (Python / PyQt6)

Serial logger I built as the ground station for the 2nd edition of the CubeSat (late 2025). It reads newline-terminated packets from the LoRa receiver, pretty-prints JSON, auto-plots every numeric field and exports the session to CSV.

## Features

- Background serial reader (`QThread`) with line buffering
- JSON detection and recursive extraction of numeric fields for live plotting (`pyqtgraph`)
- Timestamped log with millisecond precision, CSV export
- Configurable command buttons for sending text commands to the device
- Settings persisted in `~/.lora_logger/settings.json`

## Run

```bash
pip install -r requirements.txt
python main.py
```

On Windows, `run.bat` does the same.

Pick the COM port of the receiver, set the baud rate to **9600** and click **Connect**.

## Layout

```
main.py            entry point
core/              serial_manager, data_parser, data_logger
ui/                main window, log, chart, command and settings panels
config/settings.py persisted user settings
```

Sample logs from this edition are in [`docs/sample-data/`](../../docs/sample-data).
