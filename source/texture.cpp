
#include "../include/texture.hpp"

namespace rp {

    /************************************/
    /********** CLASS: TEXTURE **********/
    /************************************/

    Texture::Texture() {
        this->error = E_CLEAR;
        this->width = 0;
        this->height = 0;
        this->pixels = nullptr;
    }
    Texture::Texture(const string& pngFile) {
        loadFromFile(pngFile);
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
    int Texture::getError() const {
        return error;
    }
    void Texture::loadFromFile(const string& pngFile) {
        error = E_CLEAR;
        png_image image = {};
        image.version = PNG_IMAGE_VERSION;
        // Try accessing the specified file
        if(png_image_begin_read_from_file(&image, pngFile.c_str())) {
            image.format = PNG_FORMAT_RGBA;
            size_t size = PNG_IMAGE_SIZE(image);
            png_byte* bytes = new png_byte[size];
            // Now try to read RGBA pixels
            if(bytes != nullptr && png_image_finish_read(&image, NULL, bytes, 0, NULL)) {
                this->width = image.width;
                this->height = image.height;
                if(pixels != nullptr)
                    delete[] pixels;
                // Store RGBA pixels as single numbers
                this->pixels = new uint32_t[size / 4];
                for(int i = 0; i < size; i += 4) {
                    // As in the scene loading, increase each of RGB channels to prevent it from being
                    // near pitch black color, which is used for pixel-detection by the engine.
                    uint8_t r, g, b, a;
                    this->pixels[i / 4] = encodeRGBA(
                        bytes[i + 0],
                        bytes[i + 1],
                        bytes[i + 2],
                        bytes[i + 3]
                    );
                }
            } else {
                error = E_FAILED_READING_FILE;
            }
            delete[] bytes;
        } else {
            error = E_INVALID_FILE;
        }
        // If no error occurred structure captured pixel data, now it has to be freed
        if(!error)
            png_image_free(&image);
    }
    uint32_t Texture::getPosition(int x, int y) const {
        error = E_CLEAR;
        if(pixels == nullptr || x < 0 || x > width - 1 || y < 0 || y > height - 1) {
            error = E_INVALID_POSITION;
            return 0x00000000;
        }
        uint32_t row = (height - 1) - y;
        return pixels[x + row * width];
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
