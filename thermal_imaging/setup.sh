#!/bin/bash
# Setup script for Thermal Gold Analyzer

echo "╔══════════════════════════════════════════════════════════╗"
echo "║        Thermal Gold Analyzer - Setup Script              ║"
echo "╚══════════════════════════════════════════════════════════╝"
echo ""

# Check Python version
echo "Checking Python version..."
python3 --version

# Fix NumPy compatibility issues
echo ""
echo "Fixing NumPy compatibility..."
echo "This will downgrade NumPy to be compatible with system packages."
echo ""

pip3 install --user "numpy<2.0.0"

echo ""
echo "✓ Setup complete!"
echo ""
echo "You can now run:"
echo "  python3 test_system.py    # Test the system"
echo "  python3 thermal_gold_analyzer.py --demo    # Run demo"
echo "  python3 thermal_gold_analyzer.py    # Run with camera"
echo ""
