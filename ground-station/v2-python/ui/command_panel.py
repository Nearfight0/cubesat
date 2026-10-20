"""
Command Panel
Configurable buttons for sending commands to LoRa device
"""

from PyQt6.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, 
                              QPushButton, QLabel, QInputDialog, QMessageBox)
from PyQt6.QtCore import pyqtSignal


class CommandPanel(QWidget):
    """Widget with configurable command buttons"""
    
    # Signal emitted when a command should be sent
    command_triggered = pyqtSignal(str)
    
    def __init__(self, commands=None, parent=None):
        super().__init__(parent)
        self.commands = commands or []
        self.command_buttons = []
        self.setup_ui()
    
    def setup_ui(self):
        """Initialize UI components"""
        layout = QVBoxLayout(self)
        
        # Header
        header_layout = QHBoxLayout()
        title_label = QLabel("Quick Commands")
        title_label.setStyleSheet("font-weight: bold; font-size: 12px;")
        header_layout.addWidget(title_label)
        header_layout.addStretch()
        
        # Configure button
        config_btn = QPushButton("Configure")
        config_btn.setMaximumWidth(100)
        config_btn.clicked.connect(self.configure_commands)
        header_layout.addWidget(config_btn)
        
        layout.addLayout(header_layout)
        
        # Command buttons container
        self.buttons_layout = QHBoxLayout()
        layout.addLayout(self.buttons_layout)
        
        # Create initial buttons
        self.create_command_buttons()
    
    def create_command_buttons(self):
        """Create buttons for each configured command"""
        # Clear existing buttons
        for button in self.command_buttons:
            button.deleteLater()
        self.command_buttons.clear()
        
        # Create new buttons
        for i, command in enumerate(self.commands):
            btn = QPushButton(command)
            btn.setMinimumHeight(35)
            btn.setStyleSheet("""
                QPushButton {
                    background-color: #0d47a1;
                    color: white;
                    border: none;
                    border-radius: 4px;
                    padding: 5px 15px;
                    font-weight: bold;
                }
                QPushButton:hover {
                    background-color: #1565c0;
                }
                QPushButton:pressed {
                    background-color: #0a3d91;
                }
                QPushButton:disabled {
                    background-color: #424242;
                    color: #888;
                }
            """)
            
            # Connect to send command
            btn.clicked.connect(lambda checked, cmd=command: self.send_command(cmd))
            
            self.buttons_layout.addWidget(btn)
            self.command_buttons.append(btn)
        
        # Add stretch to push buttons to the left
        self.buttons_layout.addStretch()
    
    def send_command(self, command):
        """Emit signal to send command"""
        self.command_triggered.emit(command)
    
    def configure_commands(self):
        """Open dialog to configure command list"""
        from PyQt6.QtWidgets import QDialog, QVBoxLayout, QLineEdit, QPushButton
        
        dialog = QDialog(self)
        dialog.setWindowTitle("Configure Commands")
        dialog.setMinimumWidth(400)
        
        layout = QVBoxLayout(dialog)
        
        info_label = QLabel("Enter commands (one per line):")
        layout.addWidget(info_label)
        
        # Text area for commands
        text_edit = QLineEdit()
        text_edit.setText('\n'.join(self.commands))
        
        # Create list of input fields
        inputs = []
        for i, cmd in enumerate(self.commands + [''] * (8 - len(self.commands))):
            input_field = QLineEdit()
            input_field.setPlaceholderText(f"Command {i+1}")
            input_field.setText(cmd)
            layout.addWidget(input_field)
            inputs.append(input_field)
        
        # Buttons
        button_layout = QHBoxLayout()
        
        save_btn = QPushButton("Save")
        save_btn.clicked.connect(dialog.accept)
        button_layout.addWidget(save_btn)
        
        cancel_btn = QPushButton("Cancel")
        cancel_btn.clicked.connect(dialog.reject)
        button_layout.addWidget(cancel_btn)
        
        layout.addLayout(button_layout)
        
        # Show dialog
        if dialog.exec() == QDialog.DialogCode.Accepted:
            # Collect non-empty commands
            new_commands = [inp.text().strip() for inp in inputs if inp.text().strip()]
            
            if new_commands:
                self.set_commands(new_commands)
            else:
                QMessageBox.warning(self, "Warning", "At least one command is required.")
    
    def set_commands(self, commands):
        """Update command list and recreate buttons"""
        self.commands = commands
        self.create_command_buttons()
    
    def get_commands(self):
        """Get current command list"""
        return self.commands.copy()
    
    def set_enabled(self, enabled):
        """Enable/disable all command buttons"""
        for button in self.command_buttons:
            button.setEnabled(enabled)