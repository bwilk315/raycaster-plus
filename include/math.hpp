
#ifndef _RP_MATH_HPP
#define _RP_MATH_HPP 

#ifdef DEBUG
#include <iostream>
#endif
#include <cmath>
#include <vector>

namespace rp {
    #ifdef DEBUG
    using ::std::ostream;
    using ::std::cout;
    using ::std::endl;
    #endif
    using ::std::abs;
    using ::std::sqrt;

    extern const float SQRT2;
    extern const float INV_SQRT2;

    class LinearFunc;
    class Vector2;

    struct LinearFunc {
        float slope;      // Rate of change
        float height;     // Height above the arguments axis
        float xMin, xMax; // Domain range
        float yMin, yMax; // Values range

        LinearFunc();
        LinearFunc(float slope, float height);
        LinearFunc(float slope, float height, float xMin, float xMax);
        LinearFunc(float slope, float height, float xMin, float xMax, float yMin, float yMax);
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
        Vector2 orthogonal() const;  // Vector that is clockwisely-perpendicular
        Vector2 rotate(float radians) const;  // Vector rotated anti-clockwisely
    };
    void operator+=(Vector2& a, const Vector2& b);
    void operator-=(Vector2& a, const Vector2& b);
    void operator*=(Vector2& vec, float scalar);
    void operator/=(Vector2& vec, float scalar);
    bool operator==(const Vector2& a, const Vector2& b);
    bool operator!=(const Vector2& a, const Vector2& b);
    Vector2 operator+(const Vector2& a, const Vector2& b);
    Vector2 operator-(const Vector2& a, const Vector2& b);
    Vector2 operator*(const Vector2& vec, float scalar);
    Vector2 operator*(float scalar, const Vector2& vec);
    Vector2 operator/(const Vector2& vec, float scalar);
    #ifdef DEBUG
    ostream& operator<<(ostream& stream, const Vector2& vec);
    #endif
}

#endif
