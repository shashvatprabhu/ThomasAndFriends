# Thermal Gold Analyzer - Quick Start Guide

## ✨ What We Built

A complete **Linux-native thermal imaging application** for gold purity analysis using:
- **UVC thermal camera** support (standard USB Video Class protocol)
- **Real-time thermal imaging** with multiple colormaps
- **Cooling curve analysis** based on thermal conductivity
- **Gold purity estimation** (24K, 22K, 18K, 14K, or Fake)

## 🚀 Quick Start

### 1. First-Time Setup
```bash
# Fix NumPy compatibility (already done!)
pip3 install --user "numpy<2.0.0"

# Test the system
python3 test_system.py
```

### 2. Run Demo Mode (No Camera Required)
```bash
python3 thermal_gold_analyzer.py --demo
```
This will show you how the cooling curve analysis works with synthetic data.

### 3. Run with Your Thermal Camera
```bash
# Connect your thermal camera via USB
python3 thermal_gold_analyzer.py
```

## 📋 How to Use

### With Camera Connected:

1. **Launch the app**: `python3 thermal_gold_analyzer.py`

2. **Position your gold sample** under the thermal camera

3. **Press 'r'** to auto-detect the hot spot region (ROI)

4. **Apply heat pulse** to the gold:
   - Use a small LED light
   - Use a camera flash
   - Use a heat gun (briefly, from distance)

5. **Press 'a'** to start recording the cooling curve

6. **Wait 20-30 seconds** while the gold cools down

7. **Press 's'** to stop and analyze

8. **View the results**:
   - Estimated gold grade (24K, 22K, 18K, 14K, Fake)
   - Purity percentage
   - Confidence score
   - Cooling rate constant

9. **Press 'p'** to see detailed plots of the cooling curve

### Keyboard Controls:
- **r** - Auto-detect ROI (hottest region)
- **a** - Start analysis (begin recording)
- **s** - Stop analysis and show results
- **c** - Change colormap (jet, hot, cool, etc.)
- **p** - Plot cooling curve
- **w** - Save current frame
- **q** - Quit

## 📊 Understanding Results

### Gold Purity Standards:
- **24K (99.9%)**: Fastest cooling, ~310 W/m·K conductivity
- **22K (91.7%)**: Fast cooling, ~265 W/m·K
- **18K (75%)**: Moderate cooling, ~200 W/m·K
- **14K (58.3%)**: Slower cooling, ~140 W/m·K
- **Fake/Plated**: Slowest cooling, ~80 W/m·K

### Confidence Score:
- **>80%**: Very reliable result
- **60-80%**: Good result
- **<60%**: Questionable, try again with better conditions

## 🔧 Troubleshooting

### Camera Not Detected
```bash
# Check connected cameras
python3 camera_detector.py

# List USB devices
lsusb

# Check video devices
ls -l /dev/video*
```

### NumPy Compatibility Issues
```bash
# Run setup script
./setup.sh

# Or manually
pip3 install --user "numpy<2.0.0"
```

### Poor Analysis Results
- Ensure consistent heat pulse
- Avoid air currents/fans
- Keep camera steady
- Record for at least 20 seconds
- Ensure ROI covers the sample

## 📁 Project Files

```
thermal_imaging/
├── thermal_gold_analyzer.py     # Main application ⭐
├── camera_detector.py           # Camera detection
├── thermal_capture.py           # Image capture
├── thermal_processing.py        # Image processing
├── gold_purity_analyzer.py      # Analysis algorithms
├── test_system.py               # System tests
├── setup.sh                     # Setup script
├── requirements.txt             # Dependencies
├── README.md                    # Full documentation
└── QUICKSTART.md                # This file
```

## 🎯 What Makes This Special

### vs. Windows Application:
✅ **Native Linux** - No Wine/VM needed
✅ **Open Source** - Fully customizable
✅ **Standard Protocol** - Works with any UVC thermal camera
✅ **Modular Design** - Easy to extend
✅ **Python-based** - Easy to modify

### Technology Used:
- **OpenCV** for image capture and processing
- **NumPy** for numerical computations
- **Matplotlib** for visualization
- **SciPy** for curve fitting and signal processing
- **UVC Protocol** for camera communication

## 🧪 Testing Individual Modules

```bash
# Test camera detection
python3 camera_detector.py

# Test thermal capture (opens camera)
python3 thermal_capture.py

# Test image processing (synthetic data)
python3 thermal_processing.py

# Test cooling curve analysis (synthetic data)
python3 gold_purity_analyzer.py

# Test complete system
python3 test_system.py
```

## 🎓 How It Works

### Scientific Principle:
Gold's thermal conductivity determines how fast it cools after heating:
- **Pure gold** has high conductivity → cools quickly
- **Gold alloys** have lower conductivity → cool slower
- **Fake gold** has very low conductivity → cools very slowly

### Algorithm:
1. Record temperature over time (cooling curve)
2. Fit exponential model: `T(t) = T_ambient + (T_initial - T_ambient)·e^(-kt)`
3. Extract cooling rate constant `k`
4. Compare `k` to known gold standards
5. Estimate purity and confidence

## 📈 Next Steps

Want to enhance the application?
- Add database for storing results
- Create PDF reports
- Add multi-point tracking
- Implement machine learning for better accuracy
- Add calibration wizard
- Create web interface

## 💡 Tips for Best Results

1. **Consistent Heat**: Use the same heat source and duration
2. **Stable Environment**: Minimize air movement
3. **Good ROI**: Ensure ROI covers the entire sample
4. **Sufficient Data**: Record for at least 20-30 seconds
5. **Multiple Tests**: Run several tests and average results

## 🆘 Need Help?

- Read full documentation: `README.md`
- Run system test: `python3 test_system.py`
- Check camera: `python3 camera_detector.py`
- Try demo first: `python3 thermal_gold_analyzer.py --demo`

---

**Ready to test gold purity? Start with demo mode!**

```bash
python3 thermal_gold_analyzer.py --demo
```

Then connect your thermal camera and run:

```bash
python3 thermal_gold_analyzer.py
```

Happy analyzing! 🔬✨
