mkdir build
cmake -S "$PWD" -B "$PWD"/build
cmake --build "$PWD"/build
cmake --install "$PWD"/build