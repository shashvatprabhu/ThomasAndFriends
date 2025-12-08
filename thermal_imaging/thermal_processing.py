#!/usr/bin/env python3
"""
Thermal Image Processing Module
Advanced processing for thermal images including filtering, enhancement, and analysis
"""

import cv2
import numpy as np
from scipy import signal
from scipy.ndimage import gaussian_filter


class ThermalProcessor:
    """Advanced thermal image processing"""

    def __init__(self):
        self.calibration_offset = 0
        self.calibration_scale = 1.0

    def denoise(self, thermal_image, method='gaussian', **kwargs):
        """
        Denoise thermal image
        Args:
            thermal_image: Input grayscale thermal image
            method: 'gaussian', 'bilateral', or 'nlm' (non-local means)
        """
        if method == 'gaussian':
            sigma = kwargs.get('sigma', 1.0)
            return gaussian_filter(thermal_image, sigma=sigma)

        elif method == 'bilateral':
            d = kwargs.get('d', 9)
            sigma_color = kwargs.get('sigma_color', 75)
            sigma_space = kwargs.get('sigma_space', 75)
            # Convert to uint8 for bilateral filter
            img_uint8 = cv2.normalize(thermal_image, None, 0, 255, cv2.NORM_MINMAX).astype(np.uint8)
            filtered = cv2.bilateralFilter(img_uint8, d, sigma_color, sigma_space)
            return filtered.astype(float)

        elif method == 'nlm':
            h = kwargs.get('h', 10)
            # Convert to uint8 for NLM
            img_uint8 = cv2.normalize(thermal_image, None, 0, 255, cv2.NORM_MINMAX).astype(np.uint8)
            filtered = cv2.fastNlMeansDenoising(img_uint8, None, h, 7, 21)
            return filtered.astype(float)

        return thermal_image

    def enhance_contrast(self, thermal_image, method='clahe'):
        """
        Enhance contrast of thermal image
        Args:
            thermal_image: Input thermal image
            method: 'clahe' (Contrast Limited Adaptive Histogram Equalization) or 'histogram'
        """
        # Convert to uint8
        img_uint8 = cv2.normalize(thermal_image, None, 0, 255, cv2.NORM_MINMAX).astype(np.uint8)

        if method == 'clahe':
            clahe = cv2.createCLAHE(clipLimit=2.0, tileGridSize=(8, 8))
            enhanced = clahe.apply(img_uint8)
        elif method == 'histogram':
            enhanced = cv2.equalizeHist(img_uint8)
        else:
            enhanced = img_uint8

        return enhanced

    def apply_colormap(self, thermal_image, colormap='jet'):
        """
        Apply colormap to thermal image
        Args:
            thermal_image: Grayscale thermal image
            colormap: 'jet', 'hot', 'cool', 'rainbow', 'magma', 'inferno'
        Returns:
            Colored thermal image (BGR)
        """
        # Normalize to 0-255
        normalized = cv2.normalize(thermal_image, None, 0, 255, cv2.NORM_MINMAX).astype(np.uint8)

        # Apply colormap
        colormap_dict = {
            'jet': cv2.COLORMAP_JET,
            'hot': cv2.COLORMAP_HOT,
            'cool': cv2.COLORMAP_COOL,
            'rainbow': cv2.COLORMAP_RAINBOW,
            'magma': cv2.COLORMAP_MAGMA,
            'inferno': cv2.COLORMAP_INFERNO,
            'turbo': cv2.COLORMAP_TURBO
        }

        cmap = colormap_dict.get(colormap, cv2.COLORMAP_JET)
        colored = cv2.applyColorMap(normalized, cmap)

        return colored

    def detect_hot_spots(self, thermal_image, threshold_percentile=90):
        """
        Detect hot spots in thermal image
        Args:
            thermal_image: Input thermal image
            threshold_percentile: Percentile threshold (default: 90)
        Returns:
            Binary mask of hot spots and list of contours
        """
        threshold = np.percentile(thermal_image, threshold_percentile)

        # Create binary mask
        hot_mask = (thermal_image > threshold).astype(np.uint8) * 255

        # Find contours
        contours, _ = cv2.findContours(hot_mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

        return hot_mask, contours

    def get_roi_statistics(self, thermal_image, roi):
        """
        Get statistics for a region of interest
        Args:
            thermal_image: Input thermal image
            roi: (x, y, width, height)
        Returns:
            Dictionary with statistics
        """
        x, y, w, h = roi
        roi_data = thermal_image[y:y+h, x:x+w]

        stats = {
            'mean': np.mean(roi_data),
            'std': np.std(roi_data),
            'min': np.min(roi_data),
            'max': np.max(roi_data),
            'median': np.median(roi_data),
            'percentile_25': np.percentile(roi_data, 25),
            'percentile_75': np.percentile(roi_data, 75)
        }

        return stats

    def draw_temperature_profile(self, thermal_image, start_point, end_point):
        """
        Extract temperature profile along a line
        Args:
            thermal_image: Input thermal image
            start_point: (x1, y1)
            end_point: (x2, y2)
        Returns:
            1D array of temperature values along the line
        """
        x1, y1 = start_point
        x2, y2 = end_point

        # Get line coordinates
        length = int(np.hypot(x2-x1, y2-y1))
        x = np.linspace(x1, x2, length)
        y = np.linspace(y1, y2, length)

        # Extract values
        profile = thermal_image[y.astype(int), x.astype(int)]

        return profile

    def calculate_gradient(self, thermal_image):
        """
        Calculate temperature gradient
        Returns:
            Gradient magnitude and direction
        """
        # Calculate gradients
        grad_x = cv2.Sobel(thermal_image, cv2.CV_64F, 1, 0, ksize=3)
        grad_y = cv2.Sobel(thermal_image, cv2.CV_64F, 0, 1, ksize=3)

        # Calculate magnitude and direction
        magnitude = np.sqrt(grad_x**2 + grad_y**2)
        direction = np.arctan2(grad_y, grad_x)

        return magnitude, direction

    def segment_temperature_regions(self, thermal_image, num_levels=5):
        """
        Segment image into temperature regions
        Args:
            thermal_image: Input thermal image
            num_levels: Number of temperature levels
        Returns:
            Segmented image
        """
        # Normalize to 0-255
        normalized = cv2.normalize(thermal_image, None, 0, 255, cv2.NORM_MINMAX).astype(np.uint8)

        # Calculate thresholds
        thresholds = np.linspace(0, 255, num_levels + 1)

        # Create segmented image
        segmented = np.zeros_like(normalized)
        for i in range(num_levels):
            mask = (normalized >= thresholds[i]) & (normalized < thresholds[i+1])
            segmented[mask] = int(255 * (i / num_levels))

        return segmented


def main():
    """Test thermal processing"""
    print("=== Thermal Image Processing Test ===\n")

    # Create a synthetic thermal image for testing
    height, width = 240, 320
    x, y = np.meshgrid(np.linspace(-1, 1, width), np.linspace(-1, 1, height))

    # Create a synthetic heat source (Gaussian)
    synthetic_thermal = 100 * np.exp(-(x**2 + y**2) / 0.2)

    # Add noise
    noise = np.random.normal(0, 5, synthetic_thermal.shape)
    noisy_thermal = synthetic_thermal + noise

    # Initialize processor
    processor = ThermalProcessor()

    # Test denoising
    print("Testing denoising...")
    denoised = processor.denoise(noisy_thermal, method='gaussian', sigma=2.0)

    # Test contrast enhancement
    print("Testing contrast enhancement...")
    enhanced = processor.enhance_contrast(denoised)

    # Test colormap
    print("Testing colormap...")
    colored = processor.apply_colormap(enhanced, colormap='jet')

    # Test hot spot detection
    print("Testing hot spot detection...")
    hot_mask, contours = processor.detect_hot_spots(denoised, threshold_percentile=90)
    print(f"  Found {len(contours)} hot spots")

    # Test ROI statistics
    print("Testing ROI statistics...")
    roi = (100, 80, 120, 80)
    stats = processor.get_roi_statistics(denoised, roi)
    print(f"  ROI mean temperature: {stats['mean']:.2f}")
    print(f"  ROI std: {stats['std']:.2f}")

    # Display results
    cv2.imshow('Original (Noisy)', cv2.normalize(noisy_thermal, None, 0, 255, cv2.NORM_MINMAX).astype(np.uint8))
    cv2.imshow('Denoised', cv2.normalize(denoised, None, 0, 255, cv2.NORM_MINMAX).astype(np.uint8))
    cv2.imshow('Enhanced', enhanced)
    cv2.imshow('Colored', colored)
    cv2.imshow('Hot Spots', hot_mask)

    print("\nPress any key to close...")
    cv2.waitKey(0)
    cv2.destroyAllWindows()


if __name__ == "__main__":
    main()
