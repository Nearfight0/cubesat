"""
Log Panel
Displays real-time incoming data with timestamps
"""

from PyQt6.QtWidgets import (QWidget, QVBoxLayout, QTextEdit, 
                              QPushButton, QHBoxLayout, QLabel)
from PyQt6.QtCore import Qt, pyqtSlot
from PyQt6.QtGui import QFont, QTextCursor
from datetime import datetime


class LogPanel(QWidget):
    """Widget for displaying real-time log data"""
    
    def __init__(self, parent=None):
        super().__init__(parent)
        self.show_timestamps = True
        self.auto_scroll = True
        self.max_lines = 1000
        self.setup_ui()
    
    def setup_ui(self):
        """Initialize UI components"""
        layout = QVBoxLayout(self)
        
        # Header with title and entry count
        header_layout = QHBoxLayout()
        
        title_label = QLabel("Real-time Data Log")
        title_label.setStyleSheet("font-weight: bold; font-size: 12px;")
        header_layout.addWidget(title_label)
        
        self.entry_count_label = QLabel("Entries: 0")
        self.entry_count_label.setStyleSheet("color: #666;")
        header_layout.addWidget(self.entry_count_label)
        
        header_layout.addStretch()
        
        # Clear button
        self.clear_btn = QPushButton("Clear Log")
        self.clear_btn.setMaximumWidth(100)
        self.clear_btn.clicked.connect(self.clear_log)
        header_layout.addWidget(self.clear_btn)
        
        layout.addLayout(header_layout)
        
        # Log text display
        self.log_text = QTextEdit()
        self.log_text.setReadOnly(True)
        self.log_text.setFont(QFont("Consolas", 9))
        self.log_text.setLineWrapMode(QTextEdit.LineWrapMode.NoWrap)
        
        # Style the log area
        self.log_text.setStyleSheet("""
            QTextEdit {
                background-color: #1e1e1e;
                color: #d4d4d4;
                border: 1px solid #3e3e3e;
                border-radius: 4px;
                padding: 5px;
            }
        """)
        
        layout.addWidget(self.log_text)
        
        self.entry_count = 0
    
    @pyqtSlot(str)
    def append_data(self, data):
        """
        Append new data to log
        Args:
            data: Raw data string to display
        """
        self.entry_count += 1
        self.entry_count_label.setText(f"Entries: {self.entry_count}")
        
        # Format entry with timestamp
        if self.show_timestamps:
            timestamp = datetime.now().strftime('%H:%M:%S.%f')[:-3]
            entry = f"[{timestamp}] {data}"
        else:
            entry = data
        
        # Add to log
        self.log_text.append(entry)
        
        # Limit number of lines
        self._trim_log()
        
        # Auto-scroll to bottom
        if self.auto_scroll:
            cursor = self.log_text.textCursor()
            cursor.movePosition(QTextCursor.MoveOperation.End)
            self.log_text.setTextCursor(cursor)
    
    def _trim_log(self):
        """Keep log size under max_lines"""
        doc = self.log_text.document()
        if doc.blockCount() > self.max_lines:
            cursor = QTextCursor(doc)
            cursor.movePosition(QTextCursor.MoveOperation.Start)
            cursor.movePosition(
                QTextCursor.MoveOperation.Down, 
                QTextCursor.MoveMode.KeepAnchor,
                doc.blockCount() - self.max_lines
            )
            cursor.removeSelectedText()
    
    def clear_log(self):
        """Clear all log content"""
        self.log_text.clear()
        self.entry_count = 0
        self.entry_count_label.setText("Entries: 0")
    
    def set_show_timestamps(self, enabled):
        """Toggle timestamp display"""
        self.show_timestamps = enabled
    
    def set_auto_scroll(self, enabled):
        """Toggle auto-scroll"""
        self.auto_scroll = enabled
    
    def get_log_text(self):
        """Get all log text"""
        return self.log_text.toPlainText()