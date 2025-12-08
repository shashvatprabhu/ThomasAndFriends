#!/usr/bin/env python3
"""
Create synthetic thermal image sequence for testing
Simulates a gold cooling curve after heat pulse
"""

import cv2
import numpy as np
import os
from datetime import datetime

print("=== Creating Demo Thermal Image Sequence ===\n")

# Create output directory
output_dir = "demo_thermal_frames"
os.makedirs(output_dir, exist_ok=True)

# Simulation parameters (22K gold)
T_ambient = 25.0  # Ambient temperature
T_initial = 85.0  # Initial temperature after heat pulse
k = 0.85  # Cooling rate constant (22K gold)

# Image dimensions (similar to DYT camera)
width, height = 256, 384

# Number of frames and duration
num_frames = 40
duration = 25.0  # seconds
time_step = duration / num_frames

print(f"Generating {num_frames} frames...")
print(f"Simulating 22K gold cooling")
print(f"Duration: {duration} seconds\n")

for i in range(num_frames):
    t = i * time_step

    # Calculate temperature at time t using exponential cooling
    # T(t) = T_ambient + (T_initial - T_ambient) * exp(-k*t)
    temp = T_ambient + (T_initial - T_ambient) * np.exp(-k * t)

    # Create thermal image
    # Center hot spot that cools over time
    y, x = np.ogrid[:height, :width]
    center_x, center_y = width // 2, height // 2

    # Create Gaussian hot spot
    distance = np.sqrt((x - center_x)**2 + (y - center_y)**2)
    hotspot = np.exp(-(distance**2) / (50**2))

    # Scale to temperature range (0-255)
    # Higher temp = brighter image
    temp_normalized = (temp - T_ambient) / (T_initial - T_ambient)
    thermal_image = (hotspot * temp_normalized * 255).astype(np.uint8)

    # Add some noise for realism
    noise = np.random.normal(0, 3, thermal_image.shape).astype(np.int16)
    thermal_image = np.clip(thermal_image.astype(np.int16) + noise, 0, 255).astype(np.uint8)

    # Convert to BGR for OpenCV
    thermal_bgr = cv2.cvtColor(thermal_image, cv2.COLOR_GRAY2BGR)

    # Apply colormap for visualization
    thermal_colored = cv2.applyColorMap(thermal_image, cv2.COLORMAP_JET)

    # Add info overlay
    cv2.putText(thermal_colored, f"Time: {t:.1f}s", (10, 30),
               cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 255, 255), 2)
    cv2.putText(thermal_colored, f"Temp: {temp:.1f}C", (10, 60),
               cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 255, 255), 2)
    cv2.putText(thermal_colored, f"22K Gold (simulated)", (10, 90),
               cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 1)

    # Save frame
    filename = os.path.join(output_dir, f"frame{i+1:03d}.png")
    cv2.imwrite(filename, thermal_colored)

    if (i + 1) % 10 == 0:
        print(f"  Generated {i+1}/{num_frames} frames...")

print(f"\n✓ Successfully created {num_frames} thermal frames")
print(f"✓ Saved to: {output_dir}/")
print(f"\n📊 Simulated gold properties:")
print(f"  Grade: 22K (91.7% purity)")
print(f"  Cooling rate constant: {k:.3f} s⁻¹")
print(f"  Temperature range: {T_ambient:.1f}°C - {T_initial:.1f}°C")

print("\n🚀 Now run the analyzer:")
print(f"  python3 thermal_image_analyzer.py {output_dir}/ --plot")
