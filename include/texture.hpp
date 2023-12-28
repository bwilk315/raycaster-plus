
#ifndef _TEXTURE_HPP
#define _TEXTURE_HPP

#include <iostream> // Ekhem
#include <string>
#include <png.h>
#include <SDL2/SDL_pixels.h>

namespace rp {
    using ::std::string;

    class Texture {
        private:
            int width;
            int height;
            png_bytep rawData = nullptr;
        public:
            static const int COLOR_COMPS = 4; // Number of color components used
            enum {
                E_CLEAR,
                E_CANNOT_OPEN_FILE,
                E_FAILED_READING_FILE
            };

            Texture();
            Texture(const string& file);
            ~Texture();
            int getWidth() const;
            int getHeight() const;
            int loadFromFile(const string& file);
            SDL_Color getPixelAt(int x, int y) const;
            SDL_Color getPixelNorm(float x, float y) const;
    };
}

#endif
