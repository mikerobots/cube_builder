#!/usr/bin/env python3
"""
Simple script to debug ray coordinates
"""

def debug_ray_coordinates():
    # Default window size from test
    window_width = 1280
    window_height = 720
    
    # Center position from test
    center_x = window_width // 2  # 640
    center_y = window_height // 2 # 360
    
    print(f"Window size: {window_width}x{window_height}")
    print(f"Center position: ({center_x}, {center_y})")
    
    # Convert to NDC (same as MouseInteraction::getMouseRay)
    ndc_x = (2.0 * center_x) / window_width - 1.0
    ndc_y = 1.0 - (2.0 * center_y) / window_height
    
    print(f"NDC coordinates: ({ndc_x}, {ndc_y})")
    print(f"Expected NDC for center: (0, 0)")
    
    # Ray length from FeedbackRenderer is 10.0f
    ray_length = 10.0
    print(f"Ray length: {ray_length}")
    
    # Check various positions from the test
    test_positions = [
        (center_x - 50, center_y),      # Slightly left of center
        (center_x, center_y),           # Center
        (center_x, center_y - 50),      # Slightly above center
        (center_x, center_y + 50),      # Slightly below center
    ]
    
    print("\nTest positions and their NDC coordinates:")
    for i, (x, y) in enumerate(test_positions):
        ndc_x = (2.0 * x) / window_width - 1.0
        ndc_y = 1.0 - (2.0 * y) / window_height
        print(f"Position {i}: ({x}, {y}) -> NDC({ndc_x:.3f}, {ndc_y:.3f})")

if __name__ == "__main__":
    debug_ray_coordinates()