@echo off

:: -mwindows : Compile a Windows executable, no cmd window
set PROD-FLAGS=-O2 -mwindows -s
set DEV-FLAGS=-g -ggdb
set CFLAGS=-Wall -Wextra -Wimplicit -Wpedantic -Wno-unused-function -std=c99

:: Compiler-Options to include dependencies
set VCPKG_DEP="-I/Program Files/vcpkg/installed/x64-windows/include" "-L/Program Files/vcpkg/installed/x64-windows/bin" "-L/Program Files/vcpkg/installed/x64-windows/lib"
set RAYLIB_DEP=-lraylib -lopengl32 -lgdi32 -lwinmm
set HPDF_DEP=-lhpdf -lzlib -llibpng16
set DEP=%VCPKG_DEP% %RAYLIB_DEP% %HPDF_DEP%

:: @TODO Let user decide which example to build
:: (including option to build all examples)
cmd /c if exist rl.exe del /F rl.exe
gcc %CFLAGS% %DEV-FLAGS% -o bin/rl src/main.c %DEP%