
# Raycaster Plus Game Engine

## Description
Project is still dynamically expanding, therefore there is no detailed description at this moment.

## Building
For now your playground is limited to one file (main.cpp), your fantastic game can be build using\
make like shown below:\
``` $ make zlib="path/to/zlib" libpng="path/to/libpng" sdl="path/to/sdl" ```
obviously you need to provide correct paths to dependencies build directories, it was tested on\
linux machine already, unsure if it works on Windows too.

## Dependencies
It requires some exact versions, because I have not wanted to check if new ones preserve structure,\
names etc.
- [SDL](https://github.com/libsdl-org/SDL/releases/tag/release-2.28.5) exactly 2.28.5
- [zlib](https://www.zlib.net/) at least 1.2.13
- [libpng](http://www.libpng.org/pub/png/libpng.html) exactly 1.6.40
