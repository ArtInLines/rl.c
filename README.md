# Reading-List

Everything about this project is still a work-in-progress (including the name).

## Prerequisites

To compile the program, you need `gcc` and `make` installed and in your path. On Windows, you can install [make for windows](https://gnuwin32.sourceforge.net/packages/make.htm).

## Quick-Start

Run `build.bat` / `build.sh` to build the program. If you didn't build the project before, it will build dynamic libraries for the dependencies (included in the `deps/` directory) as well. You can force this behavior by supplying `a` as the first argument too.

Run `run.bat` / `run.sh` to run the program. The script will automatically build the project before running, so there is no need to ever run `build.bat` / `build.sh` yourself.
