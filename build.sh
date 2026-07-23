#!/bin/bash
# Build the Laughlin C++ module and copy the .so to the project root.
# Usage:  ./build.sh [Release|RelWithDebInfo|Debug]

set -e

BUILD_TYPE="${1:-Release}"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
CPP_DIR="$SCRIPT_DIR/cpp"
BUILD_DIR="$CPP_DIR/build"
VENV_PYTHON="$SCRIPT_DIR/.venv/bin/python"

# ----- cmake & make -----
echo "=== Configuring ($BUILD_TYPE) ==="
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

cd "$BUILD_DIR"
cmake "$CPP_DIR" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DPYTHON_EXECUTABLE="$VENV_PYTHON" \
    -DPYTHON_INCLUDE_DIR="$("$VENV_PYTHON" -c 'import sysconfig; print(sysconfig.get_path("include"))')" \
    -DPYTHON_LIBRARY="$("$VENV_PYTHON" -c 'import sysconfig; print(sysconfig.get_config_var("LIBDIR"))')"

echo "=== Building ==="
make -j"$(nproc)"

# ----- copy .so to project root -----
SO_FILE=$(find . -maxdepth 1 -name 'Laughlin.cpython-*.so' -print -quit)
if [ -z "$SO_FILE" ]; then
    echo "ERROR: could not find built .so file"
    exit 1
fi

cp "$SO_FILE" "$SCRIPT_DIR/"
echo "=== Copied $SO_FILE → $SCRIPT_DIR/ ==="
echo "=== Done ==="
