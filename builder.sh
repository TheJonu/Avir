SRC_DIR="$(dirname "$0")"
BUILD_DIR="$SRC_DIR/build"

sudo -u"$USER" mkdir -p "$BUILD_DIR"

cmake -S "$SRC_DIR" -B "$BUILD_DIR"

cmake --build "$BUILD_DIR"
