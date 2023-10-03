PROD_FLAGS="-O2 -s"
DEV_FLAGS="-g -ggdb"
CFLAGS="-Wall -Wextra -Wimplicit -Wpedantic -Wno-unused-function -std=c99"

# Compiler-Options to include dependencies
VCPKG_DEP="\"-I/Program Files/vcpkg/installed/x64-windows/include\" \"-L/Program Files/vcpkg/installed/x64-windows/bin\" \"-L/Program Files/vcpkg/installed/x64-windows/lib\""
RAYLIB_DEP="-lraylib -lopengl32 -lgdi32 -lwinmm"
HPDF_DEP="-lhpdf -lzlib -llibpng16"
DEP="-I./deps/stb -I./deps/tsoding -I./deps/QuelSolaar $VCPKG_DEP $RAYLIB_DEP $HPDF_DEP"

if ![ -d "./bin/assets" ]; then
	cp -r "./assets" "./bin/assets"
fi
if [ -f "./bin/rl" ]; then
	rm -f "./bin/rl"
fi

gcc $CFLAGS $DEV_FLAGS -o bin/rl src/main.c $DEP