[ "$UID" -eq 0 ] || exec sudo bash "$0" "$@"

SRC_DIR="$(dirname "$0")"
BUILD_DIR="$SRC_DIR/build"

echo "Installing..."
cmake --install "$BUILD_DIR"