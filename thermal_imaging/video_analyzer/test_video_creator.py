#!/usr/bin/env python3
"""
Create synthetic thermal cooling video for testing
Simulates 22K gold cooling from hot to room temperature
"""

import cv2
import numpy as np
import os

print("="*60)
print("CREATING TEST COOLING VIDEO")
print("="*60)

# Video parameters
output_file = "demo_cooling.mp4"
fps = 20
duration = 30  # seconds
frame_count = fps * duration

# Image dimensions
width, height = 320, 240

# Cooling simulation (22K gold)
T_ambient = 25.0  # Room temperature
T_initial = 85.0  # Initial hot temperature
k = 0.85  # Cooling rate constant (22K gold)

print(f"\nSimulation Parameters:")
print(f"  Gold Grade: 22K (91.7% purity)")
print(f"  Cooling Rate: k = {k} s⁻¹")
print(f"  Temperature: {T_initial:.1f}°C → {T_ambient:.1f}°C")
print(f"  Duration: {duration} seconds")
print(f"  FPS: {fps}")
print(f"  Total frames: {frame_count}\n")

# Create video writer
fourcc = cv2.VideoWriter_fourcc(*'mp4v')
out = cv2.VideoWriter(output_file, fourcc, fps, (width, height))

print("Generating frames...")

for frame_num in range(frame_count):
    # Calculate time
    t = frame_num / fps

    # Exponential cooling: T(t) = T_ambient + (T_initial - T_ambient) * e^(-kt)
    temp = T_ambient + (T_initial - T_ambient) * np.exp(-k * t)

    # Create thermal image with hot spot in center
    y, x = np.ogrid[:height, :width]
    center_x, center_y = width // 2, height // 2

    # Gaussian hot spot
    distance = np.sqrt((x - center_x)**2 + (y - center_y)**2)
    hotspot = np.exp(-(distance**2) / (40**2))

    # Normalize temperature to pixel intensity (0-255)
    temp_normalized = (temp - T_ambient) / (T_initial - T_ambient)
    thermal_gray = (hotspot * temp_normalized * 255).astype(np.uint8)

    # Add realistic noise
    noise = np.random.normal(0, 2, thermal_gray.shape).astype(np.int16)
    thermal_gray = np.clip(thermal_gray.astype(np.int16) + noise, 0, 255).astype(np.uint8)

    # Apply thermal colormap (JET)
    thermal_colored = cv2.applyColorMap(thermal_gray, cv2.COLORMAP_JET)

    # Add timestamp overlay
    cv2.putText(thermal_colored, f"Time: {t:.1f}s", (10, 25),
               cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 255, 255), 2)
    cv2.putText(thermal_colored, f"Temp: {temp:.1f}C", (10, 50),
               cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 255, 255), 2)
    cv2.putText(thermal_colored, "22K Gold (simulated)", (10, 230),
               cv2.FONT_HERSHEY_SIMPLEX, 0.4, (255, 255, 255), 1)

    # Write frame
    out.write(thermal_colored)

    # Progress
    if (frame_num + 1) % 100 == 0:
        print(f"  Generated {frame_num + 1}/{frame_count} frames...")

# Release video writer
out.release()

print(f"\n✓ Successfully created test video: {output_file}")
print(f"✓ File size: {os.path.getsize(output_file) / 1024:.1f} KB")

print(f"\n{'='*60}")
print("TEST VIDEO READY!")
print(f"{'='*60}\n")

print("Now analyze it:")
print(f"  python3 analyze_video.py {output_file} --plot")
print()
