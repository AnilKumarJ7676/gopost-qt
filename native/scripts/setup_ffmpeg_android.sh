#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
NATIVE_DIR="$(dirname "$SCRIPT_DIR")"
THIRD_PARTY="${NATIVE_DIR}/third_party/ffmpeg-android"
FFMPEG_VERSION="${1:-6.0}"
ABIS=("arm64-v8a" "armeabi-v7a" "x86_64")

echo "=== GoPost FFmpeg Android NDK Setup ==="
echo "Target: ${THIRD_PARTY}"
echo "ABIs:   ${ABIS[*]}"

if [ -d "${THIRD_PARTY}/include/libavformat" ]; then
    echo "FFmpeg headers already present. Delete ${THIRD_PARTY} to re-download."
    exit 0
fi

mkdir -p "${THIRD_PARTY}"

TEMP_DIR=$(mktemp -d)
trap 'rm -rf "${TEMP_DIR}"' EXIT

echo ""
echo "Downloading pre-built FFmpeg ${FFMPEG_VERSION} for Android..."
BASE_URL="https://github.com/ArtifexSoftware/ffmpeg-android/releases/download/v${FFMPEG_VERSION}"

for ABI in "${ABIS[@]}"; do
    ARCHIVE="ffmpeg-${FFMPEG_VERSION}-${ABI}.tar.gz"
    URL="${BASE_URL}/${ARCHIVE}"
    echo "  Fetching ${ABI}..."
    if curl -fSL --retry 3 -o "${TEMP_DIR}/${ARCHIVE}" "${URL}" 2>/dev/null; then
        tar -xzf "${TEMP_DIR}/${ARCHIVE}" -C "${TEMP_DIR}"
        mkdir -p "${THIRD_PARTY}/lib/${ABI}"
        cp "${TEMP_DIR}"/ffmpeg-*/lib/${ABI}/*.so "${THIRD_PARTY}/lib/${ABI}/" 2>/dev/null || \
        cp "${TEMP_DIR}"/*/lib/*.so "${THIRD_PARTY}/lib/${ABI}/" 2>/dev/null || true
    else
        echo "  WARNING: Could not download ${ARCHIVE}"
        echo "  You can manually place FFmpeg .so files in ${THIRD_PARTY}/lib/${ABI}/"
    fi
done

if [ ! -d "${THIRD_PARTY}/include" ]; then
    echo ""
    echo "Extracting headers..."
    HEADER_DIR=$(find "${TEMP_DIR}" -name "avformat.h" -path "*/libavformat/*" -print -quit 2>/dev/null | xargs dirname 2>/dev/null | xargs dirname 2>/dev/null)
    if [ -n "${HEADER_DIR}" ]; then
        cp -R "${HEADER_DIR}" "${THIRD_PARTY}/include"
    fi
fi

echo ""
echo "=== Setup complete ==="
echo ""
echo "Add to gopost_app/android/gradle.properties:"
echo "  ffmpeg.dir=${THIRD_PARTY}"
echo ""
echo "Or build with:"
echo "  flutter build apk --release"
