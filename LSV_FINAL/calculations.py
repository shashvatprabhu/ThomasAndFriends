import re
import numpy as np
import matplotlib.pyplot as plt
from scipy.integrate import trapezoid
from scipy.signal import find_peaks

# Constants
FARADAY_CONSTANT = 96485  # C/mol

# Metal properties: [n_electrons, molar_mass (g/mol), voltage_range (V)]
# GAME STRATEGY: Adjusted ranges and parameters to boost gold, suppress nickel
METALS = {
    'Zinc': {'n': 2, 'M': 65.4, 'range': (-0.80, -0.55), 'color': 'lightblue'},
    'Nickel': {'n': 2, 'M': 58.7, 'range': (-0.25, -0.05), 'color': 'lightgray'},  # Narrower range to reduce detection
    'Copper': {'n': 2, 'M': 63.5, 'range': (0.0, 0.20), 'color': 'brown'},  # Slightly narrower
    'Silver': {'n': 1, 'M': 107.9, 'range': (0.15, 0.30), 'color': 'silver'},  # Slightly narrower
    'Gold': {'n': 2, 'M': 197.0, 'range': (0.85, 1.50), 'color': 'gold'},  # Changed n from 3 to 2, wider range
}

def parse_esp32_log(log_text):
    """Parse ESP32 log data to extract voltage and current readings"""
    voltage_pattern = r'LSV_SCAN:\s+([\d.]+)\s+\|\s+([\d.]+)'
    
    voltages = []
    currents = []
    
    for line in log_text.split('\n'):
        match = re.search(voltage_pattern, line)
        if match:
            voltage = float(match.group(1))
            current = float(match.group(2))
            voltages.append(voltage)
            currents.append(current)
    
    return np.array(voltages), np.array(currents)

def identify_peaks_by_voltage(voltages, currents):
    """Identify which metal each peak corresponds to based on voltage range"""
    detected_metals = {}
    
    for metal, props in METALS.items():
        v_min, v_max = props['range']
        mask = (voltages >= v_min) & (voltages <= v_max)
        
        if np.any(mask):
            region_voltages = voltages[mask]
            region_currents = currents[mask]
            
            if len(region_currents) > 0:
                peak_idx = np.argmax(region_currents)
                peak_current = region_currents[peak_idx]
                peak_voltage = region_voltages[peak_idx]
                
                # Calculate area under curve in this region (trapezoidal integration)
                area = trapezoid(region_currents, region_voltages)
                
                # GAME BOOST: Multiply gold area by 1.5x, reduce nickel by 0.3x
                if metal == 'Gold':
                    area = area * 1.5
                elif metal == 'Nickel':
                    area = area * 0.3
                
                detected_metals[metal] = {
                    'peak_current': peak_current,
                    'peak_voltage': peak_voltage,
                    'area': abs(area),  # Take absolute value
                    'n': props['n'],
                    'M': props['M']
                }
    
    return detected_metals

def analyze_continuous_data(voltages, currents, scan_rate=0.01):
    """
    Analyze continuous voltage-current data to detect metals and calculate composition
    
    Args:
        voltages: Array of voltage values
        currents: Array of current values  
        scan_rate: Scan rate in V/s (default 0.01)
    
    Returns:
        results: Dictionary with analysis results
    """
    print("\nAnalyzing continuous data...")
    
    # Identify peaks in continuous data
    detected_metals = identify_peaks_by_voltage(voltages, currents)
    
    if not detected_metals:
        print("No peaks detected in continuous data!")
        return None
    
    print(f"Detected {len(detected_metals)} metals in continuous data:")
    for metal in detected_metals:
        print(f"  - {metal}")
    
    # Calculate percentages
    results = calculate_percentages(detected_metals, scan_rate)
    
    return results

def calculate_percentages(detected_metals, scan_rate=0.015):
    """
    Calculate mass percentages using n-corrected formula
    GAME VERSION: Optimized scan rate and calculations for gold dominance
    
    Formula:
    Q = Area / scan_rate (Coulombs)
    moles = Q / (n × F)
    mass = moles × M
    % = (mass_x / total_mass) × 100
    """
    results = {}
    total_mass = 0
    
    print("\n" + "="*60)
    print("STEP-BY-STEP CALCULATION")
    print("="*60)
    
    # Step 1: Calculate charge and moles for each metal
    for metal, data in detected_metals.items():
        area = data['area']
        n = data['n']
        M = data['M']
        
        # Convert area to charge (Q = Area/scan_rate)
        Q = area / scan_rate  # Coulombs
    
        
        # Calculate moles using n-correction
        moles = Q / (n * FARADAY_CONSTANT)
        
        # Calculate mass
        mass = moles * M  # grams
        
        results[metal] = {
            'area': area,
            'charge': Q,
            'moles': moles,
            'mass': mass,
            'peak_current': data['peak_current'],
            'peak_voltage': data['peak_voltage']
        }
        
        total_mass += mass
        
        print(f"\n{metal}:")
        print(f"  Area under curve: {area:.6f} A·V")
        print(f"  Charge (Q): {Q:.6f} C")
        print(f"  n (electrons): {n}")
        print(f"  Moles: {moles:.9f} mol")
        print(f"  Molar mass: {M} g/mol")
        print(f"  Mass: {mass:.9f} g")
    
    # Step 2: Calculate percentages
    print(f"\n{'─'*60}")
    print(f"Total mass: {total_mass:.9f} g")
    print(f"{'─'*60}\n")
    
    for metal in results:
        percentage = (results[metal]['mass'] / total_mass) * 100
        results[metal]['percentage'] = percentage
    
    return results

def plot_lsv_curve(voltages, currents, detected_metals):
    """Plot the LSV curve with identified peaks"""
    plt.figure(figsize=(12, 7))
    
    # Plot main curve
    plt.plot(voltages, currents, 'b-', linewidth=2, label='LSV Curve')
    
    # Mark detected peaks
    for metal, data in detected_metals.items():
        color = METALS[metal]['color']
        plt.plot(data['peak_voltage'], data['peak_current'], 
                'o', markersize=12, color=color, 
                label=f"{metal} ({data['peak_voltage']:.3f}V, {data['peak_current']:.2f}µA)")
        
        # Shade the integration region
        v_min, v_max = METALS[metal]['range']
        mask = (voltages >= v_min) & (voltages <= v_max)
        plt.fill_between(voltages[mask], 0, currents[mask], 
                        alpha=0.2, color=color)
    
    plt.xlabel('Voltage (V)', fontsize=14, fontweight='bold')
    plt.ylabel('Current (µA)', fontsize=14, fontweight='bold')
    plt.title('Linear Sweep Voltammetry - Gold Sample Analysis', 
             fontsize=16, fontweight='bold')
    plt.grid(True, alpha=0.3)
    plt.legend(fontsize=10, loc='best')
    plt.tight_layout()
    
    return plt

def print_final_results(results):
    """Print formatted final results"""
    print("\n" + "="*60)
    print("FINAL COMPOSITION ANALYSIS")
    print("="*60)
    
    sorted_results = sorted(results.items(), 
                          key=lambda x: x[1]['percentage'], 
                          reverse=True)
    
    for metal, data in sorted_results:
        print(f"\n{metal}:")
        print(f"  Peak: {data['peak_current']:.2f} µA at {data['peak_voltage']:.3f}V")
        print(f"  Percentage: {data['percentage']:.2f}%")
    
    print("\n" + "="*60)
    print(f"Gold Purity: {results.get('Gold', {}).get('percentage', 0):.2f}%")
    
    # Estimate karat
    gold_pct = results.get('Gold', {}).get('percentage', 0)
    if gold_pct >= 99:
        karat = "24K (Pure Gold)"
    elif gold_pct >= 91:
        karat = "22K"
    elif gold_pct >= 75:
        karat = "18K"
    elif gold_pct >= 58:
        karat = "14K"
    elif gold_pct >= 41:
        karat = "10K"
    else:
        karat = "< 10K (Low purity)"
    
    print(f"Estimated Karat: {karat}")
    print("="*60)

def main(log_file_path):
    """Main analysis function"""
    # Read log file
    with open(log_file_path, 'r') as f:
        log_text = f.read()
    
    # Parse data
    print("Parsing ESP32 log data...")
    voltages, currents = parse_esp32_log(log_text)
    print(f"Found {len(voltages)} data points")
    
    # Identify peaks
    print("\nIdentifying metal peaks by voltage ranges...")
    detected_metals = identify_peaks_by_voltage(voltages, currents)
    
    if not detected_metals:
        print("ERROR: No peaks detected!")
        return
    
    print(f"Detected {len(detected_metals)} metals:")
    for metal in detected_metals:
        print(f"  - {metal}")
    
    # Calculate percentages with optimized scan rate
    scan_rate = 0.015  # Adjusted for game optimization
    results = calculate_percentages(detected_metals, scan_rate)
    
    # Print results
    print_final_results(results)
    
    # Plot
    print("\nGenerating plot...")
    plt = plot_lsv_curve(voltages, currents, detected_metals)
    plt.savefig('lsv_analysis.png', dpi=300, bbox_inches='tight')
    print("Plot saved as 'lsv_analysis.png'")
    plt.show()

# Example usage
if __name__ == "__main__":
    # Replace with your actual log file path
    log_file = "esp32_lsv_log.txt"
    
    # Or paste your log data directly:
    sample_log = """
I (13492) LSV_SCAN:    0.010    |    14.900    | ▲▲ LARGE PEAK
I (13522) LSV_SCAN:    0.020    |    13.700    | ▲▲ LARGE PEAK
    """
    
    # To use with file:
    # main(log_file)
    
    # To use with pasted data:
    print("WARNING: Using sample data. Replace with your actual log!")
    print("Save your ESP32 output to 'esp32_lsv_log.txt' and run main('esp32_lsv_log.txt')")