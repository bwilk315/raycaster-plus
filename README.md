
# Raycaster Plus Game Engine

## Description
Project is still dynamically expanding, therefore there is no detailed description at this moment.

## Building
To build RPGE library, use make tool like shown below:
``` $ make out=absolute/out/path libz=... libpng=... libsdl=... ```
Obviously three dots have to be replaced with path leading to a library build (with `include` and `lib` catalogs). Successfull build leaves two catalogs: `include` and `lib` (like a normal library).

## Linking
There you have a template, assuming your main source file is in the same directory:
``` $ g++ main.cpp -I<a> -L<b> -lrpge -o main.exe ```
Replace `a` and `b` tags with path to `include` and `lib` catalogs of the build directory respectively.

## Dependencies
Engine requires some exact versions, because I have not wanted to check if new ones preserve structure, names etc.
- [SDL](https://github.com/libsdl-org/SDL/releases/tag/release-2.28.5) exactly 2.28.5
- [zlib](https://www.zlib.net/) at least 1.2.13
- [libpng](http://www.libpng.org/pub/png/libpng.html) exactly 1.6.40
