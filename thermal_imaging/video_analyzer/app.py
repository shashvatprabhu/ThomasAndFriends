#!/usr/bin/env python3
"""
Flask web server for interactive thermal video analysis
"""
from flask import Flask, render_template, request, jsonify, send_from_directory
import os
import json
import numpy as np
import cv2
from scipy.optimize import curve_fit
from scipy.signal import find_peaks
import plotly.graph_objs as go
import plotly.utils

app = Flask(__name__)
app.config['UPLOAD_FOLDER'] = 'uploads'
app.config['MAX_CONTENT_LENGTH'] = 100 * 1024 * 1024  # 100MB max file size

# Ensure upload directory exists
os.makedirs(app.config['UPLOAD_FOLDER'], exist_ok=True)

# Reference thermal conductivity values (W/m·K) and cooling rates
REFERENCE_VALUES = {
    '24K': {'purity': 99.9, 'k_rate': 0.05, 'k_thermal': 318},
    '22K': {'purity': 91.7, 'k_rate': 0.04, 'k_thermal': 265},
    '18K': {'purity': 75.0, 'k_rate': 0.02, 'k_thermal': 200},
    '14K': {'purity': 58.3, 'k_rate': 0.01, 'k_thermal': 140},
    'Fake': {'purity': 0.0, 'k_rate': 0.01, 'k_thermal': 80},
}

# Impurity database with thermal conductivity (W/m·K)
# Higher thermal conductivity = faster cooling = higher k_rate
IMPURITIES_DATABASE = {
    'Copper': {
        'k_thermal': 401,
        'k_rate_min': 0.055,
        'k_rate_max': 0.070,
        'description': 'Excellent conductor, very fast cooling'
    },
    'Silver': {
        'k_thermal': 429,
        'k_rate_min': 0.060,
        'k_rate_max': 0.075,
        'description': 'Best thermal conductor, fastest cooling'
    },
    'Aluminium': {
        'k_thermal': 237,
        'k_rate_min': 0.035,
        'k_rate_max': 0.045,
        'description': 'Good conductor, fast cooling'
    },
    'Platinum': {
        'k_thermal': 71.6,
        'k_rate_min': 0.008,
        'k_rate_max': 0.015,
        'description': 'Moderate conductor, slower cooling'
    },
    'Zinc': {
        'k_thermal': 116,
        'k_rate_min': 0.015,
        'k_rate_max': 0.025,
        'description': 'Moderate conductor'
    },
    'Nickel': {
        'k_thermal': 90.9,
        'k_rate_min': 0.012,
        'k_rate_max': 0.020,
        'description': 'Moderate conductor'
    },
    'Iron': {
        'k_thermal': 80.4,
        'k_rate_min': 0.010,
        'k_rate_max': 0.018,
        'description': 'Poor conductor, slow cooling'
    },
    'Tungsten': {
        'k_thermal': 173,
        'k_rate_min': 0.025,
        'k_rate_max': 0.035,
        'description': 'Good conductor'
    },
    'Lead': {
        'k_thermal': 35.3,
        'k_rate_min': 0.004,
        'k_rate_max': 0.008,
        'description': 'Very poor conductor, very slow cooling'
    },
    'Cadmium': {
        'k_thermal': 96.8,
        'k_rate_min': 0.012,
        'k_rate_max': 0.020,
        'description': 'Moderate conductor'
    }
}

def auto_detect_roi(frame, min_size=100):
    """Auto-detect ROI based on brightest region"""
    if len(frame.shape) == 3:
        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    else:
        gray = frame

    _, thresh = cv2.threshold(gray, 0.7 * gray.max(), 255, cv2.THRESH_BINARY)
    thresh = thresh.astype(np.uint8)

    contours, _ = cv2.findContours(thresh, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

    if contours:
        largest = max(contours, key=cv2.contourArea)
        x, y, w, h = cv2.boundingRect(largest)
        if w >= min_size and h >= min_size:
            return (x, y, w, h)

    h, w = gray.shape
    size = min(w, h) // 2
    x = (w - size) // 2
    y = (h - size) // 2
    return (x, y, size, size)

def create_object_mask(gray_frame, threshold_percentile=70):
    """Create a mask to separate object from background based on temperature"""
    # Use adaptive thresholding based on percentile
    threshold_value = np.percentile(gray_frame, threshold_percentile)

    # Create binary mask - object pixels are above threshold
    mask = (gray_frame >= threshold_value).astype(np.uint8) * 255

    # Apply morphological operations to clean up the mask
    kernel = np.ones((5, 5), np.uint8)
    mask = cv2.morphologyEx(mask, cv2.MORPH_CLOSE, kernel)
    mask = cv2.morphologyEx(mask, cv2.MORPH_OPEN, kernel)

    # Find largest connected component (the main object)
    contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
    if contours:
        largest_contour = max(contours, key=cv2.contourArea)
        mask_clean = np.zeros_like(mask)
        cv2.drawContours(mask_clean, [largest_contour], -1, 255, -1)
        return mask_clean

    return mask

def extract_temperature_curve(video_path, roi=None, threshold_percentile=70):
    """Extract temperature curve from video - tracks only the hot object, not background"""
    cap = cv2.VideoCapture(video_path)

    if not cap.isOpened():
        raise ValueError("Could not open video file")

    fps = cap.get(cv2.CAP_PROP_FPS)
    frame_count = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))

    # Auto-detect ROI from first frame if not provided
    ret, first_frame = cap.read()
    if not ret:
        raise ValueError("Could not read first frame")

    if roi is None:
        roi = auto_detect_roi(first_frame)

    x, y, w, h = roi

    # Reset to beginning
    cap.set(cv2.CAP_PROP_POS_FRAMES, 0)

    temperatures = []
    times = []
    frame_idx = 0
    object_pixel_counts = []

    while True:
        ret, frame = cap.read()
        if not frame_idx % 50 == 0 and frame_idx > 0:
            frame_idx += 1
            continue

        if not ret:
            break

        # Extract ROI
        roi_frame = frame[y:y+h, x:x+w]

        # Convert to grayscale
        if len(roi_frame.shape) == 3:
            gray = cv2.cvtColor(roi_frame, cv2.COLOR_BGR2GRAY)
        else:
            gray = roi_frame

        # Create mask to identify object pixels (hottest regions)
        object_mask = create_object_mask(gray, threshold_percentile)

        # Extract temperature only from object pixels
        object_pixels = gray[object_mask > 0]

        if len(object_pixels) > 0:
            # Use mean of object pixels only
            temp = np.mean(object_pixels)
            object_pixel_counts.append(len(object_pixels))
        else:
            # Fallback to entire ROI if no object detected
            temp = np.mean(gray)
            object_pixel_counts.append(0)

        temperatures.append(temp)
        times.append(frame_idx / fps)

        frame_idx += 1

    cap.release()

    return {
        'times': times,
        'temperatures': temperatures,
        'fps': fps,
        'frame_count': frame_count,
        'roi': roi,
        'object_pixel_counts': object_pixel_counts
    }

def analyze_cooling_curve(times, temperatures):
    """Analyze cooling curve and extract parameters"""
    times = np.array(times)
    temps = np.array(temperatures)

    # Find peak
    peaks, properties = find_peaks(temps, prominence=5)

    if len(peaks) > 0:
        peak_idx = peaks[0]
    else:
        peak_idx = np.argmax(temps)

    peak_time = times[peak_idx]
    peak_temp = temps[peak_idx]

    # Cooling phase (after peak)
    cooling_times = times[peak_idx:]
    cooling_temps = temps[peak_idx:]

    if len(cooling_temps) < 10:
        return None

    # Normalize temperature
    T_inf = cooling_temps[-1]
    T_delta = cooling_temps - T_inf
    T_delta_0 = T_delta[0]

    if T_delta_0 <= 0:
        return None

    theta = T_delta / T_delta_0
    t_rel = cooling_times - cooling_times[0]

    # Fit exponential decay
    def exp_decay(t, k):
        return np.exp(-k * t)

    try:
        valid_mask = (theta > 0) & (t_rel >= 0)
        popt, _ = curve_fit(exp_decay, t_rel[valid_mask], theta[valid_mask], p0=[0.01], maxfev=10000)
        k = popt[0]

        # Calculate R²
        y_pred = exp_decay(t_rel[valid_mask], k)
        ss_res = np.sum((theta[valid_mask] - y_pred) ** 2)
        ss_tot = np.sum((theta[valid_mask] - np.mean(theta[valid_mask])) ** 2)
        r_squared = 1 - (ss_res / ss_tot)

        # Calculate RMSE
        rmse = np.sqrt(np.mean((theta[valid_mask] - y_pred) ** 2))

    except Exception as e:
        return None

    # Estimate purity based on cooling rate
    min_diff = float('inf')
    best_grade = '24K'

    for grade, props in REFERENCE_VALUES.items():
        diff = abs(k - props['k_rate'])
        if diff < min_diff:
            min_diff = diff
            best_grade = grade

    purity = REFERENCE_VALUES[best_grade]['purity']

    # Confidence based on R²
    confidence = max(0, min(100, r_squared * 100))

    # Analyze impurities - rule out based on cooling rate
    impurity_analysis = []
    tolerance = 0.005  # ±0.005 tolerance for ruling out

    for impurity, props in IMPURITIES_DATABASE.items():
        # Check if measured cooling rate falls within this impurity's range
        # Add tolerance to account for measurement error
        if k >= (props['k_rate_min'] - tolerance) and k <= (props['k_rate_max'] + tolerance):
            status = 'POSSIBLE'
            likelihood = 'High' if abs(k - (props['k_rate_min'] + props['k_rate_max'])/2) < 0.01 else 'Medium'
        elif k < (props['k_rate_min'] - tolerance):
            status = 'RULED OUT'
            likelihood = 'Zero'
        else:  # k > props['k_rate_max'] + tolerance
            status = 'RULED OUT'
            likelihood = 'Zero'

        impurity_analysis.append({
            'name': impurity,
            'k_thermal': props['k_thermal'],
            'k_rate_range': f"{props['k_rate_min']:.3f} - {props['k_rate_max']:.3f}",
            'status': status,
            'likelihood': likelihood,
            'description': props['description']
        })

    # Sort by thermal conductivity (descending)
    impurity_analysis.sort(key=lambda x: x['k_thermal'], reverse=True)

    # Count how many are possible
    possible_impurities = [imp for imp in impurity_analysis if imp['status'] == 'POSSIBLE']
    ruled_out_impurities = [imp for imp in impurity_analysis if imp['status'] == 'RULED OUT']

    return {
        'peak_time': float(peak_time),
        'peak_temp': float(peak_temp),
        'final_temp': float(T_inf),
        'temp_delta': float(peak_temp - T_inf),
        'cooling_rate': float(k),
        'r_squared': float(r_squared),
        'rmse': float(rmse),
        'estimated_grade': best_grade,
        'estimated_purity': float(purity),
        'confidence': float(confidence),
        'normalized_times': t_rel.tolist(),
        'normalized_theta': theta.tolist(),
        'impurity_analysis': impurity_analysis,
        'possible_impurities_count': len(possible_impurities),
        'ruled_out_impurities_count': len(ruled_out_impurities)
    }

@app.route('/')
def index():
    """Render main page"""
    return render_template('index.html')

@app.route('/videos/<path:filename>')
def serve_video(filename):
    """Serve video files"""
    return send_from_directory('.', filename)

@app.route('/analyze', methods=['POST'])
def analyze():
    """Analyze video and return interactive plot data"""
    try:
        video_name = request.json.get('video_path')
        video_source = request.json.get('video_source', 'demo')
        roi = request.json.get('roi', None)

        # Determine the full path based on source
        if video_source == 'uploaded':
            video_path = os.path.join(app.config['UPLOAD_FOLDER'], video_name)
        else:
            video_path = video_name

        if not video_path or not os.path.exists(video_path):
            return jsonify({'error': 'Video file not found'}), 400

        # Extract temperature curve
        data = extract_temperature_curve(video_path, roi)

        # Analyze cooling curve
        analysis = analyze_cooling_curve(data['times'], data['temperatures'])

        if analysis is None:
            return jsonify({'error': 'Failed to analyze cooling curve'}), 400

        # Create interactive plots

        # Plot 1: Temperature vs Time
        temp_trace = go.Scatter(
            x=data['times'],
            y=data['temperatures'],
            mode='lines+markers',
            name='Temperature',
            line=dict(color='rgb(255, 127, 14)', width=2),
            marker=dict(size=4),
            hovertemplate='Time: %{x:.2f}s<br>Temp: %{y:.2f}<extra></extra>'
        )

        peak_trace = go.Scatter(
            x=[analysis['peak_time']],
            y=[analysis['peak_temp']],
            mode='markers',
            name='Peak',
            marker=dict(color='red', size=12, symbol='star'),
            hovertemplate='Peak<br>Time: %{x:.2f}s<br>Temp: %{y:.2f}<extra></extra>'
        )

        temp_layout = go.Layout(
            title='Temperature vs Time',
            xaxis=dict(title='Time (s)', gridcolor='#ddd'),
            yaxis=dict(title='Temperature (relative)', gridcolor='#ddd'),
            hovermode='closest',
            plot_bgcolor='white',
            paper_bgcolor='white',
            font=dict(size=12)
        )

        temp_fig = go.Figure(data=[temp_trace, peak_trace], layout=temp_layout)

        # Plot 2: Normalized Cooling Curve
        normalized_trace = go.Scatter(
            x=analysis['normalized_times'],
            y=analysis['normalized_theta'],
            mode='markers',
            name='Measured',
            marker=dict(color='blue', size=6, opacity=0.6),
            hovertemplate='Time: %{x:.2f}s<br>θ: %{y:.4f}<extra></extra>'
        )

        # Fit curve
        t_fit = np.linspace(0, max(analysis['normalized_times']), 100)
        theta_fit = np.exp(-analysis['cooling_rate'] * t_fit)

        fit_trace = go.Scatter(
            x=t_fit.tolist(),
            y=theta_fit.tolist(),
            mode='lines',
            name=f'Fit (k={analysis["cooling_rate"]:.3f})',
            line=dict(color='red', width=3, dash='dash'),
            hovertemplate='Time: %{x:.2f}s<br>θ: %{y:.4f}<extra></extra>'
        )

        cooling_layout = go.Layout(
            title=f'Normalized Cooling Curve (R²={analysis["r_squared"]:.4f})',
            xaxis=dict(title='Time (s)', gridcolor='#ddd'),
            yaxis=dict(title='θ(t) = (T - T∞) / (T₀ - T∞)', gridcolor='#ddd'),
            hovermode='closest',
            plot_bgcolor='white',
            paper_bgcolor='white',
            font=dict(size=12)
        )

        cooling_fig = go.Figure(data=[normalized_trace, fit_trace], layout=cooling_layout)

        # Convert to JSON
        result = {
            'analysis': analysis,
            'reference_values': REFERENCE_VALUES,
            'temp_plot': json.loads(plotly.utils.PlotlyJSONEncoder().encode(temp_fig)),
            'cooling_plot': json.loads(plotly.utils.PlotlyJSONEncoder().encode(cooling_fig)),
            'video_info': {
                'fps': data['fps'],
                'frame_count': data['frame_count'],
                'duration': data['frame_count'] / data['fps'],
                'roi': data['roi']
            }
        }

        return jsonify(result)

    except Exception as e:
        import traceback
        traceback.print_exc()
        return jsonify({'error': str(e)}), 500

@app.route('/list_videos')
def list_videos():
    """List available demo videos"""
    videos = []

    # List videos from current directory
    for f in os.listdir('.'):
        if f.endswith(('.mp4', '.avi', '.mov', '.mkv')):
            videos.append({'name': f, 'source': 'demo'})

    # List videos from uploads directory
    if os.path.exists(app.config['UPLOAD_FOLDER']):
        for f in os.listdir(app.config['UPLOAD_FOLDER']):
            if f.endswith(('.mp4', '.avi', '.mov', '.mkv')):
                videos.append({'name': f, 'source': 'uploaded'})

    return jsonify({'videos': videos})

@app.route('/upload', methods=['POST'])
def upload_video():
    """Handle video file upload"""
    try:
        if 'video' not in request.files:
            return jsonify({'error': 'No file provided'}), 400

        file = request.files['video']

        if file.filename == '':
            return jsonify({'error': 'No file selected'}), 400

        # Check file extension
        allowed_extensions = {'.mp4', '.avi', '.mov', '.mkv'}
        file_ext = os.path.splitext(file.filename)[1].lower()

        if file_ext not in allowed_extensions:
            return jsonify({'error': 'Invalid file type. Allowed: MP4, AVI, MOV, MKV'}), 400

        # Save file
        filename = file.filename
        filepath = os.path.join(app.config['UPLOAD_FOLDER'], filename)
        file.save(filepath)

        return jsonify({
            'success': True,
            'filename': filename,
            'message': 'Video uploaded successfully'
        })

    except Exception as e:
        import traceback
        traceback.print_exc()
        return jsonify({'error': str(e)}), 500

@app.route('/uploads/<path:filename>')
def serve_uploaded_video(filename):
    """Serve uploaded video files"""
    return send_from_directory(app.config['UPLOAD_FOLDER'], filename)

if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0', port=5000)
