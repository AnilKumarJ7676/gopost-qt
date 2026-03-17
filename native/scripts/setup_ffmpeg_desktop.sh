#!/usr/bin/env bash
set -euo pipefail

echo "=== GoPost FFmpeg Desktop Setup ==="
echo ""

case "$(uname -s)" in
    Linux*)
        echo "Installing FFmpeg development libraries on Linux..."
        if command -v apt-get &>/dev/null; then
            sudo apt-get update
            sudo apt-get install -y libavformat-dev libavcodec-dev libswscale-dev \
                libswresample-dev libavutil-dev libwebp-dev pkg-config
        elif command -v dnf &>/dev/null; then
            sudo dnf install -y ffmpeg-devel libwebp-devel pkg-config
        elif command -v pacman &>/dev/null; then
            sudo pacman -S --noconfirm ffmpeg libwebp pkg-config
        else
            echo "Unsupported package manager. Install these packages manually:"
            echo "  libavformat-dev libavcodec-dev libswscale-dev"
            echo "  libswresample-dev libavutil-dev libwebp-dev"
            exit 1
        fi
        echo ""
        echo "Verifying installation..."
        pkg-config --libs libavformat libavcodec libswscale libswresample libavutil
        echo "FFmpeg development libraries installed successfully."
        ;;
    Darwin*)
        echo "macOS uses AVFoundation natively — no FFmpeg needed for macOS."
        echo "If you need WebP encoding, install libwebp:"
        echo "  brew install webp"
        ;;
    MINGW*|MSYS*|CYGWIN*)
        echo "On Windows, install FFmpeg and set FFMPEG_ROOT:"
        echo ""
        echo "Option 1 - vcpkg:"
        echo "  vcpkg install ffmpeg:x64-windows"
        echo "  set FFMPEG_ROOT=C:/vcpkg/installed/x64-windows"
        echo ""
        echo "Option 2 - Pre-built:"
        echo "  1. Download from https://github.com/BtbN/FFmpeg-Builds/releases"
        echo "  2. Extract to C:\\ffmpeg"
        echo "  3. set FFMPEG_ROOT=C:\\ffmpeg"
        echo ""
        echo "Option 3 - winget:"
        echo "  winget install ffmpeg"
        ;;
    *)
        echo "Unknown platform: $(uname -s)"
        exit 1
        ;;
esac

echo ""
echo "=== Done ==="
