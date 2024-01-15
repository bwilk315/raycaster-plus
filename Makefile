
#
# User is obligated to provide paths to dependencies build directories, see example below:
# $ make out="output/path" libz="path/to/zlib" libpng="path/to/libpng" libsdl="path/to/sdl"
#
# This makefile works very primitively, I mean that it compiles the sources to object files,
# and then merges them into one library with dependency libraries' object files, similar situation
# occurrs to header files.
#

OUT    = $(out)
LIBZ   = -I$(libz)/include -L$(libz)/lib -lz
LIBPNG = -I$(libpng)/include -L$(libpng)/lib -lpng
LIBSDL = -I$(libsdl)/include -L$(libsdl)/lib -lSDL2
EXTS   = $(LIBZ) $(LIBPNG) $(LIBSDL)

all: clean camera dda engine globals math scene texture
	@echo Creating library archive file ...

	mkdir -p $(OUT)/lib $(OUT)/include

	# Create library archive, initially with engine object files included
	ar rcs -o $(OUT)/lib/librpge.a $(OUT)/*.o
	rm $(OUT)/*.o

	# Include zlib object files in the archive
	cp $(libz)/lib/libz.a $(OUT)
	ar x $(OUT)/libz.a --output $(OUT)
	ar r $(OUT)/lib/librpge.a $(OUT)/*.o
	rm $(OUT)/libz.a $(OUT)/*.o

	# Include libpng object files in the archive
	cp $(libpng)/lib/libpng.a $(OUT)
	ar x $(OUT)/libpng.a --output $(OUT)
	ar r $(OUT)/lib/librpge.a $(OUT)/*.o
	rm $(OUT)/libpng.a $(OUT)/*.o

	# Include libSDL2 and libSDL2main object files in the archive
	cp $(libsdl)/lib/libSDL2.a $(OUT)
	cp $(libsdl)/lib/libSDL2main.a $(OUT)
	ar x $(OUT)/libSDL2.a --output $(OUT)
	ar x $(OUT)/libSDL2main.a --output $(OUT)
	ar r $(OUT)/lib/librpge.a $(OUT)/*.o
	rm $(OUT)/libSDL2.a $(OUT)/libSDL2main.a $(OUT)/*.o

	# Fill include folder with engine's and every other libs' header files
	cp include/* $(OUT)/include
	cp -r $(libz)/include/* $(OUT)/include
	cp -r $(libpng)/include/* $(OUT)/include
	cp -r $(libsdl)/include/* $(OUT)/include

	@echo Library successfuly built

camera: globals math
	@echo Compiling camera.cpp ...
	g++ -c source/camera.cpp $(EXTS) -o $(OUT)/camera.o

dda: globals math scene
	@echo Compiling dda.cpp ...
	g++ -c source/dda.cpp $(EXTS) -o $(OUT)/dda.o

engine: camera dda globals math scene texture
	@echo Compiling engine.cpp ...
	g++ -c source/engine.cpp $(EXTS) -o $(OUT)/engine.o

globals:
	@echo Compiling globals.cpp ...
	g++ -c source/globals.cpp $(EXTS) -o $(OUT)/globals.o

math: globals
	@echo Compiling math.cpp ...
	g++ -c source/math.cpp $(EXTS) -o $(OUT)/math.o

scene: globals math texture
	@echo Compiling scene.cpp ...
	g++ -c source/scene.cpp $(EXTS) -o $(OUT)/scene.o

texture: globals
	@echo Compiling texture.cpp ...
	g++ -c source/texture.cpp $(EXTS) -o $(OUT)/texture.o

clean:
	# Output directory gets cleaned if it is not empty and does not point the root directory
ifneq ("$(OUT)", "")
ifneq ("$(ls $(OUT))", "")
	rm -r $(OUT)/*
endif
endif
