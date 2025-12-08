#!/usr/bin/env python3
"""Quick test to check camera access"""
import cv2

print("Testing camera access...")
for i in range(4):
    print(f"\nTrying /dev/video{i}...")
    try:
        cap = cv2.VideoCapture(i)
        if cap.isOpened():
            ret, frame = cap.read()
            if ret:
                print(f"  ✓ SUCCESS! Camera {i} works!")
                print(f"    Resolution: {frame.shape[1]}x{frame.shape[0]}")
                print(f"    Channels: {frame.shape[2] if len(frame.shape) > 2 else 1}")
            else:
                print(f"  ✗ Opened but can't read frames")
            cap.release()
        else:
            print(f"  ✗ Can't open")
    except Exception as e:
        print(f"  ✗ Error: {e}")
