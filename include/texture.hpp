
#ifndef _RP_TEXTURE_HPP
#define _RP_TEXTURE_HPP

#include <string>
#include <png.h>
#include <SDL2/SDL_pixels.h>
#include "globals.hpp"

namespace rp {
    using ::std::string;

    /**
     * Lets you load a PNG image file and work with its pixel data in RGBA color space.
     * 
     * Stored pixel colors are encoded into single 32-bit positive number using `SDL_MapRGBA` function
     * and because of this, pointer to `SDL_PixelFormat` instance needs to be provided.
     * Obviously you can later retrieve separate color channels back using `SDL_GetRGBA` function.
     */
    class Texture {
        private:
            bool                   loaded;
            mutable int            error;
            int                    width;
            int                    height;
            uint32_t*              pixels;
            const SDL_PixelFormat* format;
        public:
            enum {
                E_CLEAR,
                E_FILE_INACCESSIBLE,
                E_FILE_IRREADABLE,
                E_NOT_LOADED,
                E_INVALID_POSITION
            };

            Texture(const SDL_PixelFormat* format);
            Texture(const SDL_PixelFormat* format, const string& pngFile);
            ~Texture();

            /* Returns color of the pixel at normalized coordinates `u` (horizontal) and `v` (vertical).
             * Coordinate (0, 0) is the bottom-left corner, while (1, 1) is the top-right corner.
             * Exceeding <0;1> limit range of values for `u` and `v` results in them cycling, for example
             * if you provide u=1.1 and v=-0.2, it gets translated into u=0.1 and v=0.8 */
            uint32_t getCoords(float u, float v) const noexcept;

            /* Returns the latest error code */
            int      getError() const noexcept;

            /* Returns the loaded texture height in pixels, or 0 if it is not loaded */
            int      getHeight() const noexcept;

            /* Returns color of the pixel at position (`x`, `y`).
             * Position (0, 0) is the bottom-left corner, while (`getWidth()` - 1, `getHeight()` - 1) is the
             * top-right corner. */
            uint32_t getPosition(int x, int y) const;
            
            /* Returns the loaded texture width in pixels, or 0 if it is not loaded */
            int      getWidth() const noexcept;

            /* Tells whether the texture successfuly loaded pixel data from a file at least once */
            bool     isLoaded() const noexcept;

            /* Tries to load pixel data from the specified PNG image file `pngFile` */
            void     loadFromFile(const string& pngFile);
    };
    #ifdef DEBUG
    ostream& operator<<(ostream& stream, const Texture& tex);
    #endif
}

#endif
