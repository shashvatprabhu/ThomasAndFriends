#!/usr/bin/env python3
"""
Thermal Image Sequence Analyzer
Analyzes cooling curves from a sequence of saved thermal images
Works with images captured from DYT Windows software
"""

import cv2
import numpy as np
import os
import glob
from datetime import datetime
import argparse

from thermal_processing import ThermalProcessor
from gold_purity_analyzer import CoolingCurveAnalyzer


class ThermalImageSequenceAnalyzer:
    """Analyzes thermal images from a folder"""

    def __init__(self, image_folder=None):
        self.image_folder = image_folder
        self.processor = ThermalProcessor()
        self.analyzer = CoolingCurveAnalyzer()
        self.images = []
        self.roi = None

    def load_images(self, pattern="*.png"):
        """Load all images from folder"""
        if not self.image_folder:
            print("Error: No image folder specified")
            return False

        # Find all images
        search_pattern = os.path.join(self.image_folder, pattern)
        image_files = sorted(glob.glob(search_pattern))

        if len(image_files) == 0:
            print(f"✗ No images found matching: {search_pattern}")
            return False

        print(f"✓ Found {len(image_files)} images")

        # Load images
        self.images = []
        for img_path in image_files:
            img = cv2.imread(img_path)
            if img is not None:
                self.images.append({
                    'path': img_path,
                    'filename': os.path.basename(img_path),
                    'image': img,
                    'timestamp': os.path.getmtime(img_path)
                })

        print(f"✓ Loaded {len(self.images)} images successfully")

        # Sort by timestamp
        self.images.sort(key=lambda x: x['timestamp'])

        # Calculate time intervals
        if len(self.images) > 1:
            start_time = self.images[0]['timestamp']
            for img_data in self.images:
                img_data['time'] = img_data['timestamp'] - start_time

            duration = self.images[-1]['time']
            print(f"✓ Sequence duration: {duration:.2f} seconds")
            print(f"  Average interval: {duration / len(self.images):.3f} seconds")

        return True

    def set_roi(self, x, y, width, height):
        """Set region of interest"""
        self.roi = (x, y, width, height)
        print(f"✓ ROI set to: ({x}, {y}, {width}x{height})")

    def auto_detect_roi(self, image_index=0):
        """Auto-detect ROI from first image"""
        if len(self.images) == 0:
            return False

        img = self.images[image_index]['image']

        # Convert to grayscale
        if len(img.shape) == 3:
            gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
        else:
            gray = img

        # Find hot spots
        hot_mask, contours = self.processor.detect_hot_spots(gray, threshold_percentile=85)

        if len(contours) > 0:
            # Get largest contour
            largest_contour = max(contours, key=cv2.contourArea)
            x, y, w, h = cv2.boundingRect(largest_contour)

            # Expand ROI slightly
            margin = 10
            x = max(0, x - margin)
            y = max(0, y - margin)
            w = min(img.shape[1] - x, w + 2*margin)
            h = min(img.shape[0] - y, h + 2*margin)

            self.roi = (x, y, w, h)
            print(f"✓ Auto-detected ROI: ({x}, {y}, {w}x{h})")
            return True

        return False

    def get_roi_temperature(self, img):
        """Get average temperature in ROI"""
        if self.roi is None:
            # Use whole image
            gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY) if len(img.shape) == 3 else img
            return np.mean(gray)

        x, y, w, h = self.roi

        # Extract ROI
        roi_img = img[y:y+h, x:x+w]

        # Convert to grayscale if needed
        if len(roi_img.shape) == 3:
            gray = cv2.cvtColor(roi_img, cv2.COLOR_BGR2GRAY)
        else:
            gray = roi_img

        return np.mean(gray)

    def analyze_sequence(self):
        """Analyze the entire sequence"""
        if len(self.images) == 0:
            print("✗ No images loaded")
            return False

        print("\n=== Analyzing Image Sequence ===")

        # Auto-detect ROI if not set
        if self.roi is None:
            print("Auto-detecting ROI from first image...")
            self.auto_detect_roi(0)

        # Start recording
        self.analyzer.start_recording()

        # Process each image
        print("Processing images...")
        for i, img_data in enumerate(self.images):
            img = img_data['image']
            time = img_data.get('time', i)  # Use timestamp or frame index

            # Get temperature
            temp = self.get_roi_temperature(img)

            # Add to analyzer
            self.analyzer.time_data.append(time)
            self.analyzer.cooling_data.append(temp)

            if (i + 1) % 10 == 0:
                print(f"  Processed {i + 1}/{len(self.images)} images...")

        print(f"✓ Processed all {len(self.images)} images")

        # Generate report
        print("\n" + "="*60)
        self.analyzer.generate_report()

        return True

    def visualize_sequence(self, save_video=None):
        """Create visualization of the sequence"""
        if len(self.images) == 0:
            print("✗ No images loaded")
            return

        print("\n=== Creating Visualization ===")

        # Prepare video writer if saving
        video_writer = None
        if save_video:
            height, width = self.images[0]['image'].shape[:2]
            fourcc = cv2.VideoWriter_fourcc(*'mp4v')
            video_writer = cv2.VideoWriter(save_video, fourcc, 10, (width, height))

        # Show each image
        for i, img_data in enumerate(self.images):
            img = img_data['image'].copy()
            time = img_data.get('time', i)

            # Draw ROI
            if self.roi is not None:
                x, y, w, h = self.roi
                cv2.rectangle(img, (x, y), (x+w, y+h), (0, 255, 0), 2)

            # Add info
            cv2.putText(img, f"Frame {i+1}/{len(self.images)}", (10, 30),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)
            cv2.putText(img, f"Time: {time:.2f}s", (10, 60),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)

            if len(self.analyzer.cooling_data) > i:
                temp = self.analyzer.cooling_data[i]
                cv2.putText(img, f"Temp: {temp:.1f}", (10, 90),
                           cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)

            # Show
            cv2.imshow('Thermal Sequence', img)

            # Save to video
            if video_writer:
                video_writer.write(img)

            # Wait
            key = cv2.waitKey(100) & 0xFF
            if key == ord('q'):
                break

        if video_writer:
            video_writer.release()
            print(f"✓ Saved video: {save_video}")

        cv2.destroyAllWindows()

    def plot_results(self):
        """Plot cooling curve results"""
        if len(self.analyzer.cooling_data) == 0:
            print("✗ No data to plot")
            return

        self.analyzer.plot_cooling_curve(show_fit=True)


def main():
    parser = argparse.ArgumentParser(description='Analyze thermal image sequences')
    parser.add_argument('folder', nargs='?', default='thermal_frames',
                       help='Folder containing thermal images')
    parser.add_argument('--pattern', default='*.png',
                       help='Image file pattern (default: *.png)')
    parser.add_argument('--roi', nargs=4, type=int, metavar=('X', 'Y', 'W', 'H'),
                       help='ROI coordinates (x y width height)')
    parser.add_argument('--plot', action='store_true',
                       help='Show plot after analysis')
    parser.add_argument('--video', type=str,
                       help='Save visualization as video')

    args = parser.parse_args()

    print("="*60)
    print("THERMAL IMAGE SEQUENCE ANALYZER")
    print("="*60)

    # Create analyzer
    analyzer = ThermalImageSequenceAnalyzer(args.folder)

    # Load images
    if not analyzer.load_images(args.pattern):
        print("\n⚠ Usage:")
        print("  1. Create a folder: mkdir thermal_frames")
        print("  2. Save thermal images from Windows app to that folder")
        print("  3. Run: python3 thermal_image_analyzer.py thermal_frames")
        return

    # Set ROI if specified
    if args.roi:
        analyzer.set_roi(*args.roi)

    # Analyze
    analyzer.analyze_sequence()

    # Visualize
    if args.video:
        analyzer.visualize_sequence(save_video=args.video)

    # Plot
    if args.plot:
        analyzer.plot_results()


if __name__ == "__main__":
    main()
