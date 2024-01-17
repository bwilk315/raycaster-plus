
#include <RPGE_globals.hpp>

namespace rpge {
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
}
