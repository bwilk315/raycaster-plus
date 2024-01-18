
#
# User is obligated to provide paths to dependencies build directories, see example below:
# $ make out="output/path" libz="path/to/zlib" libpng="path/to/libpng" libsdl="path/to/sdl"
#
# This makefile works very primitively, I mean that it compiles the sources to object files,
# and then merges them into one library with dependency libraries' object files, similar situation
# occurrs to header files, oh and it builds it in DEBUG mode - it is going to change later on.
#

OUT    = $(out)
LIBZ   = -I$(libz)/include -L$(libz)/lib -lz
LIBPNG = -I$(libpng)/include -L$(libpng)/lib -lpng
LIBSDL = -I$(libsdl)/include -L$(libsdl)/lib -lSDL2
EXTS   = -DDEBUG -I$(shell pwd)/include $(LIBZ) $(LIBPNG) $(LIBSDL)

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
	@echo Compiling camera ...
	g++ -c source/RPGE_camera.cpp $(EXTS) -o $(OUT)/RPGE_camera.o

dda: globals math scene
	@echo Compiling dda ...
	g++ -c source/RPGE_dda.cpp $(EXTS) -o $(OUT)/RPGE_dda.o

engine: camera dda globals math scene texture
	@echo Compiling engine ...
	g++ -c source/RPGE_engine.cpp $(EXTS) -o $(OUT)/RPGE_engine.o

globals:
	@echo Compiling globals ...
	g++ -c source/RPGE_globals.cpp $(EXTS) -o $(OUT)/RPGE_globals.o

math: globals
	@echo Compiling math ...
	g++ -c source/RPGE_math.cpp $(EXTS) -o $(OUT)/RPGE_math.o

scene: globals math texture
	@echo Compiling scene ...
	g++ -c source/RPGE_scene.cpp $(EXTS) -o $(OUT)/RPGE_scene.o

texture: globals
	@echo Compiling texture ...
	g++ -c source/RPGE_texture.cpp $(EXTS) -o $(OUT)/RPGE_texture.o

clean:
	# Output directory gets cleaned if it is not empty and does not point the root directory
ifneq ("$(OUT)", "")
ifneq ("$(ls $(OUT))", "")
	rm -r $(OUT)/*
endif
endif
