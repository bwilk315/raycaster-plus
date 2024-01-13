
#
# User is obligated to provide paths to dependencies build directories, see example below:
# $ make zlib="path/to/zlib" libpng="path/to/libpng" sdl="path/to/sdl"
#

INCLUDE_DIRS = -I"$(zlib)/include" -I"$(libpng)/include" -I"$(sdl)/include"
LIBRARY_DIRS = -L"$(zlib)/lib" -L"$(libpng)/lib" -L"$(sdl)/lib"
LIBRARY_LINKS = -lz -lpng -lSDL2

# For windows two additional links needs to be included (not tested yet)
ifeq ( $(OS), Windows_NT )
	LIBRARY_LINKS +=  -lmingw -lSDL2main
endif

build: main.cpp
	@echo "Building ..."
	g++ main.cpp source/* $(INCLUDE_DIRS) $(LIBRARY_DIRS) $(LIBRARY_LINKS) -o build.exe
	@echo "Success!"

