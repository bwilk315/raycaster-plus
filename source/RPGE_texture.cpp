
#include <RPGE_texture.hpp>

namespace rpge {

    /************************************/
    /********** CLASS: TEXTURE **********/
    /************************************/

    Texture::Texture(const SDL_PixelFormat* format) {
        this->loaded = false;
        this->error  = E_CLEAR;
        this->width  = 0;
        this->height = 0;
        this->pixels = nullptr;
        this->format = format;
    }
    Texture::Texture(const SDL_PixelFormat* format, const string& pngFile) : Texture(format) {
        loadFromFile(pngFile);
    }
    Texture::~Texture() {
        if(pixels != nullptr)
            delete[] pixels;
    }
    int Texture::getWidth() const noexcept {
        return width;
    }
    bool Texture::isLoaded() const noexcept {
        return loaded;
    }
    int Texture::getHeight() const noexcept {
        return height;
    }
    int Texture::getError() const noexcept {
        return error;
    }
    void Texture::loadFromFile(const string& pngFile) {
        error = E_CLEAR;
        png_image image = {};
        image.version = PNG_IMAGE_VERSION;

        if(png_image_begin_read_from_file(&image, pngFile.c_str())) {
            image.format = PNG_FORMAT_RGBA;
            int size  = PNG_IMAGE_SIZE(image);
            png_byte* bytes = new png_byte[size];
            if(bytes != nullptr && png_image_finish_read(&image, NULL, bytes, 0, NULL)) {
                this->width  = image.width;
                this->height = image.height;
                if(pixels != nullptr)
                    delete[] pixels;
                this->pixels = new uint32_t[size / 4];
                for(int i = 0; i < size; i += 4) {
                    this->pixels[i / 4] = SDL_MapRGBA(
                        format,
                        bytes[i + 0],
                        bytes[i + 1],
                        bytes[i + 2],
                        bytes[i + 3]
                    );
                }
            }
            else {
                error = E_FILE_IRREADABLE;
            }
            delete[] bytes;
        }
        else {
            error = E_FILE_INACCESSIBLE;
        }

        if(!error) {
            png_image_free(&image);
            loaded = true;
        }
    }
    uint32_t Texture::getPosition(int x, int y) const {
        error = E_CLEAR;
        if(pixels == nullptr) {
            error = E_NOT_LOADED;
        }
        else if(x < 0 || x > width - 1 || y < 0 || y > height - 1) {
            error = E_INVALID_POSITION;
            return 0x00000000;
        }
        uint32_t row = (height - 1) - y;
        return pixels[x + row * width];
    }
    uint32_t Texture::getCoords(float u, float v) const noexcept {
        return getPosition((u - (int)u) * width, (v - (int)v) * height);
    }
    #ifdef DEBUG
    ostream& operator<<(ostream& stream, const Texture& tex) {
        stream << "Texture(width=" << tex.getWidth() << ", height=" << tex.getHeight() << ")";
        return stream;
    }
    #endif
}
