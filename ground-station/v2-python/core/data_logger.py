"""
Data Logger
Manages data logging and CSV export functionality
"""

import csv
from datetime import datetime
from collections import deque


class DataLogger:
    """Handles data logging and CSV export"""
    
    def __init__(self, max_entries=10000):
        """
        Initialize data logger
        Args:
            max_entries: Maximum number of entries to keep in memory
        """
        self.log_entries = deque(maxlen=max_entries)
        self.max_entries = max_entries
    
    def add_entry(self, data, timestamp=None):
        """
        Add new data entry with timestamp
        Args:
            data: Raw data string
            timestamp: Optional timestamp (uses current time if None)
        """
        if timestamp is None:
            timestamp = datetime.now()
        
        entry = {
            'timestamp': timestamp,
            'data': data
        }
        
        self.log_entries.append(entry)
    
    def clear_log(self):
        """Clear all logged entries"""
        self.log_entries.clear()
    
    def get_entry_count(self):
        """Get number of logged entries"""
        return len(self.log_entries)
    
    def export_to_csv(self, filename):
        """
        Export logged data to CSV file
        Args:
            filename: Output CSV file path
        Returns:
            Tuple of (success: bool, message: str)
        """
        if not self.log_entries:
            return False, "No data to export"
        
        try:
            with open(filename, 'w', newline='', encoding='utf-8') as csvfile:
                writer = csv.writer(csvfile)
                
                # Write header
                writer.writerow(['Timestamp', 'Data'])
                
                # Write data rows
                for entry in self.log_entries:
                    timestamp_str = entry['timestamp'].strftime('%Y-%m-%d %H:%M:%S.%f')[:-3]
                    writer.writerow([timestamp_str, entry['data']])
            
            return True, f"Successfully exported {len(self.log_entries)} entries to {filename}"
            
        except Exception as e:
            return False, f"Export failed: {str(e)}"
    
    def get_latest_entries(self, count=100):
        """
        Get the most recent entries
        Args:
            count: Number of entries to retrieve
        Returns:
            List of entry dictionaries
        """
        if count >= len(self.log_entries):
            return list(self.log_entries)
        else:
            return list(self.log_entries)[-count:]
    
    def get_all_entries(self):
        """Get all logged entries"""
        return list(self.log_entries)