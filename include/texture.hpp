
#ifndef _RP_TEXTURE_HPP
#define _RP_TEXTURE_HPP

#ifdef DEBUG
#include <iostream>
#endif

#include <string>
#include <png.h>

namespace rp {
    using ::std::string;
    #ifdef DEBUG
    using ::std::ostream;
    #endif

    class Texture {
        private:
            int width;
            int height;
            uint32_t* pixels; // RGBAs compressed to single numbers
        public:
            enum {
                E_CLEAR,
                E_CANNOT_OPEN_FILE,
                E_FAILED_READING_FILE
            };

            // Do not use generated number to set surface pixels, use appropriate provided by SDL instead
            static uint32_t getColorAsNumber(const uint8_t& r, const uint8_t& g, const uint8_t& b, const uint8_t& a);
            static void getNumberAsColor(uint32_t n, uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a);

            Texture();
            Texture(const string& file);
            ~Texture();
            int getWidth() const;
            int getHeight() const;
            int loadFromFile(const string& file);
            uint32_t getPosition(int x, int y) const;
            uint32_t getCoords(float u, float v) const;
    };
    #ifdef DEBUG
    ostream& operator<<(ostream& stream, const Texture& tex);
    #endif
}

#endif
