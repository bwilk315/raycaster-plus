
#include "../include/texture.hpp"

namespace rp {

    /*******************************************/
    /********** STRUCTURE: RGBA COLOR **********/
    /*******************************************/

    const Color Color::CLEAR = Color(0, 0, 0, 0);
    const Color Color::BLACK = Color(0, 0, 0);
    const Color Color::WHITE = Color(255, 255, 255);
    const Color Color::RED = Color(255, 0, 0);
    const Color Color::GREEN = Color(0, 255, 0);
    const Color Color::BLUE = Color(0, 0, 255);

    Color::Color() {
        this->red = 0;
        this->green = 0;
        this->blue = 0;
        this->alpha = 0;
    }
    Color::Color(uint8_t red, uint8_t green, uint8_t blue) {
        this->red = red;
        this->green = green;
        this->blue = blue;
        this->alpha = 255;
    }
    Color::Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha) : Color(red, green, blue) {
        this->alpha = alpha;
    }
    #ifdef DEBUG
    ostream& operator<<(ostream& stream, const Color& color) {
        stream << "Color(red=" << (int)color.red << ", green=" << (int)color.green << ", blue=" << (int)color.blue;
        stream << ", alpha=" << (int)color.alpha << ")";
        return stream;
    }
    #endif

    /*****************************************/
    /********** CLASS: RGBA TEXTURE **********/
    /*****************************************/

    const uint8_t Texture::CHANNELS = 4;

    Texture::Texture() {
        this->width = 0;
        this->height = 0;
        this->bytes = nullptr;
    }
    Texture::Texture(const string& file) {
        loadFromFile(file);
    }
    Texture::~Texture() {
        if(bytes != nullptr)
            delete[] bytes;
    }
    int Texture::getWidth() const {
        return width;
    }
    int Texture::getHeight() const {
        return height;
    }
    int Texture::loadFromFile(const string& file) {
        // Initialize image structure
        png_image image = {};
        image.version = PNG_IMAGE_VERSION;

        // Read the file contents, output them to the private buffer
        int error = Texture::E_CLEAR;
        if(png_image_begin_read_from_file(&image, file.c_str())) {
            image.format = PNG_FORMAT_RGBA;
            bytes = new png_byte[PNG_IMAGE_SIZE(image)];
            if(bytes != nullptr && png_image_finish_read(&image, NULL, this->bytes, 0, NULL)) {
                this->width = image.width;
                this->height = image.height;
            } else
                error = Texture::E_FAILED_READING_FILE;
                
        } else
            error = Texture::E_CANNOT_OPEN_FILE;

        // Free the temporary structure
        if(error == Texture::E_CLEAR)
            png_image_free(&image);
        return error;
    }
    Color Texture::getPosition(int x, int y) const {
        if(bytes == nullptr || x < 0 || x > width - 1 || y < 0 || y > height - 1)
            return Color::CLEAR;
        // Read next four bytes of appropriate location in the raw data array
        uint32_t r = (height - 1) - y;
        png_bytep start = &bytes[Texture::CHANNELS * (x + r * width)];
        return Color(start[0], start[1], start[2], start[3]);
    }
    Color Texture::getCoords(float u, float v) const {
        return getPosition((u - (int)u) * width, (v - (int)v) * height);
    }
    #ifdef DEBUG
    ostream& operator<<(ostream& stream, const Texture& tex) {
        stream << "Texture(width=" << tex.getWidth() << ", height=" << tex.getHeight() << ")";
        return stream;
    }
    #endif
}
