#!/usr/bin/env python3
"""Test DYT Spectrum Owl thermal camera"""
import cv2
import numpy as np

print("=== DYT Thermal Camera Test ===\n")

# Open thermal camera (index 2)
cap = cv2.VideoCapture(2)

if not cap.isOpened():
    print("✗ Failed to open camera!")
    exit(1)

print("✓ Camera opened successfully!")

# Get camera properties
width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
fps = cap.get(cv2.CAP_PROP_FPS)

print(f"Resolution: {width}x{height}")
print(f"FPS: {fps}")

# Capture a few frames
print("\nCapturing frames...")
for i in range(5):
    ret, frame = cap.read()
    if ret:
        print(f"Frame {i+1}: Shape={frame.shape}, Type={frame.dtype}, Range=[{frame.min()}, {frame.max()}]")
    else:
        print(f"Frame {i+1}: Failed to read")

# Analyze thermal encoding
ret, frame = cap.read()
if ret:
    print("\n=== Thermal Data Analysis ===")

    # Check if it's grayscale or color
    if len(frame.shape) == 2:
        print("Format: Grayscale (pure thermal)")
        thermal_data = frame
    else:
        print(f"Format: Color (BGR) - {frame.shape[2]} channels")

        # Check which channel has thermal data
        b, g, r = cv2.split(frame)

        print(f"Blue channel: range=[{b.min()}, {b.max()}], std={b.std():.2f}")
        print(f"Green channel: range=[{g.min()}, {g.max()}], std={g.std():.2f}")
        print(f"Red channel: range=[{r.min()}, {r.max()}], std={r.std():.2f}")

        # Convert to grayscale
        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        print(f"Grayscale: range=[{gray.min()}, {gray.max()}], std={gray.std():.2f}")

        thermal_data = gray

    # Apply colormap
    thermal_colored = cv2.applyColorMap(thermal_data, cv2.COLORMAP_JET)

    # Save test frames
    cv2.imwrite('dyt_raw_frame.png', frame)
    cv2.imwrite('dyt_thermal_colored.png', thermal_colored)
    print("\n✓ Saved test frames:")
    print("  - dyt_raw_frame.png (raw from camera)")
    print("  - dyt_thermal_colored.png (with colormap)")

    # Show live feed for 10 seconds
    print("\n=== Live Feed Test ===")
    print("Showing thermal feed for 10 seconds...")
    print("Press 'q' to quit early\n")

    start_time = cv2.getTickCount()
    frame_count = 0

    while True:
        ret, frame = cap.read()
        if not ret:
            break

        frame_count += 1

        # Convert to grayscale
        if len(frame.shape) > 2:
            gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        else:
            gray = frame

        # Apply colormap
        thermal_colored = cv2.applyColorMap(gray, cv2.COLORMAP_JET)

        # Add info overlay
        cv2.putText(thermal_colored, f"DYT Thermal Camera", (10, 30),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)
        cv2.putText(thermal_colored, f"Resolution: {width}x{height}", (10, 60),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 1)
        cv2.putText(thermal_colored, f"Frame: {frame_count}", (10, 90),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 1)

        # Display
        cv2.imshow('DYT Thermal Camera Test', thermal_colored)
        cv2.imshow('Raw Feed', frame)

        # Check for quit or timeout
        elapsed = (cv2.getTickCount() - start_time) / cv2.getTickFrequency()
        if elapsed > 10 or cv2.waitKey(1) & 0xFF == ord('q'):
            break

    # Calculate actual FPS
    actual_fps = frame_count / elapsed
    print(f"✓ Captured {frame_count} frames in {elapsed:.2f} seconds")
    print(f"  Actual FPS: {actual_fps:.2f}")

cap.release()
cv2.destroyAllWindows()

print("\n=== Test Complete ===")
print("✓ DYT thermal camera is working!")
print("\nNext step: Run full analyzer")
print("  python3 thermal_gold_analyzer.py --camera 2")
