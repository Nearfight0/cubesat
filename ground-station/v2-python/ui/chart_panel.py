"""
Chart Panel
Real-time plotting of numeric data with dynamic auto-scaling
"""

from PyQt6.QtWidgets import QWidget, QVBoxLayout, QLabel, QHBoxLayout, QPushButton, QCheckBox
from collections import deque
import pyqtgraph as pg
from pyqtgraph import PlotWidget
import time


class ChartPanel(QWidget):
    """Widget for plotting real-time numeric data with enhanced scaling"""
    
    def __init__(self, parent=None):
        super().__init__(parent)
        self.max_points = 500
        self.data_series = {}
        self.plot_lines = {}
        self.colors = [
            (255, 0, 0),      # Red
            (0, 255, 0),      # Green
            (0, 0, 255),      # Blue
            (255, 255, 0),    # Yellow
            (255, 0, 255),    # Magenta
            (0, 255, 255),    # Cyan
        ]
        
        # Scaling options
        self.auto_scale = True
        self.scale_margin = 0.1  # 10% margin above/below data range
        self.normalize_data = False
        
        self.setup_ui()
    
    def setup_ui(self):
        """Initialize UI components"""
        layout = QVBoxLayout(self)
        
        # Header with controls
        header_layout = QHBoxLayout()
        
        title_label = QLabel("Real-time Data Chart")
        title_label.setStyleSheet("font-weight: bold; font-size: 12px;")
        header_layout.addWidget(title_label)
        
        self.data_info_label = QLabel("No data")
        self.data_info_label.setStyleSheet("color: #666;")
        header_layout.addWidget(self.data_info_label)
        
        header_layout.addStretch()
        
        # Auto-scale checkbox
        self.auto_scale_cb = QCheckBox("Auto-scale")
        self.auto_scale_cb.setChecked(True)
        self.auto_scale_cb.stateChanged.connect(self.toggle_auto_scale)
        self.auto_scale_cb.setToolTip("Dynamically adjust Y-axis to data range")
        header_layout.addWidget(self.auto_scale_cb)
        
        # Normalize checkbox
        self.normalize_cb = QCheckBox("Normalize")
        self.normalize_cb.setChecked(False)
        self.normalize_cb.stateChanged.connect(self.toggle_normalize)
        self.normalize_cb.setToolTip("Scale all series to 0-1 range")
        header_layout.addWidget(self.normalize_cb)
        
        # Reset zoom button
        reset_btn = QPushButton("Reset Zoom")
        reset_btn.setMaximumWidth(100)
        reset_btn.clicked.connect(self.reset_zoom)
        reset_btn.setToolTip("Reset to auto-scaled view")
        header_layout.addWidget(reset_btn)
        
        layout.addLayout(header_layout)
        
        # Create plot widget with enhanced features
        self.plot_widget = PlotWidget()
        self.plot_widget.setBackground('#1e1e1e')
        self.plot_widget.showGrid(x=True, y=True, alpha=0.3)
        self.plot_widget.setLabel('left', 'Value')
        self.plot_widget.setLabel('bottom', 'Time (s)')
        self.plot_widget.addLegend()
        
        # Enable mouse interaction
        self.plot_widget.setMouseEnabled(x=True, y=True)  # Allow zoom/pan
        
        # Get the ViewBox for manual control
        self.view_box = self.plot_widget.getViewBox()
        self.view_box.enableAutoRange(axis='y', enable=True)  # Enable Y auto-range
        self.view_box.enableAutoRange(axis='x', enable=True)  # Enable X auto-range
        
        layout.addWidget(self.plot_widget)
    
    def add_data_point(self, numeric_values):
        """
        Add new data points to chart with enhanced scaling
        Args:
            numeric_values: List of (name, value) tuples
        """
        if not numeric_values:
            return
        
        current_time = time.time()
        
        for i, (name, value) in enumerate(numeric_values):
            # Initialize series if new
            if name not in self.data_series:
                self.data_series[name] = {
                    'times': deque(maxlen=self.max_points),
                    'values': deque(maxlen=self.max_points),
                    'raw_values': deque(maxlen=self.max_points),  # Store original values
                    'start_time': current_time,
                    'min_value': value,
                    'max_value': value
                }
                
                # Create plot line
                color = self.colors[len(self.plot_lines) % len(self.colors)]
                self.plot_lines[name] = self.plot_widget.plot(
                    pen=pg.mkPen(color=color, width=2),
                    name=name
                )
            
            # Add data point
            series = self.data_series[name]
            relative_time = current_time - series['start_time']
            
            # Store raw value
            series['raw_values'].append(value)
            series['times'].append(relative_time)
            
            # Update min/max tracking
            series['min_value'] = min(series['min_value'], value)
            series['max_value'] = max(series['max_value'], value)
            
            # Process value based on normalization setting
            if self.normalize_data:
                processed_value = self._normalize_value(value, series)
            else:
                processed_value = value
            
            series['values'].append(processed_value)
            
            # Update plot
            self.plot_lines[name].setData(
                list(series['times']),
                list(series['values'])
            )
        
        # Apply auto-scaling if enabled
        if self.auto_scale:
            self._apply_auto_scale()
        
        # Update info label
        self._update_info_label()
    
    def _normalize_value(self, value, series):
        """
        Normalize value to 0-1 range based on series min/max
        Args:
            value: Current value
            series: Data series dictionary
        Returns:
            Normalized value (0-1)
        """
        min_val = series['min_value']
        max_val = series['max_value']
        
        # Avoid division by zero
        if max_val - min_val < 1e-10:
            return 0.5
        
        return (value - min_val) / (max_val - min_val)
    
    def _apply_auto_scale(self):
        """Apply dynamic auto-scaling to Y-axis based on visible data"""
        if not self.data_series:
            return
        
        # Find overall min/max across all series
        all_values = []
        for series in self.data_series.values():
            if series['values']:
                all_values.extend(series['values'])
        
        if not all_values:
            return
        
        data_min = min(all_values)
        data_max = max(all_values)
        
        # Add margin for better visibility
        data_range = data_max - data_min
        
        # Avoid division by zero for flat data
        if data_range < 1e-10:
            data_range = abs(data_max) * 0.1 if data_max != 0 else 1.0
        
        margin = data_range * self.scale_margin
        
        y_min = data_min - margin
        y_max = data_max + margin
        
        # Apply the range to Y-axis
        self.view_box.setYRange(y_min, y_max, padding=0)
        
        # Update Y-axis label with range info
        if self.normalize_data:
            self.plot_widget.setLabel('left', 'Normalized Value (0-1)')
        else:
            range_str = f"Range: {data_min:.2f} to {data_max:.2f}"
            self.plot_widget.setLabel('left', 'Value', units='', **{'color': '#888'})
    
    def _update_info_label(self):
        """Update information label with current stats"""
        series_count = len(self.data_series)
        point_count = len(list(self.data_series.values())[0]['times']) if self.data_series else 0
        
        # Calculate statistics
        if self.data_series:
            stats_parts = []
            for name, series in self.data_series.items():
                if series['raw_values']:
                    current_val = series['raw_values'][-1]
                    min_val = series['min_value']
                    max_val = series['max_value']
                    stats_parts.append(f"{name}: {current_val:.2f} (Δ={max_val-min_val:.2f})")
            
            stats_text = " | ".join(stats_parts[:2])  # Show first 2 series
            self.data_info_label.setText(f"{series_count} series, {point_count} pts | {stats_text}")
        else:
            self.data_info_label.setText("No data")
    
    def toggle_auto_scale(self, state):
        """Toggle auto-scaling feature"""
        self.auto_scale = bool(state)
        
        if self.auto_scale:
            self._apply_auto_scale()
        else:
            # Allow manual zoom/pan
            self.view_box.enableAutoRange(axis='y', enable=False)
    
    def toggle_normalize(self, state):
        """Toggle normalization feature"""
        self.normalize_data = bool(state)
        
        # Reprocess all data with new normalization setting
        for name, series in self.data_series.items():
            series['values'].clear()
            
            for raw_value in series['raw_values']:
                if self.normalize_data:
                    processed_value = self._normalize_value(raw_value, series)
                else:
                    processed_value = raw_value
                series['values'].append(processed_value)
            
            # Update plot
            self.plot_lines[name].setData(
                list(series['times']),
                list(series['values'])
            )
        
        # Update scaling
        if self.auto_scale:
            self._apply_auto_scale()
    
    def reset_zoom(self):
        """Reset zoom to show all data with auto-scaling"""
        self.auto_scale_cb.setChecked(True)
        self.view_box.enableAutoRange(axis='x', enable=True)
        self.view_box.enableAutoRange(axis='y', enable=True)
        
        if self.data_series:
            self._apply_auto_scale()
    
    def clear_chart(self):
        """Clear all chart data"""
        self.data_series.clear()
        self.plot_widget.clear()
        self.plot_lines.clear()
        self.plot_widget.addLegend()
        self.data_info_label.setText("No data")
        
        # Reset to default view
        self.auto_scale_cb.setChecked(True)
        self.normalize_cb.setChecked(False)
    
    def set_max_points(self, max_points):
        """Set maximum number of points to display"""
        self.max_points = max_points
    
    def set_scale_margin(self, margin):
        """
        Set margin for auto-scaling (0.0 to 1.0)
        Args:
            margin: Fraction of data range to add as margin (e.g., 0.1 = 10%)
        """
        self.scale_margin = max(0.0, min(1.0, margin))
        if self.auto_scale:
            self._apply_auto_scale()