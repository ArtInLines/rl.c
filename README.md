# Reading-List

Everything about this project is still a work-in-progress (including the name).

## Build

As I make this project only for myself for now and I'm working on a windows, the following instructions will only work for windows.

Download additional dependencies via `vcpkg`:

```cmd
> vcpkg install raylib
> vcpkg install libharu
```

Copy the required dlls into `bin/`:

```cmd
> mkdir bin
> copy <path_to_vcpkg>\installed\x64_windows\bin bin
```

Build the project:

```cmd
> build.bat
```

Run the project:

```cmd
> bin\rl.exe
```
