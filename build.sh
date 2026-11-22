#!/bin/bash
# Builds build/witchaven.xex in WSL Debian, with xsdk at ~/xsdk and SDL-xenon built at ~/SDL-xenon/build-xsdk
# (README.md). Run from this folder: ./build.sh
#
# Other places for them: XSDK=/path/to/xsdk SDL3_DIR=/path/to/SDL-xenon/build-xsdk ./build.sh
set -e
cd "$(dirname "$0")"

XSDK="${XSDK:-$HOME/xsdk}"
SDL3_DIR="${SDL3_DIR:-$HOME/SDL-xenon/build-xsdk}"

if [ ! -f "$XSDK/cmake/xsdk.cmake" ]; then
	echo "xsdk isn't at $XSDK (no cmake/xsdk.cmake there). Run with XSDK=/path/to/xsdk ./build.sh"
	exit 1
fi
if [ ! -d "$SDL3_DIR" ]; then
	echo "SDL-xenon's build isn't at $SDL3_DIR. Run with SDL3_DIR=/path/to/SDL-xenon/build-xsdk ./build.sh"
	exit 1
fi

cmake -S . -B build -G Ninja \
	-DCMAKE_TOOLCHAIN_FILE="$XSDK/cmake/xsdk.cmake" \
	-DSDL3_DIR="$SDL3_DIR" \
	-DCMAKE_BUILD_TYPE=Release
cmake --build build
ls -l build/witchaven.xex
