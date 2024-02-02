
/**
 * This file concentrates all functions/variables that are used extensively by more than one class,
 * therefore not really belonging to any of them.
 */

#ifndef _RPGE_GLOBALS_HPP
#define _RPGE_GLOBALS_HPP

#ifdef DEBUG
#include <iostream>
#endif
#include <cstdint>
#include <sstream>
#include <string>

namespace rpge {
    #ifdef DEBUG
    using ::std::cout;
    using ::std::endl;
    using ::std::ostream;
    #endif
    using ::std::istringstream;
    using ::std::string;
    using ::std::noskipws;

    /* Limits the number `value` so it is included in inclusive range < `min` ; `max` >.
     * If `value` exceeds minimum or maximum, it becomes the limiting value. */
    int   clamp(int value, int min, int max);
    float clamp(float value, float min, float max);

    /* Checks whether given string `text` is a floating point number in any form, for example
     * number 100.23 can be written in scienfitic notation as 1.0023e+2, both are considered floats. */
    bool  isFloat(const string& text);

    uint32_t enColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    void deColor(uint32_t color, uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a);
}

#endif
