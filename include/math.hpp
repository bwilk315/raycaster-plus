
#ifndef _RP_MATH_HPP
#define _RP_MATH_HPP

#ifdef DEBUG
#include <iostream>
#endif
#include <vector>
#include <string>
#include <sstream>
#include <cmath>

namespace rp {
    #ifdef DEBUG
    using ::std::ostream;
    #endif
    using ::std::string;
    using ::std::sqrt;
    using ::std::abs;

    class LinearFunc;
    class Vector2;

    bool isFloat(const string& str);
    int digitCount(int n);
    float clamp(float value, float min, float max);

    struct LinearFunc {
        static const float MAX_SLOPE;

        float slope;
        float height;
        float xMin, xMax;
        float yMin, yMax;

        LinearFunc();
        LinearFunc(float slope, float height);
        LinearFunc(float slope, float height, float xMin, float xMax);
        LinearFunc(float slope, float height, float xMin, float xMax, float yMin, float yMax);

        float getValue(float argument) const;
        float getDistanceFromPoint(const Vector2& point) const;
        Vector2 getCommonPoint(const LinearFunc& other) const;
    };
    #ifdef DEBUG
    ostream& operator<<(ostream& stream, const LinearFunc& func);
    #endif

    struct Vector2 {
        static const Vector2 ZERO;
        static const Vector2 UP;
        static const Vector2 RIGHT;
        static const Vector2 DOWN;
        static const Vector2 LEFT;
        
        float x, y;

        Vector2();
        Vector2(float x, float y);

        float dot(const Vector2& other) const;
        float magnitude() const;
        Vector2 normalized() const;
        Vector2 orthogonal() const;
        Vector2 rotate(float radians) const;

        Vector2 operator+(const Vector2& other) const;
        Vector2 operator-(const Vector2& other) const;
        Vector2 operator*(const float scalar) const;
        Vector2 operator/(const float scalar) const;
    };
    #ifdef DEBUG
    ostream& operator<<(ostream& stream, const Vector2& vec);
    #endif
}

#endif
