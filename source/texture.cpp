
#include "../include/texture.hpp"

namespace rp {

    /*****************************************/
    /********** CLASS: RGBA TEXTURE **********/
    /*****************************************/

    uint32_t Texture::getColorAsNumber(const uint8_t& r, const uint8_t& g, const uint8_t& b, const uint8_t& a) {
        // Do not simplify this, it contains integer division which results in floored result!
        return 0x01000000 * (16 * (r / 16) + r % 16) +
               0x00010000 * (16 * (g / 16) + g % 16) +
               0x00000100 * (16 * (b / 16) + b % 16) +
               0x00000001 * (16 * (a / 16) + a % 16);
    }
    void Texture::getNumberAsColor(uint32_t n, uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) {
        r = (n % 0x100000000 - n % 0x001000000) / 0x001000000;
        g = (n % 0x001000000 - n % 0x000010000) / 0x000010000;
        b = (n % 0x000010000 - n % 0x000000100) / 0x000000100;
        a = n % 0x000000100;
    }
    Texture::Texture() {
        this->width = 0;
        this->height = 0;
        this->pixels = nullptr;
    }
    Texture::Texture(const string& file) {
        loadFromFile(file);
    }
    Texture::~Texture() {
        if(pixels != nullptr)
            delete[] pixels;
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

        // Read the file contents
        int error = Texture::E_CLEAR;
        if(png_image_begin_read_from_file(&image, file.c_str())) {
            image.format = PNG_FORMAT_RGBA;
            size_t size = PNG_IMAGE_SIZE(image);
            png_byte* bytes = new png_byte[size];
            this->pixels = new uint32_t[size / 4];
            if(bytes != nullptr && png_image_finish_read(&image, NULL, bytes, 0, NULL)) {
                this->width = image.width;
                this->height = image.height;
                // Store colors as numbers
                for(int i = 0; i < size; i += 4) {
                    this->pixels[i / 4] = getColorAsNumber(
                        bytes[i + 0],
                        bytes[i + 1],
                        bytes[i + 2],
                        bytes[i + 3]
                    );
                }
            } else
                error = Texture::E_FAILED_READING_FILE;
            delete[] bytes;
                
        } else
            error = Texture::E_CANNOT_OPEN_FILE;

        // Free the temporary structure
        if(error == Texture::E_CLEAR)
            png_image_free(&image);
        return error;
    }
    uint32_t Texture::getPosition(int x, int y) const {
        if(pixels == nullptr || x < 0 || x > width - 1 || y < 0 || y > height - 1)
            return 0x00000000;
        // Read next four bytes of appropriate location in the raw data array
        uint32_t r = (height - 1) - y;
        return pixels[x + r * width];
    }
    uint32_t Texture::getCoords(float u, float v) const {
        return getPosition((u - (int)u) * width, (v - (int)v) * height);
    }
    #ifdef DEBUG
    ostream& operator<<(ostream& stream, const Texture& tex) {
        stream << "Texture(width=" << tex.getWidth() << ", height=" << tex.getHeight() << ")";
        return stream;
    }
    #endif
}
