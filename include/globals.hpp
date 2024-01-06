
#ifndef _RP_GLOBALS_HPP
#define _RP_GLOBALS_HPP

#include <sstream>
#include <string>

namespace rp {
    using ::std::istringstream;
    using ::std::string;
    using ::std::abs;
    using ::std::noskipws;

    /**
     * This file concentrates all members that does not belong to any class
     */

    // Minimum value for each RGBA channel except the alpha, lower values are used in pixel-detection
    // purposes later on by the engine (see `Engine` class).
    extern const int MIN_CHANNEL;

    void decodeRGBA(uint32_t n, uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a);
    bool isFloat(const string& text);
    int clamp(int value, int min, int max);
    float clamp(float value, float min, float max);
    uint32_t encodeRGBA(const uint8_t& r, const uint8_t& g, const uint8_t& b, const uint8_t& a);
}

#endif
