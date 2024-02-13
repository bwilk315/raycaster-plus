
#include <RPGE_globals.hpp>

namespace rpge
{
    bool isFloat(const string& str)
    {
        istringstream iss(str);
        float _;
        iss >> noskipws >> _;
        return iss.eof() && !iss.fail();
    }
    int clamp(int value, int min, int max)
    {
        return value <= min ? min : (value >= max ? max : value);
    }
    float clamp(float value, float min, float max)
    {
        return value <= min ? min : (value >= max ? max : value);
    }
    uint32_t enColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
    {
        return ((a * 256 + r) * 256 + g) * 256 + b;
    }
    void deColor(uint32_t color, uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a)
    {
        b = color % 256; color /= 256;
        g = color % 256; color /= 256;
        r = color % 256; color /= 256;
        a = color % 256;
    }
}
