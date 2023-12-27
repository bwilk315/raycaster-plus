
#ifndef _RMATH_HPP
#define _RMATH_HPP

#include <vector>
#include <string>
#include <sstream>
#include <cmath>

namespace rp {
    using ::std::string;
    using ::std::ostream;
    using ::std::sqrt;
    using ::std::abs;
    
    extern const float EI_TOL;

    class LineEquation;
    class Vector2;

    bool isFloat(const string& str);
    int digitCount(int n);
    int ensureInteger(float n);
    float clamp(float value, float min, float max);

    struct LineEquation {
        static const float MAX_SLOPE;

        float slope;
        float height;
        float domainStart;
        float domainEnd;

        LineEquation();
        LineEquation(float slope, float intercept, float domainStart, float domainEnd);
        float pointDistance(const Vector2& point);
        // Having one kinda vertical line, use it as the method host to make it work
        Vector2 intersection(const LineEquation& other) const;
        Vector2 operator&&(const LineEquation& other) const;
    };
    ostream& operator<<(ostream& stream, const LineEquation& line);

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
        Vector2 add(const Vector2& other) const;
        Vector2 normalized() const;
        Vector2 orthogonal() const;
        Vector2 rotate(float radians) const;
        Vector2 scale(float scalar) const;

        float operator*(const Vector2& other) const;
        Vector2 operator+(const Vector2& other) const;
        Vector2 operator-(const Vector2& other) const;
        Vector2 operator*(const float scalar) const;
    };
    ostream& operator<<(ostream& stream, const Vector2& vec);
}

#endif
