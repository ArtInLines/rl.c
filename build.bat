@echo off

:: -mwindows : Compile a Windows executable, no cmd window
set PROD-FLAGS=-O2 -mwindows -s
set DEV-FLAGS=-g -ggdb
set CFLAGS=-Wall -Wextra -Wimplicit -Wpedantic -Wno-unused-function -std=c99

:: Compiler-Options to include dependencies
set VCPKG_DEP="-I/Program Files/vcpkg/installed/x64-windows/include" "-L/Program Files/vcpkg/installed/x64-windows/bin" "-L/Program Files/vcpkg/installed/x64-windows/lib"
set RAYLIB_DEP=-lraylib -lopengl32 -lgdi32 -lwinmm
set HPDF_DEP=-lhpdf -lzlib -llibpng16

:: Build raygui
if "%~1"=="a" (
	echo "test"
	gcc -o bin/raygui.dll deps/raygui/src/raygui.c -shared -DRAYGUI_IMPLEMENTATION -DBUILD_LIBTYPE_SHARED -static-libgcc -Wl,--out-implib,bin/librayguidll.a %VCPKG_DEP% %RAYLIB_DEP%
)
set RAYGUI_DEP=%-lraygui%

set DEP=-I./deps/stb -I./deps/tsoding -I./deps/QuelSolaar -I./deps/raygui/src %VCPKG_DEP% %RAYLIB_DEP% %RAYGUI_DEP% %HPDF_DEP%

:: Build rl.c
cmd /c if not exist bin\assets\ xcopy assets\ bin\assets\ /E
cmd /c if exist bin\rl.exe del /F bin\rl.exe
gcc %CFLAGS% %DEV-FLAGS% -o bin/rl src/main.c %DEP%