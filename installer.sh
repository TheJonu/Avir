[ "$UID" -eq 0 ] || exec sudo bash "$0" "$@"

SRC_DIR="$(dirname "$0")"
BUILD_DIR="$SRC_DIR/build"

echo "Source dir: $SRC_DIR"
echo "Build dir:  $BUILD_DIR"

#echo "Running builder..."
#"$SRC_DIR/builder.sh"

#echo "Running installer..."
#exec "$SRC_DIR/installer.sh"

sudo -u"$USER" mkdir -p "$BUILD_DIR"

cmake -S "$SRC_DIR" -B "$BUILD_DIR"
cmake --build "$BUILD_DIR" --target install