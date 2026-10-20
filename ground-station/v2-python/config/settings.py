"""
Application Settings
Manages configuration and user preferences
"""

import json
import os
from pathlib import Path


class Settings:
    """Application settings manager"""
    
    DEFAULT_COMMANDS = [
        "AT+RESET",
        "AT+PING",
        "AT+LED_ON",
        "AT+LED_OFF",
        "AT+STATUS",
        "AT+VERSION",
        "AT+CONFIG",
        "AT+TEST"
    ]
    
    DEFAULT_BAUDRATES = [
        300, 1200, 2400, 4800, 9600, 14400, 
        19200, 28800, 38400, 57600, 115200
    ]
    
    def __init__(self):
        self.config_dir = Path.home() / ".lora_logger"
        self.config_file = self.config_dir / "settings.json"
        
        # Default settings
        self.settings = {
            'last_port': None,
            'baudrate': 9600,
            'commands': self.DEFAULT_COMMANDS.copy(),
            'auto_scroll': True,
            'show_timestamps': True,
            'chart_enabled': True,
            'max_log_entries': 10000,
            'lora_frequency': 915.0,
            'lora_spreading_factor': 7,
            'lora_bandwidth': 125,
            'lora_coding_rate': 5,
            'lora_tx_power': 14
        }
        
        self.load_settings()
    
    def load_settings(self):
        """Load settings from file"""
        if self.config_file.exists():
            try:
                with open(self.config_file, 'r') as f:
                    loaded = json.load(f)
                    self.settings.update(loaded)
            except Exception as e:
                print(f"Error loading settings: {e}")
    
    def save_settings(self):
        """Save settings to file"""
        try:
            self.config_dir.mkdir(parents=True, exist_ok=True)
            with open(self.config_file, 'w') as f:
                json.dump(self.settings, f, indent=2)
            return True
        except Exception as e:
            print(f"Error saving settings: {e}")
            return False
    
    def get(self, key, default=None):
        """Get setting value"""
        return self.settings.get(key, default)
    
    def set(self, key, value):
        """Set setting value"""
        self.settings[key] = value
        self.save_settings()
    
    def get_commands(self):
        """Get configured commands"""
        return self.settings.get('commands', self.DEFAULT_COMMANDS.copy())
    
    def set_commands(self, commands):
        """Set command list"""
        self.settings['commands'] = commands
        self.save_settings()
    
    def reset_to_defaults(self):
        """Reset all settings to defaults"""
        self.settings = {
            'last_port': None,
            'baudrate': 9600,
            'commands': self.DEFAULT_COMMANDS.copy(),
            'auto_scroll': True,
            'show_timestamps': True,
            'chart_enabled': True,
            'max_log_entries': 10000,
            'lora_frequency': 915.0,
            'lora_spreading_factor': 7,
            'lora_bandwidth': 125,
            'lora_coding_rate': 5,
            'lora_tx_power': 14
        }
        self.save_settings()