
#ifndef _RP_TEXTURE_HPP
#define _RP_TEXTURE_HPP

#ifdef DEBUG
#include <iostream>
#endif
#include <string>
#include <png.h>
#include "globals.hpp"

namespace rp {
    #ifdef DEBUG
    using ::std::ostream;
    using ::std::cout;
    using ::std::endl;
    #endif
    using ::std::string;

    /**
     * Lets you load a PNG image file and work with its pixel data in RGBA color space.
     * It is a good practice to check for errors after every interaction with the class
     * object, use `getError` method to get the latest error code.
     * 
     * Stored pixels are encoded to single number using `encodeRGBA` function, if you want
     * to access individual channels data you have to decode a number using `decodeRGBA` function.
     */
    class Texture {
        private:
            mutable int error;
            int width;
            int height;
            uint32_t* pixels;
        public:
            enum {
                E_CLEAR,
                E_CANNOT_OPEN_FILE,
                E_FAILED_READING_FILE,
                E_INVALID_POSITION
            };

            Texture();
            Texture(const string& pngFile);
            ~Texture();
            int getWidth() const;
            int getHeight() const;
            int getError() const;
            void loadFromFile(const string& pngFile);
            uint32_t getPosition(int x, int y) const;
            uint32_t getCoords(float u, float v) const;
    };
    #ifdef DEBUG
    ostream& operator<<(ostream& stream, const Texture& tex);
    #endif
}

#endif
