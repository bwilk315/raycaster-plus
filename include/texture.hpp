
#ifndef _RP_TEXTURE_HPP
#define _RP_TEXTURE_HPP

#ifdef DEBUG
#include <iostream>
#endif

#include <string>
#include <png.h>

namespace rp {
    using ::std::string;
    using ::std::ostream;

    struct Color {
        static const Color CLEAR;
        static const Color BLACK;
        static const Color WHITE;
        static const Color RED;
        static const Color GREEN;
        static const Color BLUE;

        uint8_t red;
        uint8_t green;
        uint8_t blue;
        uint8_t alpha;

        Color();
        Color(uint8_t red, uint8_t green, uint8_t blue);
        Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha);
    };
    #ifdef DEBUG
    ostream& operator<<(ostream& stream, const Color& color);
    #endif

    class Texture {
        private:
            int width;
            int height;
            png_bytep bytes;
        public:
            static const uint8_t CHANNELS;
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
            Color getPosition(int x, int y) const;
            Color getCoords(float u, float v) const;
    };
    #ifdef DEBUG
    ostream& operator<<(ostream& stream, const Texture& tex);
    #endif
}

#endif
