SRC_DIR="$(dirname "$0")"
BUILD_DIR="$SRC_DIR/build"

echo "Source dir: $SRC_DIR"
echo "Build dir:  $BUILD_DIR"

sudo -u"$USER" mkdir -p "$BUILD_DIR"

echo "Generating..."
cmake -S "$SRC_DIR" -B "$BUILD_DIR"

echo "Building..."
cmake --build "$BUILD_DIR"

echo "Running installer..."
exec "$SRC_DIR/installer.sh"