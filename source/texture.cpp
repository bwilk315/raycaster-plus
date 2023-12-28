
#include "../include/texture.hpp"

namespace rp {
    Texture::Texture() {
        this->width = -1;
        this->height = -1;
    }
    Texture::Texture(const string& file) {
        loadFromFile(file);
    }
    Texture::~Texture() {
        if(rawData != nullptr)
            delete[] rawData;
    }
    int Texture::getWidth() const {
        return width;
    }
    int Texture::getHeight() const {
        return height;
    }
    int Texture::loadFromFile(const string& file) {
        // Initialize image structure
        png_image image;
        memset(&image, 0, sizeof(png_image));
        image.version = PNG_IMAGE_VERSION;

        // Read the file contents, output them to the private buffer
        int error = Texture::E_CLEAR;

        if(png_image_begin_read_from_file(&image, file.c_str())) {
            image.format = PNG_FORMAT_RGBA;
            this->rawData = new png_byte[PNG_IMAGE_SIZE(image)];
            if(this->rawData != nullptr && png_image_finish_read(&image, NULL, this->rawData, 0, NULL)) {
                this->width = image.width;
                this->height = image.height;
            } else
                error = Texture::E_FAILED_READING_FILE;
                
        } else
            error = Texture::E_CANNOT_OPEN_FILE;

        // Free the temporary structure
        if(error == Texture::E_CLEAR) {
            png_image_free(&image);
        }
        return error;
    }
    SDL_Color Texture::getPixelAt(int x, int y) const {
        if(rawData == nullptr || x < 0 || x > width - 1 || y < 0 || y > height - 1)
            return { 0, 0, 0, 0 };
        // Read next four bytes of appropriate location in the raw data array
        int r = (height - 1) - y;
        png_bytep start = &rawData[Texture::COLOR_COMPS * (x + r * width)];
        return { start[0], start[1], start[2], start[3] };
    }
    SDL_Color Texture::getPixelNorm(float x, float y) const {
        return getPixelAt((x - (int)x) * width, (y - (int)y) * height);
    }
}
