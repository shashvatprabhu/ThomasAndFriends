#!/usr/bin/env python3
"""
Analyze demo frames with proper timing
"""

import cv2
import numpy as np
import glob
import os

from thermal_processing import ThermalProcessor
from gold_purity_analyzer import CoolingCurveAnalyzer

print("="*60)
print("DEMO: THERMAL GOLD PURITY ANALYSIS")
print("="*60)

# Load demo frames
image_folder = "demo_thermal_frames"
image_files = sorted(glob.glob(os.path.join(image_folder, "*.png")))

print(f"\n✓ Found {len(image_files)} demo frames")

# Initialize
processor = ThermalProcessor()
analyzer = CoolingCurveAnalyzer()

# Process each frame with PROPER TIMING
# Demo simulates 25 seconds, 40 frames = 0.625 seconds per frame
duration = 25.0
time_interval = duration / len(image_files)

print(f"✓ Simulated duration: {duration} seconds")
print(f"✓ Frame interval: {time_interval:.3f} seconds")

# Auto-detect ROI from first image
first_img = cv2.imread(image_files[0])
if len(first_img.shape) == 3:
    gray = cv2.cvtColor(first_img, cv2.COLOR_BGR2GRAY)
else:
    gray = first_img

hot_mask, contours = processor.detect_hot_spots(gray, threshold_percentile=85)

if len(contours) > 0:
    largest_contour = max(contours, key=cv2.contourArea)
    x, y, w, h = cv2.boundingRect(largest_contour)
    margin = 10
    x = max(0, x - margin)
    y = max(0, y - margin)
    w = min(first_img.shape[1] - x, w + 2*margin)
    h = min(first_img.shape[0] - y, h + 2*margin)
    roi = (x, y, w, h)
    print(f"✓ Auto-detected ROI: ({x}, {y}, {w}x{h})")
else:
    # Use whole image
    roi = (0, 0, first_img.shape[1], first_img.shape[0])
    print("⚠ Using whole image as ROI")

# Start recording
analyzer.start_recording()

print("\nProcessing frames...")
for i, img_path in enumerate(image_files):
    img = cv2.imread(img_path)

    # Extract ROI
    x, y, w, h = roi
    roi_img = img[y:y+h, x:x+w]

    # Convert to grayscale if needed
    if len(roi_img.shape) == 3:
        gray_roi = cv2.cvtColor(roi_img, cv2.COLOR_BGR2GRAY)
    else:
        gray_roi = roi_img

    # Get average temperature
    temp = np.mean(gray_roi)

    # Use PROPER time (not file timestamp!)
    time = i * time_interval

    analyzer.time_data.append(time)
    analyzer.cooling_data.append(temp)

    if (i + 1) % 10 == 0:
        print(f"  Processed {i+1}/{len(image_files)} frames...")

print(f"✓ Processed all {len(image_files)} frames\n")

# Generate report
analyzer.generate_report()

# Plot results
print("\nGenerating plot...")
analyzer.plot_cooling_curve(show_fit=True, save_path="demo_cooling_curve.png")

print("\n" + "="*60)
print("✅ DEMO COMPLETE!")
print("="*60)
print("\n📁 Generated files:")
print("  - demo_thermal_frames/    (40 thermal images)")
print("  - demo_cooling_curve.png  (analysis plot)")
print("\n📊 This demonstrates:")
print("  ✓ Image sequence loading")
print("  ✓ ROI detection")
print("  ✓ Temperature extraction")
print("  ✓ Cooling curve fitting")
print("  ✓ Gold purity estimation")
print("  ✓ Statistical validation")
print("\n🎯 Next step: Use with REAL thermal images!")
print("  1. Capture frames from DYT Windows app")
print("  2. Transfer to Linux")
print("  3. Run: python3 thermal_image_analyzer.py your_frames/")
