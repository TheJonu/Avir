[ "$UID" -eq 0 ] || exec sudo bash "$0" "$@"

SRC_DIR="$(dirname "$0")"
BUILD_DIR="$SRC_DIR/build"
INSTALL_DIR="/usr/local/bin"

echo "Source dir:   $SRC_DIR"
echo "Build dir:    $BUILD_DIR"
echo "Install dir:  $INSTALL_DIR"

cp "$BUILD_DIR/avir" "$INSTALL_DIR"