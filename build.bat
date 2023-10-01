@echo off

:: -mwindows : Compile a Windows executable, no cmd window
set PROD-FLAGS=-O2 -mwindows -s
set DEV-FLAGS=-g -ggdb
set CFLAGS=-Wall -Wextra -Wimplicit -Wpedantic -Wno-unused-function -std=c99

:: Compiler-Options to include dependencies
set VCPKG_DEP="-I/Program Files/vcpkg/installed/x64-windows/include" "-L/Program Files/vcpkg/installed/x64-windows/bin" "-L/Program Files/vcpkg/installed/x64-windows/lib"
set RAYLIB_DEP=-lraylib -lopengl32 -lgdi32 -lwinmm
set HPDF_DEP=-lhpdf -lzlib -llibpng16
set DEP=-I./deps/stb -I./deps/tsoding -I./deps/QuelSolaar %VCPKG_DEP% %RAYLIB_DEP% %HPDF_DEP%

cmd /c if not exist bin\assets\ xcopy assets\ bin\assets\ /E
cmd /c if exist bin\rl.exe del /F bin\rl.exe
gcc %CFLAGS% %DEV-FLAGS% -o bin/rl src/main.c %DEP%