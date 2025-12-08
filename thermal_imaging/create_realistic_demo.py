#!/usr/bin/env python3
"""
Create a realistic demo video with exponential cooling curve
Temperature: 45°C → 29°C over 1:09 (69 seconds)
"""
import cv2
import numpy as np
import os

def generate_temperature_curve(duration_sec, fps, T_start=45, T_end=29):
    """
    Generate realistic exponential cooling curve with noise
    Following Newton's law of cooling: T(t) = T_inf + (T0 - T_inf) * exp(-k*t)
    """
    num_frames = int(duration_sec * fps)
    times = np.linspace(0, duration_sec, num_frames)

    # Calculate k for exponential decay to reach T_end at the end
    T_inf = 20  # Ambient temperature (room temp)
    T0 = T_start

    # Solve for k: T_end = T_inf + (T0 - T_inf) * exp(-k * duration)
    # exp(-k * duration) = (T_end - T_inf) / (T0 - T_inf)
    # -k * duration = ln((T_end - T_inf) / (T0 - T_inf))
    k = -np.log((T_end - T_inf) / (T0 - T_inf)) / duration_sec

    # Generate exponential cooling curve
    temperatures = T_inf + (T0 - T_inf) * np.exp(-k * times)

    # Add realistic noise (temperature sensor noise + thermal fluctuations)
    # Add smooth low-frequency noise (thermal fluctuations)
    low_freq_noise = 0.3 * np.sin(2 * np.pi * 0.1 * times) + 0.2 * np.cos(2 * np.pi * 0.05 * times)

    # Add high-frequency noise (sensor noise)
    high_freq_noise = np.random.normal(0, 0.15, num_frames)

    # Combine
    temperatures = temperatures + low_freq_noise + high_freq_noise

    # Ensure we end close to T_end
    temperatures[-1] = T_end + np.random.normal(0, 0.1)

    return times, temperatures, k

def temp_to_color(temp, min_temp=20, max_temp=50):
    """Convert temperature to thermal image color (BGR)"""
    # Normalize temperature to 0-1
    norm = np.clip((temp - min_temp) / (max_temp - min_temp), 0, 1)

    # Apply colormap: cool (blue) to hot (red)
    # Using ironbow colormap approximation
    if norm < 0.25:
        # Blue to Cyan
        r = 0
        g = int(255 * (norm / 0.25))
        b = 255
    elif norm < 0.5:
        # Cyan to Green
        r = 0
        g = 255
        b = int(255 * (1 - (norm - 0.25) / 0.25))
    elif norm < 0.75:
        # Green to Yellow
        r = int(255 * ((norm - 0.5) / 0.25))
        g = 255
        b = 0
    else:
        # Yellow to Red
        r = 255
        g = int(255 * (1 - (norm - 0.75) / 0.25))
        b = 0

    return (b, g, r)  # BGR format for OpenCV

def create_thermal_frame(temp, frame_size=(640, 480), object_pos=(320, 240), object_size=150):
    """Create a realistic thermal image frame"""
    height, width = frame_size
    frame = np.zeros((height, width, 3), dtype=np.uint8)

    # Background temperature (room temp ~20-25°C)
    bg_temp = np.random.normal(22, 1.5)
    bg_color = temp_to_color(bg_temp)
    frame[:] = bg_color

    # Add some background variation
    for _ in range(20):
        x = np.random.randint(0, width)
        y = np.random.randint(0, height)
        size = np.random.randint(30, 80)
        local_temp = np.random.normal(22, 2)
        color = temp_to_color(local_temp)
        cv2.circle(frame, (x, y), size, color, -1)

    # Blur background for realism
    frame = cv2.GaussianBlur(frame, (51, 51), 0)

    # Create hot object with temperature gradient
    # Center is hottest, edges cooler
    obj_x, obj_y = object_pos

    # Create circular gradient
    y_grid, x_grid = np.ogrid[:height, :width]
    dist_from_center = np.sqrt((x_grid - obj_x)**2 + (y_grid - obj_y)**2)

    # Object mask
    mask = dist_from_center <= object_size

    # Temperature gradient: hotter in center
    gradient_factor = 1 - (dist_from_center[mask] / object_size) ** 0.7

    # Apply temperature with gradient
    for i, (y, x) in enumerate(zip(*np.where(mask))):
        local_temp = temp * gradient_factor.flat[i] + (1 - gradient_factor.flat[i]) * bg_temp
        # Add slight random variation
        local_temp += np.random.normal(0, 0.5)
        color = temp_to_color(local_temp)
        frame[y, x] = color

    # Add slight blur to object edges for realism
    # Create a copy for blending
    blurred = cv2.GaussianBlur(frame, (11, 11), 0)

    # Blend at object edges
    edge_mask = (dist_from_center > object_size - 20) & (dist_from_center <= object_size + 10)
    frame[edge_mask] = blurred[edge_mask]

    # Add sensor noise
    noise = np.random.normal(0, 3, (height, width, 3)).astype(np.int16)
    frame = np.clip(frame.astype(np.int16) + noise, 0, 255).astype(np.uint8)

    return frame

def create_demo_video(output_path, duration_sec=69, fps=30):
    """Create realistic thermal cooling demo video"""
    print(f"Creating demo video: {output_path}")
    print(f"Duration: {duration_sec} seconds ({duration_sec//60}:{duration_sec%60:02d})")
    print(f"FPS: {fps}")

    # Generate temperature curve
    times, temperatures, k = generate_temperature_curve(duration_sec, fps)

    print(f"\nTemperature range: {temperatures[0]:.1f}°C → {temperatures[-1]:.1f}°C")
    print(f"Cooling rate constant k: {k:.4f} s⁻¹")

    # Video writer - using XVID codec which is more compatible
    fourcc = cv2.VideoWriter_fourcc(*'XVID')
    temp_output = output_path.replace('.mp4', '_temp.avi')
    out = cv2.VideoWriter(temp_output, fourcc, fps, (640, 480))

    if not out.isOpened():
        raise ValueError("Failed to create video writer")

    # Generate frames
    num_frames = len(temperatures)
    for i, (t, temp) in enumerate(zip(times, temperatures)):
        # Create frame
        frame = create_thermal_frame(temp)

        # Add timestamp overlay
        time_str = f"Time: {t:.1f}s"
        temp_str = f"Temp: {temp:.1f}°C"
        cv2.putText(frame, time_str, (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)
        cv2.putText(frame, temp_str, (10, 60), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)

        out.write(frame)

        # Progress
        if (i + 1) % 30 == 0 or i == num_frames - 1:
            progress = (i + 1) / num_frames * 100
            print(f"Progress: {progress:.1f}% ({i+1}/{num_frames} frames)")

    out.release()

    # Convert to MP4 using ffmpeg
    print("\nConverting to MP4...")
    import subprocess
    result = subprocess.run([
        'ffmpeg', '-y', '-i', temp_output,
        '-vcodec', 'libx264', '-acodec', 'aac',
        '-strict', 'experimental', '-b:v', '2000k',
        output_path
    ], capture_output=True, text=True)

    # Remove temp file
    if os.path.exists(temp_output):
        os.remove(temp_output)

    if result.returncode == 0:
        print(f"\nVideo created successfully: {output_path}")
        print(f"File size: {os.path.getsize(output_path) / (1024*1024):.2f} MB")
    else:
        print(f"FFmpeg conversion failed: {result.stderr}")
        print(f"Temp AVI file kept at: {temp_output}")

if __name__ == '__main__':
    # Create uploads directory if it doesn't exist
    os.makedirs('video_analyzer/uploads', exist_ok=True)

    # Create demo video
    output_path = 'video_analyzer/uploads/demo_cooling_2.mp4'
    create_demo_video(output_path, duration_sec=69, fps=30)

    print("\n✓ Demo video ready for upload!")
