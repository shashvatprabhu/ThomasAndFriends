#!/usr/bin/env python3
"""
Gold Purity Analyzer using Thermal Analysis
Analyzes cooling curves to estimate gold purity based on thermal conductivity
"""

import numpy as np
import matplotlib.pyplot as plt
from scipy.optimize import curve_fit
from scipy.signal import savgol_filter
import time


class CoolingCurveAnalyzer:
    """
    Analyzes thermal cooling curves to estimate material properties

    Pure gold has high thermal conductivity (~310 W/m·K)
    Gold alloys have lower conductivity:
    - 24K (99.9% pure): ~310 W/m·K
    - 22K (91.7% pure): ~250-280 W/m·K
    - 18K (75% pure): ~180-220 W/m·K
    - 14K (58.3% pure): ~120-160 W/m·K
    """

    def __init__(self):
        self.cooling_data = []
        self.time_data = []
        self.start_time = None

        # Gold thermal properties (approximate)
        self.gold_standards = {
            '24K': {'purity': 99.9, 'cooling_rate': 1.0, 'thermal_conductivity': 310},
            '22K': {'purity': 91.7, 'cooling_rate': 0.85, 'thermal_conductivity': 265},
            '18K': {'purity': 75.0, 'cooling_rate': 0.65, 'thermal_conductivity': 200},
            '14K': {'purity': 58.3, 'cooling_rate': 0.50, 'thermal_conductivity': 140},
            'Fake': {'purity': 0.0, 'cooling_rate': 0.30, 'thermal_conductivity': 80}
        }

    def start_recording(self):
        """Start recording cooling curve"""
        self.cooling_data = []
        self.time_data = []
        self.start_time = time.time()
        print("✓ Started recording cooling curve")

    def add_data_point(self, temperature):
        """
        Add a temperature measurement
        Args:
            temperature: Current temperature reading
        """
        if self.start_time is None:
            self.start_recording()

        current_time = time.time() - self.start_time
        self.time_data.append(current_time)
        self.cooling_data.append(temperature)

    def exponential_cooling(self, t, T_ambient, T_initial, k):
        """
        Exponential cooling model: T(t) = T_ambient + (T_initial - T_ambient) * exp(-k*t)
        Args:
            t: time
            T_ambient: ambient temperature
            T_initial: initial temperature
            k: cooling rate constant
        """
        return T_ambient + (T_initial - T_ambient) * np.exp(-k * t)

    def fit_cooling_curve(self):
        """
        Fit exponential cooling curve to data
        Returns:
            Fitted parameters (T_ambient, T_initial, k) and quality metrics
        """
        if len(self.time_data) < 10:
            return None, None

        t = np.array(self.time_data)
        T = np.array(self.cooling_data)

        # Smooth data
        if len(T) > 5:
            T_smooth = savgol_filter(T, window_length=min(11, len(T)//2*2+1), polyorder=2)
        else:
            T_smooth = T

        # Initial parameter guesses
        T_ambient_guess = np.min(T_smooth)
        T_initial_guess = np.max(T_smooth)
        k_guess = 0.1

        try:
            # Fit the curve
            params, covariance = curve_fit(
                self.exponential_cooling,
                t, T_smooth,
                p0=[T_ambient_guess, T_initial_guess, k_guess],
                bounds=([0, 0, 0], [np.inf, np.inf, 10])
            )

            T_ambient, T_initial, k = params

            # Calculate R-squared
            residuals = T_smooth - self.exponential_cooling(t, *params)
            ss_res = np.sum(residuals**2)
            ss_tot = np.sum((T_smooth - np.mean(T_smooth))**2)
            r_squared = 1 - (ss_res / ss_tot) if ss_tot > 0 else 0

            fit_quality = {
                'r_squared': r_squared,
                'rmse': np.sqrt(np.mean(residuals**2)),
                'params': params,
                'covariance': covariance
            }

            return params, fit_quality

        except Exception as e:
            print(f"Error fitting curve: {e}")
            return None, None

    def calculate_cooling_rate(self):
        """
        Calculate average cooling rate (temperature change per second)
        """
        if len(self.cooling_data) < 2:
            return 0

        t = np.array(self.time_data)
        T = np.array(self.cooling_data)

        # Calculate derivative (cooling rate)
        dt = np.diff(t)
        dT = np.diff(T)
        cooling_rate = -np.mean(dT / dt)  # Negative because temperature decreases

        return cooling_rate

    def estimate_purity(self, cooling_rate_constant=None):
        """
        Estimate gold purity based on cooling rate
        Args:
            cooling_rate_constant: k value from exponential fit (optional)
        Returns:
            Dictionary with purity estimate and confidence
        """
        if cooling_rate_constant is None:
            params, quality = self.fit_cooling_curve()
            if params is None:
                return None
            cooling_rate_constant = params[2]  # k value

        # Normalize cooling rate (relative to 24K gold)
        normalized_rate = cooling_rate_constant / 1.0  # Assume 1.0 is reference for 24K

        # Find closest match
        best_match = None
        min_difference = float('inf')

        for grade, properties in self.gold_standards.items():
            difference = abs(properties['cooling_rate'] - normalized_rate)
            if difference < min_difference:
                min_difference = difference
                best_match = grade

        # Calculate confidence (inverse of difference)
        confidence = max(0, 100 - min_difference * 100)

        result = {
            'grade': best_match,
            'purity': self.gold_standards[best_match]['purity'],
            'confidence': confidence,
            'cooling_rate_constant': cooling_rate_constant,
            'normalized_rate': normalized_rate
        }

        return result

    def calculate_time_constant(self):
        """
        Calculate thermal time constant (tau = 1/k)
        Faster cooling = smaller time constant = higher conductivity
        """
        params, quality = self.fit_cooling_curve()
        if params is None:
            return None

        k = params[2]
        tau = 1 / k if k > 0 else float('inf')

        return tau

    def plot_cooling_curve(self, show_fit=True, save_path=None):
        """
        Plot the cooling curve with fitted model
        """
        if len(self.time_data) == 0:
            print("No data to plot")
            return

        t = np.array(self.time_data)
        T = np.array(self.cooling_data)

        plt.figure(figsize=(12, 6))

        # Plot 1: Temperature vs Time
        plt.subplot(1, 2, 1)
        plt.plot(t, T, 'b.-', label='Measured', linewidth=2, markersize=8)

        if show_fit and len(t) >= 10:
            params, quality = self.fit_cooling_curve()
            if params is not None:
                t_fit = np.linspace(0, max(t), 100)
                T_fit = self.exponential_cooling(t_fit, *params)
                plt.plot(t_fit, T_fit, 'r--', label=f'Fitted (R²={quality["r_squared"]:.3f})', linewidth=2)

                # Add equation
                T_amb, T_init, k = params
                equation = f'T(t) = {T_amb:.1f} + {T_init-T_amb:.1f}·exp(-{k:.3f}t)'
                plt.text(0.05, 0.95, equation, transform=plt.gca().transAxes,
                        fontsize=10, verticalalignment='top', bbox=dict(boxstyle='round', facecolor='wheat'))

        plt.xlabel('Time (seconds)', fontsize=12)
        plt.ylabel('Temperature (relative)', fontsize=12)
        plt.title('Cooling Curve', fontsize=14, fontweight='bold')
        plt.legend()
        plt.grid(True, alpha=0.3)

        # Plot 2: Cooling Rate vs Time
        plt.subplot(1, 2, 2)
        if len(t) > 1:
            dt = np.diff(t)
            dT = np.diff(T)
            cooling_rate = -dT / dt
            t_rate = t[:-1]
            plt.plot(t_rate, cooling_rate, 'g.-', linewidth=2, markersize=8)

        plt.xlabel('Time (seconds)', fontsize=12)
        plt.ylabel('Cooling Rate (°/s)', fontsize=12)
        plt.title('Cooling Rate over Time', fontsize=14, fontweight='bold')
        plt.grid(True, alpha=0.3)

        plt.tight_layout()

        if save_path:
            plt.savefig(save_path, dpi=150, bbox_inches='tight')
            print(f"✓ Saved plot to {save_path}")

        plt.show()

    def generate_report(self):
        """
        Generate comprehensive analysis report
        """
        print("\n" + "="*60)
        print("GOLD PURITY ANALYSIS REPORT")
        print("="*60)

        if len(self.time_data) < 10:
            print("⚠ Insufficient data for analysis")
            print(f"Collected {len(self.time_data)} points (minimum 10 required)")
            return

        # Fit cooling curve
        params, quality = self.fit_cooling_curve()
        if params is None:
            print("✗ Failed to fit cooling curve")
            return

        T_ambient, T_initial, k = params

        # Estimate purity
        purity_result = self.estimate_purity(k)

        # Print results
        print(f"\n📊 Measurement Summary:")
        print(f"  Duration: {self.time_data[-1]:.2f} seconds")
        print(f"  Data points: {len(self.time_data)}")
        print(f"  Temperature range: {np.min(self.cooling_data):.2f} - {np.max(self.cooling_data):.2f}")

        print(f"\n🔬 Cooling Curve Parameters:")
        print(f"  Initial temperature: {T_initial:.2f}")
        print(f"  Ambient temperature: {T_ambient:.2f}")
        print(f"  Cooling rate constant (k): {k:.4f} s⁻¹")
        print(f"  Time constant (τ): {1/k:.2f} seconds")

        print(f"\n📈 Fit Quality:")
        print(f"  R-squared: {quality['r_squared']:.4f}")
        print(f"  RMSE: {quality['rmse']:.4f}")

        print(f"\n💎 Gold Purity Estimate:")
        print(f"  Grade: {purity_result['grade']}")
        print(f"  Purity: {purity_result['purity']:.1f}%")
        print(f"  Confidence: {purity_result['confidence']:.1f}%")

        print(f"\n📚 Reference Values:")
        for grade, props in self.gold_standards.items():
            marker = "→" if grade == purity_result['grade'] else " "
            print(f"  {marker} {grade}: {props['purity']:.1f}% purity, "
                  f"~{props['thermal_conductivity']} W/m·K conductivity")

        print("\n" + "="*60)


def main():
    """Test cooling curve analyzer with synthetic data"""
    print("=== Cooling Curve Analyzer Test ===\n")

    # Create analyzer
    analyzer = CoolingCurveAnalyzer()

    # Generate synthetic cooling data (simulating 22K gold)
    print("Generating synthetic cooling data (22K gold)...")
    t = np.linspace(0, 30, 50)  # 30 seconds, 50 points
    T_ambient = 25
    T_initial = 80
    k_true = 0.85  # 22K gold cooling rate

    # Generate data with some noise
    noise = np.random.normal(0, 1, len(t))
    T = T_ambient + (T_initial - T_ambient) * np.exp(-k_true * t) + noise

    # Add data points
    analyzer.start_recording()
    for time_val, temp_val in zip(t, T):
        analyzer.time_data.append(time_val)
        analyzer.cooling_data.append(temp_val)

    # Analyze
    print("Analyzing cooling curve...\n")
    analyzer.generate_report()

    # Plot
    print("\nGenerating plot...")
    analyzer.plot_cooling_curve(show_fit=True)


if __name__ == "__main__":
    main()
