#!/usr/bin/env python3
"""
Thermal Camera Detection Module
Detects and lists UVC-compatible thermal cameras
"""

import cv2
import subprocess
import re


class CameraDetector:
    """Detects and manages UVC camera devices"""

    def __init__(self):
        self.cameras = []

    def get_usb_devices(self):
        """Get list of USB devices using lsusb"""
        try:
            result = subprocess.run(['lsusb'], capture_output=True, text=True)
            return result.stdout
        except Exception as e:
            print(f"Error running lsusb: {e}")
            return ""

    def get_video_devices(self):
        """Get list of video devices"""
        try:
            result = subprocess.run(['ls', '/dev/video*'],
                                  capture_output=True,
                                  text=True,
                                  shell=False)
            devices = result.stdout.strip().split('\n')
            return [d for d in devices if d]
        except Exception as e:
            print(f"Error listing video devices: {e}")
            return []

    def detect_cameras(self):
        """Detect all available cameras"""
        self.cameras = []
        video_devices = self.get_video_devices()

        print("=== Camera Detection ===")
        print(f"Found {len(video_devices)} video device(s)")

        for device_path in video_devices:
            try:
                # Extract device number
                device_num = int(re.search(r'(\d+)$', device_path).group(1))

                # Try to open the camera
                cap = cv2.VideoCapture(device_num)
                if cap.isOpened():
                    # Get camera properties
                    width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
                    height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
                    fps = int(cap.get(cv2.CAP_PROP_FPS))

                    camera_info = {
                        'device': device_path,
                        'index': device_num,
                        'width': width,
                        'height': height,
                        'fps': fps
                    }
                    self.cameras.append(camera_info)

                    print(f"\n✓ {device_path}")
                    print(f"  Resolution: {width}x{height}")
                    print(f"  FPS: {fps}")

                    cap.release()
                else:
                    print(f"\n✗ {device_path} - Could not open")

            except Exception as e:
                print(f"\n✗ {device_path} - Error: {e}")

        return self.cameras

    def get_thermal_camera(self):
        """
        Get the thermal camera device.
        For now, returns the last camera (thermal cameras are usually added later)
        In production, you'd identify by VID:PID
        """
        if not self.cameras:
            self.detect_cameras()

        if len(self.cameras) == 0:
            return None

        # Return the last camera (thermal camera is likely the last device)
        return self.cameras[-1]

    def print_usb_info(self):
        """Print USB device information"""
        print("\n=== USB Devices ===")
        usb_output = self.get_usb_devices()
        print(usb_output)


def main():
    """Test camera detection"""
    detector = CameraDetector()

    # Print USB devices
    detector.print_usb_info()

    # Detect cameras
    cameras = detector.detect_cameras()

    # Get thermal camera
    thermal_cam = detector.get_thermal_camera()
    if thermal_cam:
        print(f"\n=== Selected Thermal Camera ===")
        print(f"Device: {thermal_cam['device']}")
        print(f"Resolution: {thermal_cam['width']}x{thermal_cam['height']}")
    else:
        print("\n⚠ No thermal camera detected!")
        print("Please connect your thermal camera and run again.")


if __name__ == "__main__":
    main()
