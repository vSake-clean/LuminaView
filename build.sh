#!/bin/bash
echo "=== LuminaView Build Script ==="
echo ""

# Check Qt5
if ! command -v qmake &>/dev/null; then
    echo "[!] Installing Qt5..."
    sudo apt-get install -y qtbase5-dev qt5-qmake libqt5svg5-dev \
        qt5-image-formats-plugins 2>&1 | tail -3
fi

echo "[+] Qt5 OK: $(qmake --version | head -1)"
echo "[+] Building..."
qmake LuminaView.pro && make -j$(nproc)

if [ -f LuminaView ]; then
    strip LuminaView
    echo ""
    echo "✓ Build successful: ./LuminaView ($(du -sh LuminaView | cut -f1))"
    echo ""
    echo "Run: ./LuminaView [image_file]"
else
    echo "[!] Build failed."
    exit 1
fi
