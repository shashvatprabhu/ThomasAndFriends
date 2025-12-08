# Thermal Gold Purity Analyzer

A Linux-native thermal imaging application for analyzing gold purity using non-contact, non-destructive thermal analysis.

## Overview

This application uses a UVC-compatible thermal camera to analyze the cooling curves of gold items after a heat pulse. Pure gold has high thermal conductivity and cools rapidly, while gold alloys and fake gold cool more slowly. By analyzing the cooling pattern, the application estimates the purity of the gold sample.

## How It Works

1. **Heat Pulse**: A small, gentle heat source (LED, flash, or heat gun) is applied to the gold item
2. **Thermal Capture**: The thermal camera records the temperature over time
3. **Cooling Curve Analysis**: The application fits an exponential cooling model to the data
4. **Purity Estimation**: Based on the cooling rate constant, the application estimates the gold purity

### Gold Purity Standards

- **24K (99.9% pure)**: ~310 W/m·K thermal conductivity - fastest cooling
- **22K (91.7% pure)**: ~265 W/m·K - fast cooling
- **18K (75% pure)**: ~200 W/m·K - moderate cooling
- **14K (58.3% pure)**: ~140 W/m·K - slower cooling
- **Fake/Plated**: ~80 W/m·K - slowest cooling

## Requirements

### Hardware
- UVC-compatible thermal imaging camera (USB connection)
- Computer running Ubuntu/Linux
- Optional: Small heat source (LED, flash, heat gun)

### Software
- Python 3.7+
- OpenCV
- NumPy
- Matplotlib
- SciPy

## Installation

1. **Install system dependencies** (if needed):
```bash
sudo apt update
sudo apt install python3-opencv python3-numpy python3-matplotlib python3-scipy
```

2. **Or install via pip**:
```bash
pip3 install -r requirements.txt
```

3. **Make scripts executable**:
```bash
chmod +x *.py
```

## Usage

### Main Application

Run the complete thermal analysis application:

```bash
python3 thermal_gold_analyzer.py
```

With specific camera:
```bash
python3 thermal_gold_analyzer.py --camera 1
```

Demo mode (synthetic data):
```bash
python3 thermal_gold_analyzer.py --demo
```

### Application Controls

- **r** - Auto-detect ROI (Region of Interest) on the hottest area
- **a** - Start recording cooling curve analysis
- **s** - Stop recording and generate report
- **c** - Change colormap (jet, hot, cool, rainbow, magma, etc.)
- **p** - Plot cooling curve and analysis results
- **w** - Save current thermal frame
- **q** - Quit application

### Module Testing

Test individual modules:

```bash
# Test camera detection
python3 camera_detector.py

# Test thermal capture
python3 thermal_capture.py

# Test thermal processing
python3 thermal_processing.py

# Test cooling curve analysis
python3 gold_purity_analyzer.py
```

## Workflow

1. **Connect your thermal camera** via USB
2. **Launch the application**: `python3 thermal_gold_analyzer.py`
3. **Position the camera** to view your gold sample
4. **Auto-detect ROI** by pressing 'r' when viewing the sample
5. **Apply heat pulse** to the gold sample (quick flash or LED)
6. **Start recording** by pressing 'a'
7. **Wait for cooling** (~20-30 seconds)
8. **Stop recording** by pressing 's'
9. **View results** - the application will display:
   - Estimated gold purity (24K, 22K, 18K, 14K, or Fake)
   - Confidence level
   - Cooling rate constant
   - Thermal time constant
10. **Plot results** by pressing 'p' to see detailed cooling curves

## Project Structure

```
thermal_imaging/
├── camera_detector.py           # UVC camera detection
├── thermal_capture.py           # Thermal image capture and frame processing
├── thermal_processing.py        # Advanced thermal image processing
├── gold_purity_analyzer.py      # Cooling curve analysis and purity estimation
├── thermal_gold_analyzer.py     # Main integrated application
├── requirements.txt             # Python dependencies
└── README.md                    # This file
```

## Modules

### camera_detector.py
- Detects UVC video devices
- Lists camera properties (resolution, FPS)
- Identifies thermal camera

### thermal_capture.py
- Captures frames from thermal camera
- Converts to thermal representation
- Applies colormaps
- Tracks temperature hotspots

### thermal_processing.py
- Denoising (Gaussian, bilateral, NLM)
- Contrast enhancement (CLAHE)
- Hot spot detection
- Temperature gradient calculation
- ROI statistics

### gold_purity_analyzer.py
- Records cooling curve data
- Fits exponential cooling model
- Calculates cooling rate constant
- Estimates gold purity
- Generates comprehensive reports

### thermal_gold_analyzer.py
- Main application integrating all modules
- Interactive GUI with OpenCV
- Real-time thermal visualization
- Automated ROI detection
- Complete analysis workflow

## Technical Details

### Cooling Model

The application uses Newton's Law of Cooling:

```
T(t) = T_ambient + (T_initial - T_ambient) × e^(-kt)
```

Where:
- T(t) = temperature at time t
- T_ambient = ambient temperature
- T_initial = initial temperature after heat pulse
- k = cooling rate constant (larger k = faster cooling = higher purity)

### Analysis Algorithm

1. **Data Collection**: Records temperature over time (typically 20-30 seconds)
2. **Smoothing**: Applies Savitzky-Golay filter to reduce noise
3. **Curve Fitting**: Fits exponential model using non-linear least squares
4. **Quality Metrics**: Calculates R², RMSE to ensure good fit
5. **Purity Estimation**: Compares cooling rate constant to gold standards

## Troubleshooting

### Camera Not Detected
- Check USB connection
- Run `lsusb` to verify device is recognized
- Try different camera index: `--camera 0`, `--camera 1`, etc.
- Check permissions: `ls -l /dev/video*`

### Poor Analysis Results
- Ensure good thermal contact between heat source and sample
- Wait for complete cooling cycle (20-30 seconds minimum)
- Avoid external heat sources or air currents
- Use consistent heat pulse intensity
- Ensure ROI covers the sample completely

### Low Confidence Scores
- Collect more data points (longer recording time)
- Apply more uniform heat pulse
- Reduce environmental interference
- Ensure camera is stable and focused

## Advanced Usage

### Custom Camera Settings

Edit the thermal_capture.py file to adjust camera parameters:

```python
# Try different resolutions
self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, 384)
self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 288)
```

### Calibration

For absolute temperature readings, calibrate your camera:

1. Measure known temperature references
2. Create calibration curve
3. Update `ThermalProcessor.calibration_offset` and `calibration_scale`

### Integration

Import modules into your own applications:

```python
from thermal_capture import ThermalCamera
from gold_purity_analyzer import CoolingCurveAnalyzer

camera = ThermalCamera(0)
camera.open()

analyzer = CoolingCurveAnalyzer()
analyzer.start_recording()

# Your analysis code here...
```

## Comparison with Windows Application

This Linux application provides:
- ✅ Native Linux compatibility (no Wine/VM needed)
- ✅ Open-source codebase
- ✅ Modular architecture for customization
- ✅ UVC standard protocol support
- ✅ All core features of the original Windows app
- ✅ Enhanced visualization options
- ✅ Command-line and GUI interfaces

## Future Enhancements

Potential improvements:
- [ ] Multi-language support
- [ ] Database for storing analysis results
- [ ] Export reports to PDF
- [ ] Multiple ROI tracking
- [ ] Real-time comparison with known samples
- [ ] Machine learning for improved purity estimation
- [ ] Web interface for remote analysis

## License

This is an educational project for thermal analysis. Use responsibly.

## Contributing

Contributions welcome! Areas for improvement:
- Support for more thermal camera models
- Improved calibration methods
- Better UI/UX
- Additional analysis algorithms
- Documentation improvements

## Contact

For issues or questions, please refer to the documentation or create an issue in the repository.

---

**Note**: This application is designed for educational and research purposes. For commercial gold testing, please use certified professional equipment and methods.
