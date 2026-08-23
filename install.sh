#!/bin/bash
set -e

DIR="$(cd "$(dirname "$0")" && pwd)"
SRC="$DIR"
BIN_SRC="$SRC/LuminaView"
BIN_NAME="LuminaView"

USE_SUDO=true
if [ "$1" = "--user" ] || [ "$1" = "-u" ]; then
    USE_SUDO=false
    PREFIX="$HOME/.local"
    DESKTOP_DIR="$HOME/.local/share/applications"
    ICON_DIR="$HOME/.local/share/icons/hicolor"
    MIME_DIR="$HOME/.local/share/mime/packages"
    DB_UPDATE="update-desktop-database $DESKTOP_DIR 2>/dev/null || true"
    MIME_UPDATE="update-mime-database $HOME/.local/share/mime 2>/dev/null || true"
else
    PREFIX="/usr/local"
    DESKTOP_DIR="/usr/share/applications"
    ICON_DIR="/usr/local/share/icons/hicolor"
    MIME_DIR="/usr/share/mime/packages"
    DB_UPDATE="sudo update-desktop-database 2>/dev/null || true"
    MIME_UPDATE="sudo update-mime-database /usr/share/mime 2>/dev/null || true"
fi

BIN_DST="$PREFIX/bin/$BIN_NAME"
DESKTOP_DST="$DESKTOP_DIR/$BIN_NAME.desktop"
ICON_SVG_DIR="$ICON_DIR/scalable/apps"

echo "=== LuminaView Install ==="

# 1. Build
if [ ! -f "$BIN_SRC" ] || [ "$1" == "--rebuild" ] || [ "$2" == "--rebuild" ]; then
    echo "[*] Building..."
    cd "$SRC"
    qmake LuminaView.pro && make -j$(nproc)
    strip LuminaView
fi

# 2. Binary
echo "[*] Installing binary..."
mkdir -p "$(dirname "$BIN_DST")"
cp "$BIN_SRC" "$BIN_DST"
chmod 755 "$BIN_DST"

# 3. Desktop
echo "[*] Installing .desktop..."
mkdir -p "$DESKTOP_DIR"
sed "s|/usr/local/bin|$PREFIX/bin|g" "$SRC/$BIN_NAME.desktop" > "$DESKTOP_DIR/$BIN_NAME.desktop"
chmod 644 "$DESKTOP_DIR/$BIN_NAME.desktop"

# 4. Icon SVG
echo "[*] Installing icon..."
mkdir -p "$ICON_SVG_DIR"
cp "$SRC/$BIN_NAME.svg" "$ICON_SVG_DIR/$BIN_NAME.svg"

# generate PNG icons
RENDER=""
if command -v rsvg-convert &>/dev/null; then
    RENDER="rsvg-convert"
elif command -v convert &>/dev/null; then
    RENDER="convert"
fi
if [ -n "$RENDER" ]; then
    echo "[*] Generating PNG icons..."
    for sz in 16 22 24 32 48 64 96 128 256; do
        d="$ICON_DIR/${sz}x${sz}/apps"
        mkdir -p "$d"
        if [ "$RENDER" = "rsvg-convert" ]; then
            rsvg-convert -w "$sz" -h "$sz" "$SRC/$BIN_NAME.svg" -o "$d/$BIN_NAME.png"
        else
            convert -background none -size "${sz}x${sz}" "$SRC/$BIN_NAME.svg" "$d/$BIN_NAME.png"
        fi
    done
fi

# 5. MIME
echo "[*] Installing MIME..."
mkdir -p "$MIME_DIR"
cp "$SRC/luminaview-mime.xml" "$MIME_DIR/luminaview.xml"

# 6. Set as default image viewer
echo "[*] Setting LuminaView as default image viewer..."

MIMES=(
    image/jpeg image/png image/webp image/gif image/bmp image/tiff
    image/svg+xml image/x-ico image/vnd.microsoft.icon
    image/x-portable-pixmap image/x-portable-graymap image/x-portable-bitmap
    image/x-xpixmap image/x-xbitmap image/x-tga image/x-dds
    image/x-canon-cr2 image/x-nikon-nef image/x-sony-arw image/x-adobe-dng
    image/x-olympus-orf image/x-panasonic-rw2 image/x-pentax-pef
)

for mime in "${MIMES[@]}"; do
    xdg-mime default "$BIN_NAME.desktop" "$mime" 2>/dev/null || true
done

# 7. Update databases
echo "[*] Updating databases..."
eval "$DB_UPDATE"
eval "$MIME_UPDATE"

echo ""
echo "=== Done ==="
echo "  Binary: $BIN_DST"
echo "  Default viewer set for all image types"
echo ""
echo "  Run: $BIN_NAME [file]"
if [ "$USE_SUDO" = false ]; then
    echo "  (user install — no sudo needed)"
fi
