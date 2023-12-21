
#ifndef _RMATH_HPP
#define _RMATH_HPP

#include <vector>
#include <string>
#include <cmath>

// Checks if number is integer (1, 2, ...) or float (1.2, 3.14, ...)
bool isNumber(const std::string& str);
int digitCount(int n);
float clamp(float value, float min, float max);

class vec2f;

struct vec2i {
    int x, y;

    vec2i();
    vec2i(int x, int y);
    vec2f toFloat() const;
};

class vec2f {
    public:
        static const vec2f ZERO;
        static const vec2f UP;
        static const vec2f RIGHT;
        static const vec2f DOWN;
        static const vec2f LEFT;
        
        float x, y;

        vec2f();
        vec2f(float x, float y);
        vec2f add(const vec2f& other) const;
        float dot(const vec2f& other) const;
        float magnitude() const;
        vec2f normalized() const;
        vec2f orthogonal() const;
        vec2f rotate(float radians) const; // Clockwise rotation
        vec2f scale(float scalar) const;
        vec2i toInt() const;

        vec2f operator+(const vec2f& other) const;
        vec2f operator-(const vec2f& other) const;
        vec2f operator*(const float scalar) const;
        float operator*(const vec2f& other) const;
};

#endif
