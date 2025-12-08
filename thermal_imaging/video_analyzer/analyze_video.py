#!/usr/bin/env python3
"""
Thermal Cooling Video Analyzer
Analyzes cooling curves from thermal camera videos to estimate gold purity

Usage:
    python3 analyze_video.py video.mp4
    python3 analyze_video.py video.mp4 --plot
    python3 analyze_video.py video.mp4 --roi 100 100 80 80
"""

import cv2
import numpy as np
import matplotlib.pyplot as plt
from scipy.optimize import curve_fit
from scipy.signal import savgol_filter
import argparse
import sys
import os


class CoolingVideoAnalyzer:
    """Analyzes thermal cooling from video"""

    def __init__(self, video_path):
        self.video_path = video_path
        self.times = []
        self.temperatures = []
        self.normalized_temps = []
        self.roi = None
        self.fps = 0
        self.frame_count = 0
        self.peak_idx = None
        self.peak_time = None
        self.peak_temp = None
        self.cooling_start_idx = None

        # Gold purity standards (cooling rate constants)
        # Physics-based k values calculated from material properties and natural convection
        # Assumes standard coin geometry (20mm diameter, 2mm thick, A/V ≈ 1200 m⁻¹)
        # Formula: k = h × (A/V) / (ρ × cₚ), where h scales with thermal conductivity
        self.gold_standards = {
            '24K': {'purity': 99.9, 'k': 0.048, 'thermal_conductivity': 318},
            '22K': {'purity': 91.7, 'k': 0.037, 'thermal_conductivity': 265},
            '18K': {'purity': 75.0, 'k': 0.021, 'thermal_conductivity': 200},
            '14K': {'purity': 58.3, 'k': 0.014, 'thermal_conductivity': 140},
            'Fake': {'purity': 0.0, 'k': 0.009, 'thermal_conductivity': 80}
        }

    def load_video(self):
        """Load video and extract basic properties"""
        print(f"\n{'='*60}")
        print("THERMAL COOLING ANALYSIS")
        print(f"{'='*60}\n")

        if not os.path.exists(self.video_path):
            print(f"✗ Error: Video file not found: {self.video_path}")
            return False

        cap = cv2.VideoCapture(self.video_path)

        if not cap.isOpened():
            print(f"✗ Error: Cannot open video: {self.video_path}")
            return False

        self.fps = cap.get(cv2.CAP_PROP_FPS)
        self.frame_count = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))
        duration = self.frame_count / self.fps if self.fps > 0 else 0

        print(f"Video: {os.path.basename(self.video_path)}")
        print(f"Duration: {duration:.1f} seconds")
        print(f"Frames: {self.frame_count} ({self.fps:.1f} FPS)")

        cap.release()
        return True

    def auto_detect_roi(self, frame):
        """Auto-detect hottest region in frame"""
        # Convert to grayscale if needed
        if len(frame.shape) == 3:
            gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        else:
            gray = frame

        # Find hottest region (threshold at 85th percentile)
        threshold = np.percentile(gray, 85)
        hot_mask = (gray > threshold).astype(np.uint8) * 255

        # Find contours
        contours, _ = cv2.findContours(hot_mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

        if len(contours) == 0:
            # No hot spot found, use center region
            h, w = gray.shape
            margin = min(h, w) // 4
            return (w//2 - margin, h//2 - margin, margin*2, margin*2)

        # Get largest contour
        largest = max(contours, key=cv2.contourArea)
        x, y, w, h = cv2.boundingRect(largest)

        # Add margin
        margin = 10
        x = max(0, x - margin)
        y = max(0, y - margin)
        w = w + 2 * margin
        h = h + 2 * margin

        return (x, y, w, h)

    def extract_temperature(self, frame, roi):
        """Extract average temperature from ROI"""
        x, y, w, h = roi

        # Convert to grayscale if needed
        if len(frame.shape) == 3:
            gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        else:
            gray = frame

        # Extract ROI
        roi_region = gray[y:y+h, x:x+w]

        # Return average pixel intensity (relative temperature)
        return np.mean(roi_region)

    def process_video(self, manual_roi=None):
        """Process all frames and extract cooling curve"""
        cap = cv2.VideoCapture(self.video_path)

        frame_num = 0
        first_frame = True

        print("\nProcessing video...")

        while True:
            ret, frame = cap.read()
            if not ret:
                break

            # Auto-detect ROI from first frame
            if first_frame:
                if manual_roi:
                    self.roi = manual_roi
                    print(f"Using manual ROI: {self.roi}")
                else:
                    self.roi = self.auto_detect_roi(frame)
                    print(f"Auto-detected ROI: {self.roi}")
                first_frame = False

            # Extract temperature
            temp = self.extract_temperature(frame, self.roi)
            time = frame_num / self.fps

            self.times.append(time)
            self.temperatures.append(temp)

            frame_num += 1

            # Progress indicator
            if frame_num % 50 == 0:
                print(f"  Processed {frame_num}/{self.frame_count} frames...")

        cap.release()
        print(f"✓ Processed all {frame_num} frames\n")

        return True

    def detect_peak(self):
        """Find the actual peak temperature in the video"""
        temps = np.array(self.temperatures)

        # Find peak
        self.peak_idx = np.argmax(temps)
        self.peak_time = self.times[self.peak_idx]
        self.peak_temp = temps[self.peak_idx]

        # Cooling starts at peak
        self.cooling_start_idx = self.peak_idx

        # Warnings
        if self.peak_time < 2.0:
            print(f"✓ Peak at video start (t={self.peak_time:.1f}s)")
            print("  (Sample was already placed when recording started)\n")
        else:
            print(f"✓ Peak detected at t={self.peak_time:.1f}s (frame {self.peak_idx})")
            print(f"  (Video started {self.peak_time:.1f}s before sample reached peak)\n")

        # Check cooling duration
        cooling_duration = (len(temps) - self.peak_idx) / self.fps
        if cooling_duration < 15:
            print(f"⚠ Warning: Only {cooling_duration:.1f}s of cooling recorded")
            print("  Recommend 20-30 seconds for accurate analysis\n")

        # Warn if peak at end
        if self.peak_idx > len(temps) * 0.9:
            print("⚠ Warning: Peak at video end - sample may still be heating!\n")

        return self.peak_idx, self.peak_time, self.peak_temp

    def normalize_temperatures(self):
        """Normalize temperatures (room temp independent!)"""
        temps = np.array(self.temperatures)

        # Detect peak automatically
        peak_idx, peak_time, T_peak = self.detect_peak()

        # Ambient temperature (average of last 20% of frames - fully cooled)
        last_20_percent = int(len(temps) * 0.2)
        T_ambient = np.mean(temps[-last_20_percent:])

        # Normalize: θ(t) = (T(t) - T_ambient) / (T_peak - T_ambient)
        # Result: θ = 1.0 at peak (hot), θ = 0.0 when cooled
        self.normalized_temps = (temps - T_ambient) / (T_peak - T_ambient)

        # Cooling phase info
        cooling_frames = len(temps) - peak_idx
        cooling_duration = cooling_frames / self.fps

        print(f"Temperature Range:")
        print(f"  Peak (t={peak_time:.1f}s): {T_peak:.1f} (relative)")
        print(f"  Final: {T_ambient:.1f} (relative)")
        print(f"  Delta: {T_peak - T_ambient:.1f}\n")

        print(f"Cooling Phase:")
        print(f"  Duration: {cooling_duration:.1f} seconds")
        print(f"  Frames analyzed: {cooling_frames}\n")

        return T_peak, T_ambient

    def fit_cooling_curve(self):
        """Fit exponential cooling curve: θ(t) = e^(-kt)"""
        # Use only cooling phase (from peak onwards)
        t_all = np.array(self.times)
        theta_all = np.array(self.normalized_temps)

        # Extract cooling phase only
        t = t_all[self.cooling_start_idx:]
        theta = theta_all[self.cooling_start_idx:]

        # Reset time to 0 at peak
        t = t - t[0]

        # Remove any NaN or inf values
        valid = np.isfinite(theta) & (theta > 0)
        t = t[valid]
        theta = theta[valid]

        if len(t) < 10:
            print("✗ Error: Not enough valid data points for fitting")
            print(f"  Only {len(t)} points in cooling phase")
            return None, None

        # Fit exponential decay
        def exp_decay(t, k):
            return np.exp(-k * t)

        try:
            # Initial guess: k ≈ 0.03 (typical for gold cooling in air)
            popt, pcov = curve_fit(exp_decay, t, theta, p0=[0.03], bounds=([0], [1.0]))
            k = popt[0]

            # Calculate R-squared
            theta_fit = exp_decay(t, k)
            residuals = theta - theta_fit
            ss_res = np.sum(residuals**2)
            ss_tot = np.sum((theta - np.mean(theta))**2)
            r_squared = 1 - (ss_res / ss_tot) if ss_tot > 0 else 0

            # Calculate RMSE
            rmse = np.sqrt(np.mean(residuals**2))

            print(f"Normalized Cooling Curve Fit:")
            print(f"  θ(t) = e^(-{k:.3f}t)")
            print(f"  R² = {r_squared:.4f}")
            print(f"  RMSE = {rmse:.4f}\n")

            return k, r_squared

        except Exception as e:
            print(f"✗ Error fitting curve: {e}")
            return None, None

    def estimate_purity(self, k):
        """Estimate gold purity from cooling rate constant"""
        if k is None:
            return None

        # Find closest match to gold standards
        best_match = None
        min_diff = float('inf')

        for grade, properties in self.gold_standards.items():
            diff = abs(properties['k'] - k)
            if diff < min_diff:
                min_diff = diff
                best_match = grade

        # Calculate confidence (inverse of difference, scaled 0-100%)
        # Use 0.05 as max difference (full range of k values is ~0.048)
        confidence = max(0, min(100, 100 * (1 - min_diff / 0.05)))

        result = {
            'grade': best_match,
            'purity': self.gold_standards[best_match]['purity'],
            'confidence': confidence,
            'k': k
        }

        print(f"Cooling Rate Constant:")
        print(f"  k = {k:.3f} s⁻¹\n")

        print(f"Gold Purity Estimate:")
        print(f"  Grade: {result['grade']}")
        print(f"  Purity: {result['purity']:.1f}%")
        print(f"  Confidence: {result['confidence']:.0f}%\n")

        return result

    def plot_results(self, k, r_squared, purity_result, save_path=None):
        """Generate analysis plots"""
        fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5))

        t = np.array(self.times)
        theta = np.array(self.normalized_temps)
        temps = np.array(self.temperatures)

        # Plot 1: Raw temperature vs time
        ax1.plot(t, temps, 'b.-', linewidth=2, markersize=4, label='Measured')
        ax1.set_xlabel('Time (seconds)', fontsize=12)
        ax1.set_ylabel('Temperature (relative)', fontsize=12)
        ax1.set_title('Raw Cooling Curve', fontsize=14, fontweight='bold')
        ax1.grid(True, alpha=0.3)
        ax1.legend()

        # Plot 2: Normalized curve with fit
        ax2.plot(t, theta, 'g.-', linewidth=2, markersize=4, label='Measured (normalized)')

        # Plot fitted curve
        if k is not None:
            t_fit = np.linspace(0, max(t), 200)
            theta_fit = np.exp(-k * t_fit)
            ax2.plot(t_fit, theta_fit, 'r--', linewidth=2,
                    label=f'Fitted (R²={r_squared:.3f})')

            # Add equation
            equation = f'θ(t) = e^(-{k:.3f}t)'
            ax2.text(0.05, 0.95, equation, transform=ax2.transAxes,
                    fontsize=11, verticalalignment='top',
                    bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.8))

            # Add purity estimate
            if purity_result:
                purity_text = f"{purity_result['grade']}: {purity_result['purity']:.1f}%\n"
                purity_text += f"Confidence: {purity_result['confidence']:.0f}%"
                ax2.text(0.95, 0.05, purity_text, transform=ax2.transAxes,
                        fontsize=10, verticalalignment='bottom', horizontalalignment='right',
                        bbox=dict(boxstyle='round', facecolor='lightblue', alpha=0.8))

        ax2.set_xlabel('Time (seconds)', fontsize=12)
        ax2.set_ylabel('Normalized Temperature θ', fontsize=12)
        ax2.set_title('Normalized Cooling Curve (Room Temp Independent)',
                     fontsize=14, fontweight='bold')
        ax2.grid(True, alpha=0.3)
        ax2.legend()
        ax2.set_ylim([-0.1, 1.1])

        plt.tight_layout()

        if save_path:
            plt.savefig(save_path, dpi=150, bbox_inches='tight')
            print(f"✓ Plot saved: {save_path}")

        plt.show()

    def analyze(self, manual_roi=None, plot=False):
        """Complete analysis pipeline"""
        # Load video
        if not self.load_video():
            return False

        # Process all frames
        if not self.process_video(manual_roi):
            return False

        # Normalize temperatures
        self.normalize_temperatures()

        # Fit cooling curve
        k, r_squared = self.fit_cooling_curve()

        if k is None:
            return False

        # Estimate purity
        purity_result = self.estimate_purity(k)

        # Print reference values
        print(f"Reference Values (Cooling Rate):")
        for grade, props in self.gold_standards.items():
            marker = "→" if grade == purity_result['grade'] else " "
            print(f"  {marker} {grade}: k = {props['k']:.2f}, "
                  f"{props['purity']:.1f}% purity, "
                  f"~{props['thermal_conductivity']} W/m·K")

        print(f"\n{'='*60}\n")

        # Plot if requested
        if plot:
            save_path = os.path.splitext(self.video_path)[0] + '_analysis.png'
            self.plot_results(k, r_squared, purity_result, save_path)

        return True


def main():
    parser = argparse.ArgumentParser(
        description='Analyze thermal cooling video to estimate gold purity',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python3 analyze_video.py cooling.mp4
  python3 analyze_video.py cooling.mp4 --plot
  python3 analyze_video.py cooling.mp4 --roi 100 100 80 80
        """)

    parser.add_argument('video', help='Path to thermal video file (MP4, AVI, MOV)')
    parser.add_argument('--plot', action='store_true', help='Display and save analysis plots')
    parser.add_argument('--roi', nargs=4, type=int, metavar=('X', 'Y', 'W', 'H'),
                       help='Manual ROI: x y width height (auto-detect if not specified)')

    args = parser.parse_args()

    # Create analyzer
    analyzer = CoolingVideoAnalyzer(args.video)

    # Convert ROI if provided
    manual_roi = tuple(args.roi) if args.roi else None

    # Run analysis
    success = analyzer.analyze(manual_roi=manual_roi, plot=args.plot)

    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()
