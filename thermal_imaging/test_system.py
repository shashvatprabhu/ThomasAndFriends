#!/usr/bin/env python3
"""
System Test Script
Tests all components of the Thermal Gold Analyzer
"""

import sys

def test_imports():
    """Test if all required packages are available"""
    print("="*60)
    print("TESTING IMPORTS")
    print("="*60)

    packages = [
        ('cv2', 'OpenCV'),
        ('numpy', 'NumPy'),
        ('matplotlib', 'Matplotlib'),
        ('scipy', 'SciPy')
    ]

    all_ok = True
    for package, name in packages:
        try:
            __import__(package)
            print(f"✓ {name} - OK")
        except ImportError as e:
            print(f"✗ {name} - MISSING: {e}")
            all_ok = False

    return all_ok

def test_modules():
    """Test if all custom modules can be imported"""
    print("\n" + "="*60)
    print("TESTING MODULES")
    print("="*60)

    modules = [
        'camera_detector',
        'thermal_capture',
        'thermal_processing',
        'gold_purity_analyzer',
        'thermal_gold_analyzer'
    ]

    all_ok = True
    for module in modules:
        try:
            __import__(module)
            print(f"✓ {module}.py - OK")
        except Exception as e:
            print(f"✗ {module}.py - ERROR: {e}")
            all_ok = False

    return all_ok

def test_camera_detection():
    """Test camera detection"""
    print("\n" + "="*60)
    print("TESTING CAMERA DETECTION")
    print("="*60)

    try:
        from camera_detector import CameraDetector
        detector = CameraDetector()
        cameras = detector.detect_cameras()

        if len(cameras) > 0:
            print(f"✓ Found {len(cameras)} camera(s)")
            return True
        else:
            print("⚠ No cameras detected (this is OK if thermal camera not connected)")
            return True
    except Exception as e:
        print(f"✗ Camera detection failed: {e}")
        return False

def test_cooling_curve_analyzer():
    """Test cooling curve analyzer with synthetic data"""
    print("\n" + "="*60)
    print("TESTING COOLING CURVE ANALYZER")
    print("="*60)

    try:
        import numpy as np
        from gold_purity_analyzer import CoolingCurveAnalyzer

        analyzer = CoolingCurveAnalyzer()

        # Generate synthetic data (22K gold)
        t = np.linspace(0, 20, 30)
        T_ambient = 25
        T_initial = 75
        k_true = 0.85

        noise = np.random.normal(0, 0.5, len(t))
        T = T_ambient + (T_initial - T_ambient) * np.exp(-k_true * t) + noise

        # Add data
        analyzer.start_recording()
        for time_val, temp_val in zip(t, T):
            analyzer.time_data.append(time_val)
            analyzer.cooling_data.append(temp_val)

        # Analyze
        params, quality = analyzer.fit_cooling_curve()

        if params is not None and quality['r_squared'] > 0.9:
            print(f"✓ Curve fitting successful (R² = {quality['r_squared']:.4f})")

            # Estimate purity
            result = analyzer.estimate_purity()
            print(f"✓ Purity estimation: {result['grade']} ({result['purity']:.1f}%)")
            return True
        else:
            print("✗ Curve fitting failed or poor quality")
            return False

    except Exception as e:
        print(f"✗ Analyzer test failed: {e}")
        import traceback
        traceback.print_exc()
        return False

def main():
    """Run all tests"""
    print("\n")
    print("╔" + "="*58 + "╗")
    print("║" + " "*15 + "THERMAL GOLD ANALYZER" + " "*22 + "║")
    print("║" + " "*19 + "SYSTEM TEST" + " "*28 + "║")
    print("╚" + "="*58 + "╝")
    print()

    results = []

    # Run tests
    results.append(("Package Imports", test_imports()))
    results.append(("Module Imports", test_modules()))
    results.append(("Camera Detection", test_camera_detection()))
    results.append(("Cooling Curve Analyzer", test_cooling_curve_analyzer()))

    # Summary
    print("\n" + "="*60)
    print("TEST SUMMARY")
    print("="*60)

    for test_name, result in results:
        status = "✓ PASS" if result else "✗ FAIL"
        print(f"{test_name:.<45} {status}")

    # Overall result
    all_passed = all(result for _, result in results)

    print("\n" + "="*60)
    if all_passed:
        print("✓ ALL TESTS PASSED")
        print("="*60)
        print("\nSystem is ready! You can now run:")
        print("  python3 thermal_gold_analyzer.py")
        print("\nFor demo mode (no camera required):")
        print("  python3 thermal_gold_analyzer.py --demo")
        return 0
    else:
        print("✗ SOME TESTS FAILED")
        print("="*60)
        print("\nPlease check the errors above and:")
        print("  1. Install missing packages: pip3 install -r requirements.txt")
        print("  2. Ensure all .py files are present")
        print("  3. Connect thermal camera if needed")
        return 1

if __name__ == "__main__":
    sys.exit(main())
