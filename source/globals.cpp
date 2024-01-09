
#include "../include/globals.hpp"

namespace rp {
    void decodeRGBA(uint32_t n, uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) {
        r = (n % 0x100000000 - n % 0x001000000) / 0x001000000;
        g = (n % 0x001000000 - n % 0x000010000) / 0x000010000;
        b = (n % 0x000010000 - n % 0x000000100) / 0x000000100;
        a = n % 0x000000100;
    }
    bool isFloat(const string& str) {
        istringstream iss(str);
        float _;
        iss >> noskipws >> _;
        return iss.eof() && !iss.fail();
    }
    int clamp(int value, int min, int max) {
        return value <= min ? min : (value >= max ? max : value);
    }
    float clamp(float value, float min, float max) {
        return value <= min ? min : (value >= max ? max : value);
    }
    uint32_t encodeRGBA(const uint8_t& r, const uint8_t& g, const uint8_t& b, const uint8_t& a) {
        return 0x01000000 * (16 * (r / 16) + r % 16) +
               0x00010000 * (16 * (g / 16) + g % 16) +
               0x00000100 * (16 * (b / 16) + b % 16) +
               0x00000001 * (16 * (a / 16) + a % 16);
    }
}
