#!/usr/bin/env python3
"""
DYT Camera Initialization Script
Tries different methods to activate the thermal sensor
"""
import cv2
import numpy as np
import time

print("=== DYT Camera Initialization ===\n")

camera_index = 2

print("Method 1: Standard initialization with various settings...")
cap = cv2.VideoCapture(camera_index)

if not cap.isOpened():
    print("✗ Failed to open camera!")
    exit(1)

# Try different backend
print("Trying different backends and settings...")

# Try setting various properties
settings = [
    (cv2.CAP_PROP_CONVERT_RGB, 0),  # Don't convert to RGB
    (cv2.CAP_PROP_CONVERT_RGB, 1),  # Convert to RGB
    (cv2.CAP_PROP_FORMAT, -1),      # Default format
    (cv2.CAP_PROP_MODE, 0),          # Mode 0
    (cv2.CAP_PROP_MODE, 1),          # Mode 1
]

for prop, value in settings:
    try:
        cap.set(prop, value)
        print(f"  Set property {prop} = {value}")
    except:
        pass

# Wait a bit for camera to initialize
print("\nWaiting 2 seconds for camera to initialize...")
time.sleep(2)

# Try reading multiple frames (sometimes first frames are black)
print("\nReading first 20 frames (first frames often black)...")
for i in range(20):
    ret, frame = cap.read()
    if ret:
        non_zero = np.count_nonzero(frame)
        total = frame.size
        percent = (non_zero / total) * 100
        print(f"Frame {i+1:2d}: {percent:5.1f}% non-zero pixels", end='')

        if non_zero > 0:
            print(f" - min={frame.min()}, max={frame.max()} ✓")
            if frame.max() > 10:  # Found real data!
                print(f"\n✓ SUCCESS! Frame {i+1} has thermal data!")
                cv2.imwrite('dyt_working_frame.png', frame)
                print("Saved: dyt_working_frame.png")

                # Show live feed
                print("\nShowing live feed (press 'q' to quit)...")
                while True:
                    ret, frame = cap.read()
                    if not ret:
                        break

                    if len(frame.shape) > 2:
                        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
                    else:
                        gray = frame

                    thermal_colored = cv2.applyColorMap(gray, cv2.COLORMAP_JET)

                    cv2.imshow('DYT Thermal - Working!', thermal_colored)
                    cv2.imshow('Raw', frame)

                    if cv2.waitKey(1) & 0xFF == ord('q'):
                        break

                cap.release()
                cv2.destroyAllWindows()
                exit(0)
        else:
            print()
    time.sleep(0.1)

print("\n✗ Still getting black frames after 20 attempts")
print("\n=== Troubleshooting ===")
print("The camera is connected but not sending thermal data.")
print("\nPossible solutions:")
print("1. Try turning the camera OFF and ON again (power button)")
print("2. Unplug and replug the USB cable")
print("3. The camera might need Windows software to configure it first")
print("4. Some thermal cameras need special UVC commands")
print("\nTry this:")
print("  1. Turn camera OFF")
print("  2. Unplug USB")
print("  3. Wait 5 seconds")
print("  4. Plug USB back in")
print("  5. Turn camera ON")
print("  6. Run this script again")

cap.release()
cv2.destroyAllWindows()
