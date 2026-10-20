"""
Settings Dialog
Configuration panel for application and LoRa parameters
"""

from PyQt6.QtWidgets import (QDialog, QVBoxLayout, QHBoxLayout, QLabel,
                              QLineEdit, QSpinBox, QDoubleSpinBox, QCheckBox,
                              QPushButton, QGroupBox, QFormLayout, QTabWidget,
                              QWidget)
from PyQt6.QtCore import Qt


class SettingsDialog(QDialog):
    """Dialog for configuring application settings"""
    
    def __init__(self, settings, parent=None):
        super().__init__(parent)
        self.settings = settings
        self.setWindowTitle("Settings")
        self.setMinimumWidth(500)
        self.setup_ui()
        self.load_current_settings()
    
    def setup_ui(self):
        """Initialize UI components"""
        layout = QVBoxLayout(self)
        
        # Tab widget for different setting categories
        tabs = QTabWidget()
        
        # Application settings tab
        app_tab = self.create_app_settings_tab()
        tabs.addTab(app_tab, "Application")
        
        # LoRa settings tab
        lora_tab = self.create_lora_settings_tab()
        tabs.addTab(lora_tab, "LoRa Parameters")
        
        layout.addWidget(tabs)
        
        # Buttons
        button_layout = QHBoxLayout()
        
        reset_btn = QPushButton("Reset to Defaults")
        reset_btn.clicked.connect(self.reset_defaults)
        button_layout.addWidget(reset_btn)
        
        button_layout.addStretch()
        
        save_btn = QPushButton("Save")
        save_btn.setDefault(True)
        save_btn.clicked.connect(self.save_and_close)
        button_layout.addWidget(save_btn)
        
        cancel_btn = QPushButton("Cancel")
        cancel_btn.clicked.connect(self.reject)
        button_layout.addWidget(cancel_btn)
        
        layout.addLayout(button_layout)
    
    def create_app_settings_tab(self):
        """Create application settings tab"""
        widget = QWidget()
        layout = QVBoxLayout(widget)
        
        # Display settings group
        display_group = QGroupBox("Display Settings")
        display_layout = QFormLayout()
        
        self.show_timestamps_cb = QCheckBox()
        display_layout.addRow("Show Timestamps:", self.show_timestamps_cb)
        
        self.auto_scroll_cb = QCheckBox()
        display_layout.addRow("Auto-scroll Log:", self.auto_scroll_cb)
        
        self.chart_enabled_cb = QCheckBox()
        display_layout.addRow("Enable Chart:", self.chart_enabled_cb)
        
        display_group.setLayout(display_layout)
        layout.addWidget(display_group)
        
        # Data settings group
        data_group = QGroupBox("Data Settings")
        data_layout = QFormLayout()
        
        self.max_entries_spin = QSpinBox()
        self.max_entries_spin.setRange(100, 100000)
        self.max_entries_spin.setSingleStep(1000)
        self.max_entries_spin.setSuffix(" entries")
        data_layout.addRow("Max Log Entries:", self.max_entries_spin)
        
        data_group.setLayout(data_layout)
        layout.addWidget(data_group)
        
        layout.addStretch()
        return widget
    
    def create_lora_settings_tab(self):
        """Create LoRa parameters tab"""
        widget = QWidget()
        layout = QVBoxLayout(widget)
        
        info_label = QLabel(
            "Note: These settings may need to be applied to the LoRa device\n"
            "using specific commands. Check your device documentation."
        )
        info_label.setStyleSheet("color: #666; font-style: italic;")
        info_label.setWordWrap(True)
        layout.addWidget(info_label)
        
        # LoRa parameters group
        lora_group = QGroupBox("LoRa Radio Parameters")
        lora_layout = QFormLayout()
        
        self.frequency_spin = QDoubleSpinBox()
        self.frequency_spin.setRange(137.0, 1020.0)
        self.frequency_spin.setSingleStep(0.1)
        self.frequency_spin.setSuffix(" MHz")
        self.frequency_spin.setDecimals(1)
        lora_layout.addRow("Frequency:", self.frequency_spin)
        
        self.sf_spin = QSpinBox()
        self.sf_spin.setRange(6, 12)
        lora_layout.addRow("Spreading Factor:", self.sf_spin)
        
        self.bandwidth_spin = QSpinBox()
        self.bandwidth_spin.setRange(7, 500)
        self.bandwidth_spin.setSuffix(" kHz")
        lora_layout.addRow("Bandwidth:", self.bandwidth_spin)
        
        self.coding_rate_spin = QSpinBox()
        self.coding_rate_spin.setRange(5, 8)
        lora_layout.addRow("Coding Rate (4/):", self.coding_rate_spin)
        
        self.tx_power_spin = QSpinBox()
        self.tx_power_spin.setRange(2, 20)
        self.tx_power_spin.setSuffix(" dBm")
        lora_layout.addRow("TX Power:", self.tx_power_spin)
        
        lora_group.setLayout(lora_layout)
        layout.addWidget(lora_group)
        
        layout.addStretch()
        return widget
    
    def load_current_settings(self):
        """Load current settings into UI"""
        # Application settings
        self.show_timestamps_cb.setChecked(self.settings.get('show_timestamps', True))
        self.auto_scroll_cb.setChecked(self.settings.get('auto_scroll', True))
        self.chart_enabled_cb.setChecked(self.settings.get('chart_enabled', True))
        self.max_entries_spin.setValue(self.settings.get('max_log_entries', 10000))
        
        # LoRa settings
        self.frequency_spin.setValue(self.settings.get('lora_frequency', 915.0))
        self.sf_spin.setValue(self.settings.get('lora_spreading_factor', 7))
        self.bandwidth_spin.setValue(self.settings.get('lora_bandwidth', 125))
        self.coding_rate_spin.setValue(self.settings.get('lora_coding_rate', 5))
        self.tx_power_spin.setValue(self.settings.get('lora_tx_power', 14))
    
    def save_and_close(self):
        """Save settings and close dialog"""
        # Application settings
        self.settings.set('show_timestamps', self.show_timestamps_cb.isChecked())
        self.settings.set('auto_scroll', self.auto_scroll_cb.isChecked())
        self.settings.set('chart_enabled', self.chart_enabled_cb.isChecked())
        self.settings.set('max_log_entries', self.max_entries_spin.value())
        
        # LoRa settings
        self.settings.set('lora_frequency', self.frequency_spin.value())
        self.settings.set('lora_spreading_factor', self.sf_spin.value())
        self.settings.set('lora_bandwidth', self.bandwidth_spin.value())
        self.settings.set('lora_coding_rate', self.coding_rate_spin.value())
        self.settings.set('lora_tx_power', self.tx_power_spin.value())
        
        self.accept()
    
    def reset_defaults(self):
        """Reset all settings to defaults"""
        from PyQt6.QtWidgets import QMessageBox
        
        reply = QMessageBox.question(
            self,
            "Reset Settings",
            "Are you sure you want to reset all settings to defaults?",
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No
        )
        
        if reply == QMessageBox.StandardButton.Yes:
            self.settings.reset_to_defaults()
            self.load_current_settings()