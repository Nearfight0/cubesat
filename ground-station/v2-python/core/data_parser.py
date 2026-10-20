"""
Data Parser
Parses incoming serial data, especially JSON packets
"""

import json
import re


class DataParser:
    """Parse and extract information from serial data"""
    
    @staticmethod
    def parse_data(raw_data):
        """
        Parse raw data string
        Returns: dict with parsed information
        """
        result = {
            'raw': raw_data,
            'is_json': False,
            'json_data': None,
            'numeric_values': []
        }
        
        # Try to parse as JSON
        try:
            json_data = json.loads(raw_data)
            result['is_json'] = True
            result['json_data'] = json_data
            
            # Extract numeric values from JSON
            result['numeric_values'] = DataParser._extract_numbers_from_dict(json_data)
            
        except (json.JSONDecodeError, ValueError):
            # Not valid JSON, try to extract numbers from raw string
            result['numeric_values'] = DataParser._extract_numbers_from_string(raw_data)
        
        return result
    
    @staticmethod
    def _extract_numbers_from_dict(data, prefix=''):
        """Recursively extract numeric values from dictionary"""
        numbers = []
        
        if isinstance(data, dict):
            for key, value in data.items():
                full_key = f"{prefix}.{key}" if prefix else key
                if isinstance(value, (int, float)):
                    numbers.append((full_key, float(value)))
                elif isinstance(value, dict):
                    numbers.extend(DataParser._extract_numbers_from_dict(value, full_key))
                elif isinstance(value, list):
                    for i, item in enumerate(value):
                        if isinstance(item, (int, float)):
                            numbers.append((f"{full_key}[{i}]", float(item)))
        
        return numbers
    
    @staticmethod
    def _extract_numbers_from_string(text):
        """Extract numeric values from plain text"""
        numbers = []
        
        # Find all numbers (integers and floats)
        pattern = r'-?\d+\.?\d*'
        matches = re.findall(pattern, text)
        
        for i, match in enumerate(matches):
            try:
                value = float(match)
                numbers.append((f"value_{i}", value))
            except ValueError:
                continue
        
        return numbers
    
    @staticmethod
    def format_for_display(parsed_data):
        """Format parsed data for display in log"""
        raw = parsed_data['raw']
        
        if parsed_data['is_json']:
            try:
                # Pretty print JSON
                formatted = json.dumps(
                    parsed_data['json_data'], 
                    indent=2, 
                    ensure_ascii=False
                )
                return formatted
            except:
                return raw
        
        return raw