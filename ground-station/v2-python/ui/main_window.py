"""
Main Window
Primary application window integrating all components
"""

from PyQt6.QtWidgets import (QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
                              QPushButton, QComboBox, QLabel, QSplitter,
                              QFileDialog, QMessageBox, QStatusBar)
from PyQt6.QtCore import Qt, pyqtSlot
from datetime import datetime

from core.serial_manager import SerialManager
from core.data_parser import DataParser
from core.data_logger import DataLogger
from config.settings import Settings
from ui.log_panel import LogPanel
from ui.chart_panel import ChartPanel
from ui.command_panel import CommandPanel
from ui.settings_dialog import SettingsDialog


class MainWindow(QMainWindow):
    """Main application window"""
    
    def __init__(self):
        super().__init__()
        self.setWindowTitle("LoRa Serial Logger")
        self.setMinimumSize(1200, 800)
        
        # Core components
        self.serial_manager = SerialManager()
        self.data_parser = DataParser()
        self.data_logger = DataLogger()
        self.settings = Settings()
        
        # Setup UI
        self.setup_ui()
        self.setup_connections()
        
        # Load saved settings
        self.apply_settings()
    
    def setup_ui(self):
        """Initialize all UI components"""
        # Central widget
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        main_layout = QVBoxLayout(central_widget)
        
        # Create UI components first (before top bar)
        # Log panel
        self.log_panel = LogPanel()
        
        # Chart panel
        self.chart_panel = ChartPanel()
        
        # Command panel
        self.command_panel = CommandPanel(self.settings.get_commands())
        self.command_panel.set_enabled(False)
        
        # Now create top bar (which references chart_panel)
        top_bar = self.create_top_bar()
        main_layout.addLayout(top_bar)
        
        # Main content area - Log and Chart
        content_splitter = QSplitter(Qt.Orientation.Vertical)
        content_splitter.addWidget(self.log_panel)
        content_splitter.addWidget(self.chart_panel)
        
        # Set initial splitter sizes (70% log, 30% chart)
        content_splitter.setSizes([700, 300])
        
        main_layout.addWidget(content_splitter)
        
        # Add command panel at bottom
        main_layout.addWidget(self.command_panel)
        
        # Status bar
        self.statusBar = QStatusBar()
        self.setStatusBar(self.statusBar)
        self.statusBar.showMessage("Ready")
    
    def create_top_bar(self):
        """Create top control bar"""
        layout = QHBoxLayout()
        
        # Port selection
        port_label = QLabel("COM Port:")
        layout.addWidget(port_label)
        
        self.port_combo = QComboBox()
        self.port_combo.setMinimumWidth(150)
        self.refresh_ports()
        layout.addWidget(self.port_combo)
        
        # Refresh ports button
        refresh_btn = QPushButton("↻")
        refresh_btn.setMaximumWidth(40)
        refresh_btn.setToolTip("Refresh port list")
        refresh_btn.clicked.connect(self.refresh_ports)
        layout.addWidget(refresh_btn)
        
        # Baudrate selection
        baud_label = QLabel("Baudrate:")
        layout.addWidget(baud_label)
        
        self.baud_combo = QComboBox()
        for rate in Settings.DEFAULT_BAUDRATES:
            self.baud_combo.addItem(str(rate), rate)
        default_baud = self.settings.get('baudrate', 9600)
        index = self.baud_combo.findData(default_baud)
        if index >= 0:
            self.baud_combo.setCurrentIndex(index)
        layout.addWidget(self.baud_combo)
        
        # Connect button
        self.connect_btn = QPushButton("Connect")
        self.connect_btn.setMinimumWidth(100)
        self.connect_btn.setStyleSheet("""
            QPushButton {
                background-color: #2e7d32;
                color: white;
                border: none;
                border-radius: 4px;
                padding: 5px 15px;
                font-weight: bold;
            }
            QPushButton:hover {
                background-color: #388e3c;
            }
            QPushButton:pressed {
                background-color: #1b5e20;
            }
        """)
        self.connect_btn.clicked.connect(self.toggle_connection)
        layout.addWidget(self.connect_btn)
        
        layout.addStretch()
        
        # Action buttons
        self.save_csv_btn = QPushButton("Save CSV")
        self.save_csv_btn.clicked.connect(self.save_csv)
        layout.addWidget(self.save_csv_btn)
        
        clear_chart_btn = QPushButton("Clear Chart")
        clear_chart_btn.clicked.connect(self.chart_panel.clear_chart)
        layout.addWidget(clear_chart_btn)
        
        settings_btn = QPushButton("Settings")
        settings_btn.clicked.connect(self.open_settings)
        layout.addWidget(settings_btn)
        
        return layout
    
    def setup_connections(self):
        """Setup signal-slot connections"""
        # Serial manager signals
        self.serial_manager.data_received.connect(self.on_data_received)
        self.serial_manager.connection_changed.connect(self.on_connection_changed)
        self.serial_manager.error_occurred.connect(self.on_error)
        
        # Command panel signals
        self.command_panel.command_triggered.connect(self.send_command)
    
    def apply_settings(self):
        """Apply loaded settings to UI"""
        self.log_panel.set_show_timestamps(self.settings.get('show_timestamps', True))
        self.log_panel.set_auto_scroll(self.settings.get('auto_scroll', True))
        
        chart_enabled = self.settings.get('chart_enabled', True)
        self.chart_panel.setVisible(chart_enabled)
    
    def refresh_ports(self):
        """Refresh available COM ports"""
        current_port = self.port_combo.currentText()
        self.port_combo.clear()
        
        ports = SerialManager.get_available_ports()
        if ports:
            self.port_combo.addItems(ports)
            
            # Try to restore previous selection
            index = self.port_combo.findText(current_port)
            if index >= 0:
                self.port_combo.setCurrentIndex(index)
            else:
                # Try to select last used port
                last_port = self.settings.get('last_port')
                if last_port:
                    index = self.port_combo.findText(last_port)
                    if index >= 0:
                        self.port_combo.setCurrentIndex(index)
        else:
            self.port_combo.addItem("No ports available")
            self.connect_btn.setEnabled(False)
    
    def toggle_connection(self):
        """Connect or disconnect from serial port"""
        if self.serial_manager.is_connected():
            self.disconnect_serial()
        else:
            self.connect_serial()
    
    def connect_serial(self):
        """Establish serial connection"""
        port = self.port_combo.currentText()
        if not port or port == "No ports available":
            QMessageBox.warning(self, "Error", "Please select a valid COM port")
            return
        
        baudrate = self.baud_combo.currentData()
        
        if self.serial_manager.connect(port, baudrate):
            self.settings.set('last_port', port)
            self.settings.set('baudrate', baudrate)
            self.statusBar.showMessage(f"Connected to {port} @ {baudrate} baud")
        else:
            QMessageBox.critical(self, "Connection Error", 
                               f"Failed to connect to {port}")
    
    def disconnect_serial(self):
        """Disconnect from serial port"""
        self.serial_manager.disconnect()
        self.statusBar.showMessage("Disconnected")
    
    @pyqtSlot(str)
    def on_data_received(self, data):
        """Handle incoming serial data"""
        # Add to logger
        self.data_logger.add_entry(data)
        
        # Display in log panel
        self.log_panel.append_data(data)
        
        # Parse data
        parsed = self.data_parser.parse_data(data)
        
        # Update chart if numeric data available
        if parsed['numeric_values']:
            self.chart_panel.add_data_point(parsed['numeric_values'])
    
    @pyqtSlot(bool)
    def on_connection_changed(self, connected):
        """Handle connection state changes"""
        if connected:
            self.connect_btn.setText("Disconnect")
            self.connect_btn.setStyleSheet("""
                QPushButton {
                    background-color: #c62828;
                    color: white;
                    border: none;
                    border-radius: 4px;
                    padding: 5px 15px;
                    font-weight: bold;
                }
                QPushButton:hover {
                    background-color: #d32f2f;
                }
                QPushButton:pressed {
                    background-color: #b71c1c;
                }
            """)
            self.port_combo.setEnabled(False)
            self.baud_combo.setEnabled(False)
            self.command_panel.set_enabled(True)
        else:
            self.connect_btn.setText("Connect")
            self.connect_btn.setStyleSheet("""
                QPushButton {
                    background-color: #2e7d32;
                    color: white;
                    border: none;
                    border-radius: 4px;
                    padding: 5px 15px;
                    font-weight: bold;
                }
                QPushButton:hover {
                    background-color: #388e3c;
                }
                QPushButton:pressed {
                    background-color: #1b5e20;
                }
            """)
            self.port_combo.setEnabled(True)
            self.baud_combo.setEnabled(True)
            self.command_panel.set_enabled(False)
    
    @pyqtSlot(str)
    def on_error(self, error_msg):
        """Handle errors from serial manager"""
        self.statusBar.showMessage(f"Error: {error_msg}")
        QMessageBox.warning(self, "Error", error_msg)
    
    def send_command(self, command):
        """Send command via serial"""
        if self.serial_manager.send_command(command):
            self.statusBar.showMessage(f"Sent: {command}")
            self.log_panel.append_data(f">>> {command}")
        else:
            self.statusBar.showMessage(f"Failed to send: {command}")
    
    def save_csv(self):
        """Save logged data to CSV file"""
        if self.data_logger.get_entry_count() == 0:
            QMessageBox.information(self, "No Data", 
                                  "No data available to export")
            return
        
        # Get filename from user
        timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
        default_name = f"lora_log_{timestamp}.csv"
        
        filename, _ = QFileDialog.getSaveFileName(
            self,
            "Save CSV",
            default_name,
            "CSV Files (*.csv);;All Files (*)"
        )
        
        if filename:
            success, message = self.data_logger.export_to_csv(filename)
            
            if success:
                QMessageBox.information(self, "Success", message)
                self.statusBar.showMessage(message)
            else:
                QMessageBox.critical(self, "Error", message)
    
    def open_settings(self):
        """Open settings dialog"""
        dialog = SettingsDialog(self.settings, self)
        if dialog.exec() == SettingsDialog.DialogCode.Accepted:
            self.apply_settings()
            
            # Update command panel if commands changed
            self.command_panel.set_commands(self.settings.get_commands())
    
    def closeEvent(self, event):
        """Handle window close event"""
        # Disconnect serial if connected
        if self.serial_manager.is_connected():
            self.serial_manager.disconnect()
        
        # Save command configuration
        self.settings.set_commands(self.command_panel.get_commands())
        
        event.accept()