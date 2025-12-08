#!/usr/bin/env python3
"""
Thermal Gold Purity Analyzer - Main Application
Integrated application for gold purity testing using thermal imaging
"""

import cv2
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.backends.backend_agg import FigureCanvasAgg
import time
from datetime import datetime
import argparse

from camera_detector import CameraDetector
from thermal_capture import ThermalCamera
from thermal_processing import ThermalProcessor
from gold_purity_analyzer import CoolingCurveAnalyzer


class ThermalGoldAnalyzer:
    """
    Main application for thermal gold purity analysis
    Combines camera detection, thermal imaging, and cooling curve analysis
    """

    def __init__(self, camera_index=None):
        """Initialize the analyzer"""
        print("="*60)
        print("THERMAL GOLD PURITY ANALYZER")
        print("="*60)

        # Initialize components
        self.detector = CameraDetector()
        self.camera = None
        self.processor = ThermalProcessor()
        self.analyzer = CoolingCurveAnalyzer()

        # State
        self.recording = False
        self.roi = None  # Region of interest for temperature measurement
        self.baseline_temp = None

        # Settings
        self.colormap = 'jet'

        # Detect camera
        if camera_index is None:
            cameras = self.detector.detect_cameras()
            if cameras:
                camera_index = cameras[-1]['index']  # Use last camera
                print(f"\n✓ Using camera at index {camera_index}")
            else:
                print("\n⚠ No camera detected!")
                camera_index = 0

        self.camera = ThermalCamera(camera_index)

    def set_roi(self, x, y, width, height):
        """Set region of interest for temperature measurement"""
        self.roi = (x, y, width, height)
        print(f"✓ ROI set to: ({x}, {y}, {width}x{height})")

    def auto_detect_roi(self, frame):
        """Automatically detect the hottest region as ROI"""
        temp_map = self.camera.get_temperature_map(frame)

        # Find hot spot
        hot_mask, contours = self.processor.detect_hot_spots(temp_map, threshold_percentile=85)

        if len(contours) > 0:
            # Get largest contour
            largest_contour = max(contours, key=cv2.contourArea)
            x, y, w, h = cv2.boundingRect(largest_contour)

            # Expand ROI slightly
            margin = 10
            x = max(0, x - margin)
            y = max(0, y - margin)
            w = min(frame.shape[1] - x, w + 2*margin)
            h = min(frame.shape[0] - y, h + 2*margin)

            self.roi = (x, y, w, h)
            return True
        return False

    def get_roi_temperature(self, frame):
        """Get average temperature in ROI"""
        if self.roi is None:
            return None

        x, y, w, h = self.roi
        return self.camera.get_roi_temperature(frame, x, y, w, h)

    def start_analysis(self):
        """Start recording cooling curve"""
        self.recording = True
        self.analyzer.start_recording()
        self.baseline_temp = None
        print("\n🔴 Recording cooling curve...")
        print("Instructions:")
        print("  1. Apply heat pulse to gold sample")
        print("  2. Keep camera steady on the sample")
        print("  3. Press 's' to stop recording")

    def stop_analysis(self):
        """Stop recording and analyze"""
        self.recording = False
        print("\n⏹ Stopped recording")
        print("\nAnalyzing data...")
        self.analyzer.generate_report()
        return True

    def draw_overlay(self, frame, thermal_colored):
        """Draw overlay with information"""
        overlay = thermal_colored.copy()

        # Draw ROI
        if self.roi is not None:
            x, y, w, h = self.roi
            cv2.rectangle(overlay, (x, y), (x+w, y+h), (0, 255, 0), 2)
            cv2.putText(overlay, "ROI", (x, y-10),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 2)

            # Get and display ROI temperature
            roi_temp = self.get_roi_temperature(frame)
            if roi_temp is not None:
                cv2.putText(overlay, f"Temp: {roi_temp:.1f}",
                           (x, y+h+20),
                           cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 2)

                # Record temperature if recording
                if self.recording:
                    self.analyzer.add_data_point(roi_temp)

        # Recording indicator
        if self.recording:
            cv2.circle(overlay, (30, 30), 10, (0, 0, 255), -1)
            cv2.putText(overlay, "RECORDING", (50, 35),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 0, 255), 2)

            # Show data points collected
            n_points = len(self.analyzer.time_data)
            cv2.putText(overlay, f"Points: {n_points}", (50, 60),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 1)

        # Instructions
        instructions = [
            "Keys:",
            "r - Auto-detect ROI",
            "a - Start analysis",
            "s - Stop analysis",
            "c - Change colormap",
            "p - Plot results",
            "q - Quit"
        ]

        y_offset = frame.shape[0] - 20 - len(instructions) * 20
        for i, instruction in enumerate(instructions):
            cv2.putText(overlay, instruction,
                       (10, y_offset + i*20),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.4, (255, 255, 255), 1)

        return overlay

    def change_colormap(self):
        """Cycle through colormaps"""
        colormaps = ['jet', 'hot', 'cool', 'rainbow', 'magma', 'inferno', 'turbo']
        current_idx = colormaps.index(self.colormap)
        self.colormap = colormaps[(current_idx + 1) % len(colormaps)]
        print(f"✓ Changed colormap to: {self.colormap}")

    def run(self):
        """Run the main application loop"""
        if not self.camera.open():
            print("✗ Failed to open camera!")
            return

        print("\n✓ Application started")
        print("\nWaiting for thermal camera feed...")

        try:
            while True:
                # Read frame
                ret, frame = self.camera.read_frame()
                if not ret:
                    print("✗ Failed to read frame")
                    break

                # Process thermal image
                thermal_colored, gray_frame = self.camera.convert_to_thermal(frame)

                # Apply selected colormap
                thermal_colored = self.processor.apply_colormap(gray_frame, self.colormap)

                # Draw overlay
                display_frame = self.draw_overlay(frame, thermal_colored)

                # Display
                cv2.imshow('Thermal Gold Analyzer', display_frame)

                # Handle key presses
                key = cv2.waitKey(1) & 0xFF

                if key == ord('q'):
                    break

                elif key == ord('r'):
                    # Auto-detect ROI
                    if self.auto_detect_roi(frame):
                        print("✓ ROI auto-detected")
                    else:
                        print("⚠ Could not detect hot spot")

                elif key == ord('a'):
                    # Start analysis
                    if self.roi is None:
                        print("⚠ Please set ROI first (press 'r')")
                    else:
                        self.start_analysis()

                elif key == ord('s'):
                    # Stop analysis
                    if self.recording:
                        self.stop_analysis()

                elif key == ord('c'):
                    # Change colormap
                    self.change_colormap()

                elif key == ord('p'):
                    # Plot results
                    if len(self.analyzer.time_data) > 0:
                        self.analyzer.plot_cooling_curve()
                    else:
                        print("⚠ No data to plot")

                elif key == ord('w'):
                    # Save frame
                    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
                    filename = f"thermal_capture_{timestamp}.png"
                    self.camera.save_frame(display_frame, filename)

        except KeyboardInterrupt:
            print("\n\n✓ Interrupted by user")

        finally:
            self.camera.close()
            cv2.destroyAllWindows()
            print("\n✓ Application closed")


def main():
    """Main entry point"""
    parser = argparse.ArgumentParser(description='Thermal Gold Purity Analyzer')
    parser.add_argument('--camera', type=int, default=None,
                       help='Camera index (default: auto-detect)')
    parser.add_argument('--demo', action='store_true',
                       help='Run demo mode with synthetic data')

    args = parser.parse_args()

    if args.demo:
        print("Running demo mode with synthetic data...")
        from gold_purity_analyzer import CoolingCurveAnalyzer
        analyzer = CoolingCurveAnalyzer()

        # Generate synthetic data
        t = np.linspace(0, 30, 50)
        T_ambient = 25
        T_initial = 80
        k_true = 0.85  # 22K gold

        noise = np.random.normal(0, 1, len(t))
        T = T_ambient + (T_initial - T_ambient) * np.exp(-k_true * t) + noise

        analyzer.start_recording()
        for time_val, temp_val in zip(t, T):
            analyzer.time_data.append(time_val)
            analyzer.cooling_data.append(temp_val)

        analyzer.generate_report()
        analyzer.plot_cooling_curve()

    else:
        # Run full application
        app = ThermalGoldAnalyzer(camera_index=args.camera)
        app.run()


if __name__ == "__main__":
    main()
