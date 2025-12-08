#!/usr/bin/env python3
"""
Thermal Image Capture Module
Captures thermal images from UVC thermal camera
"""

import cv2
import numpy as np
from datetime import datetime
import time


class ThermalCamera:
    """Manages thermal camera capture and frame processing"""

    def __init__(self, camera_index=0):
        """
        Initialize thermal camera
        Args:
            camera_index: Video device index (default: 0)
        """
        self.camera_index = camera_index
        self.cap = None
        self.is_open = False
        self.current_frame = None
        self.thermal_frame = None

    def open(self):
        """Open the camera device"""
        try:
            self.cap = cv2.VideoCapture(self.camera_index)

            if not self.cap.isOpened():
                raise Exception(f"Could not open camera at index {self.camera_index}")

            # Try to set higher resolution if available (common thermal camera resolutions)
            # Common thermal resolutions: 160x120, 320x240, 384x288, 640x480
            self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
            self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)

            # Read actual resolution
            self.width = int(self.cap.get(cv2.CAP_PROP_FRAME_WIDTH))
            self.height = int(self.cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
            self.fps = int(self.cap.get(cv2.CAP_PROP_FPS))

            self.is_open = True
            print(f"✓ Thermal camera opened successfully")
            print(f"  Resolution: {self.width}x{self.height}")
            print(f"  FPS: {self.fps}")

            return True

        except Exception as e:
            print(f"✗ Error opening camera: {e}")
            self.is_open = False
            return False

    def read_frame(self):
        """
        Read a frame from the camera
        Returns:
            tuple: (success, frame)
        """
        if not self.is_open:
            return False, None

        ret, frame = self.cap.read()
        if ret:
            self.current_frame = frame
        return ret, frame

    def convert_to_thermal(self, frame):
        """
        Convert raw frame to thermal representation
        Many thermal cameras output grayscale or YUV format
        This function converts to a thermal colormap
        """
        # Check if frame is already grayscale
        if len(frame.shape) == 2:
            gray = frame
        else:
            # Convert BGR to grayscale
            gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)

        # Apply colormap for better thermal visualization
        # COLORMAP_JET: Blue (cold) -> Red (hot)
        thermal_colored = cv2.applyColorMap(gray, cv2.COLORMAP_JET)

        self.thermal_frame = thermal_colored
        return thermal_colored, gray

    def get_temperature_map(self, frame):
        """
        Extract temperature map from frame
        Note: Actual temperature mapping depends on camera calibration
        This provides relative temperature values
        """
        if len(frame.shape) == 2:
            gray = frame
        else:
            gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)

        # Normalize to 0-100 scale (representing relative temperature)
        temp_map = gray.astype(float)
        temp_map = (temp_map - temp_map.min()) / (temp_map.max() - temp_map.min()) * 100

        return temp_map

    def get_roi_temperature(self, frame, x, y, width, height):
        """
        Get average temperature in a region of interest
        Args:
            frame: Input frame
            x, y: Top-left corner of ROI
            width, height: ROI dimensions
        Returns:
            Average temperature in ROI
        """
        temp_map = self.get_temperature_map(frame)
        roi = temp_map[y:y+height, x:x+width]
        return np.mean(roi)

    def get_max_temp_location(self, frame):
        """
        Find the location of maximum temperature in frame
        Returns:
            tuple: (x, y, max_temp)
        """
        temp_map = self.get_temperature_map(frame)
        max_temp = np.max(temp_map)
        max_loc = np.unravel_index(np.argmax(temp_map), temp_map.shape)

        return max_loc[1], max_loc[0], max_temp  # (x, y, temp)

    def save_frame(self, frame, filename=None):
        """Save frame to file"""
        if filename is None:
            timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
            filename = f"thermal_frame_{timestamp}.png"

        cv2.imwrite(filename, frame)
        print(f"✓ Saved frame to {filename}")
        return filename

    def close(self):
        """Close the camera"""
        if self.cap is not None:
            self.cap.release()
            self.is_open = False
            print("✓ Camera closed")


def main():
    """Test thermal camera capture"""
    print("=== Thermal Camera Capture Test ===\n")

    # Initialize camera (change index if needed)
    camera = ThermalCamera(camera_index=0)

    if not camera.open():
        print("Failed to open camera!")
        return

    print("\nPress 'q' to quit, 's' to save frame, 't' to show temperature info")

    try:
        while True:
            ret, frame = camera.read_frame()

            if not ret:
                print("Failed to read frame")
                break

            # Convert to thermal representation
            thermal_frame, gray_frame = camera.convert_to_thermal(frame)

            # Get max temperature location
            max_x, max_y, max_temp = camera.get_max_temp_location(frame)

            # Draw crosshair at hottest point
            cv2.drawMarker(thermal_frame, (max_x, max_y),
                         color=(255, 255, 255),
                         markerType=cv2.MARKER_CROSS,
                         markerSize=20,
                         thickness=2)

            # Add temperature text
            cv2.putText(thermal_frame,
                       f"Max Temp: {max_temp:.1f}",
                       (10, 30),
                       cv2.FONT_HERSHEY_SIMPLEX,
                       0.7,
                       (255, 255, 255),
                       2)

            # Display
            cv2.imshow('Thermal Camera', thermal_frame)
            cv2.imshow('Raw Frame', frame)

            # Handle key presses
            key = cv2.waitKey(1) & 0xFF
            if key == ord('q'):
                break
            elif key == ord('s'):
                camera.save_frame(thermal_frame)
            elif key == ord('t'):
                print(f"Max temperature: {max_temp:.1f} at ({max_x}, {max_y})")

    except KeyboardInterrupt:
        print("\nInterrupted by user")

    finally:
        camera.close()
        cv2.destroyAllWindows()


if __name__ == "__main__":
    main()
