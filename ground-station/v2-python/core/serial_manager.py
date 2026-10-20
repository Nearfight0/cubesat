"""
Serial Manager
Handles all serial port communication with LoRa device
"""

import serial
import serial.tools.list_ports
from PyQt6.QtCore import QThread, pyqtSignal, QMutex
import time


class SerialManager(QThread):
    """Thread-safe serial communication manager"""
    
    # Signals
    data_received = pyqtSignal(str)  # Emitted when data arrives
    connection_changed = pyqtSignal(bool)  # Emitted on connect/disconnect
    error_occurred = pyqtSignal(str)  # Emitted on error
    
    def __init__(self):
        super().__init__()
        self.serial_port = None
        self.is_running = False
        self.mutex = QMutex()
        self.port_name = None
        self.baudrate = 9600
        
    @staticmethod
    def get_available_ports():
        """Get list of available COM ports"""
        ports = serial.tools.list_ports.comports()
        return [port.device for port in ports]
    
    def connect(self, port_name, baudrate=9600):
        """Connect to specified serial port"""
        self.port_name = port_name
        self.baudrate = baudrate
        
        try:
            self.serial_port = serial.Serial(
                port=port_name,
                baudrate=baudrate,
                timeout=1,
                write_timeout=1
            )
            self.is_running = True
            self.start()  # Start the reading thread
            self.connection_changed.emit(True)
            return True
        except serial.SerialException as e:
            self.error_occurred.emit(f"Connection failed: {str(e)}")
            return False
    
    def disconnect(self):
        """Disconnect from serial port"""
        self.is_running = False
        
        if self.serial_port and self.serial_port.is_open:
            try:
                self.serial_port.close()
            except Exception as e:
                self.error_occurred.emit(f"Disconnect error: {str(e)}")
        
        self.wait()  # Wait for thread to finish
        self.connection_changed.emit(False)
    
    def send_command(self, command):
        """Send command to serial port"""
        self.mutex.lock()
        try:
            if self.serial_port and self.serial_port.is_open:
                # Ensure command ends with newline
                if not command.endswith('\n'):
                    command += '\n'
                self.serial_port.write(command.encode('utf-8'))
                self.serial_port.flush()
                return True
            else:
                self.error_occurred.emit("Not connected to serial port")
                return False
        except Exception as e:
            self.error_occurred.emit(f"Send error: {str(e)}")
            return False
        finally:
            self.mutex.unlock()
    
    def run(self):
        """Main thread loop for reading serial data"""
        buffer = ""
        
        while self.is_running:
            try:
                if self.serial_port and self.serial_port.is_open:
                    # Read available bytes
                    if self.serial_port.in_waiting > 0:
                        data = self.serial_port.read(
                            self.serial_port.in_waiting
                        ).decode('utf-8', errors='ignore')
                        
                        buffer += data
                        
                        # Process complete lines
                        while '\n' in buffer:
                            line, buffer = buffer.split('\n', 1)
                            line = line.strip()
                            if line:
                                self.data_received.emit(line)
                    else:
                        time.sleep(0.01)  # Small delay to prevent CPU spinning
                else:
                    break
                    
            except serial.SerialException as e:
                self.error_occurred.emit(f"Read error: {str(e)}")
                self.is_running = False
                break
            except Exception as e:
                self.error_occurred.emit(f"Unexpected error: {str(e)}")
                time.sleep(0.1)
        
        # Cleanup
        if self.serial_port and self.serial_port.is_open:
            self.serial_port.close()
        self.connection_changed.emit(False)
    
    def is_connected(self):
        """Check if currently connected"""
        return self.serial_port and self.serial_port.is_open