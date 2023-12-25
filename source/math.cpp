
#include "../include/math.hpp"

namespace rp {
    const float EI_TOL = 0.00001f;

    bool isFloat(const string& str) {
        std::istringstream iss(str);
        float _;
        iss >> std::noskipws >> _;
        return iss.eof() && !iss.fail();
    }
    int digitCount(int n) {
        int posNum = std::abs(n);
        int digits = 1;
        int power = 1;
        while(true) {
            power *= 10;
            if(power > posNum)
                break;
            digits++;
        }
        if(n < 0) digits++; // Include the minus sign
        return ((n < 0) ? (1) : (0)) + digits;
    }
    int ensureInteger(float n) {
        // This simple function kills all demons that output integer while it is not a one, for example
        // take number 4.999999(...), C++ tells me that it is 5, so I evaluate (int)5 and get 4 ... bruh!
        return ((int)(n + EI_TOL) > (int)n) ? (ceilf(n)) : (floorf(n));
    }
    float clamp(float value, float min, float max) {
        return (value < min) ? (min) : ((value > max) ? (max) : (value));
    }

    /******************************************/
    /********** STRUCTURE: LINE EQUATION ******/
    /******************************************/

    const float LineEquation::MAX_SLOPE = 100.0f;

    LineEquation::LineEquation() {
        this->slope = 0;
        this->height = 0;
        this->domainStart = 0;
        this->domainEnd = 0;
    }
    LineEquation::LineEquation(float slope, float height, float domainStart, float domainEnd) {
        this->slope = slope;
        this->height = height;
        this->domainStart = domainStart;
        this->domainEnd = domainEnd;
    }
    float LineEquation::pointDistance(const Vector2& point) {
        return std::abs(slope * point.x - point.y + height) / sqrtf(slope * slope + 1);
    }
    Vector2 LineEquation::intersection(const LineEquation& other) const {
        // When current line is very vertical
        if(this->slope >= LineEquation::MAX_SLOPE)
            return Vector2(this->height, other.slope * this->height + other.height);
        // Any other scenario
        float x = (this->height - other.height) / (other.slope - this->slope);
        return Vector2(x, this->slope * x + this->height);
    }
    Vector2 LineEquation::operator&&(const LineEquation& other) const {
        return intersection(other);
    }
    ostream& operator<<(ostream& stream, const LineEquation& line) {
        stream << line.slope << " * x + " << line.height;
        return stream;
    }

    /*************************************************/
    /********** STRUCTURE: 2-DIMENSIONAL VECTOR ******/
    /*************************************************/

    const Vector2 Vector2::ZERO = Vector2(0, 0);
    const Vector2 Vector2::UP = Vector2(0, 1);
    const Vector2 Vector2::RIGHT = Vector2(1, 0);
    const Vector2 Vector2::DOWN = Vector2(0, -1);
    const Vector2 Vector2::LEFT = Vector2(-1, 0);

    Vector2::Vector2() {
        this->x = 0;
        this->y = 0;
    }
    Vector2::Vector2(float x, float y) {
        this->x = x;
        this->y = y;
    }
    float Vector2::dot(const Vector2& other) const {
        return x * other.x + y * other.y;
    }
    float Vector2::magnitude() const {
        return sqrtf(x * x + y * y);
    }
    Vector2 Vector2::add(const Vector2& other) const {
        return Vector2(x + other.x, y + other.y);
    }
    Vector2 Vector2::normalized() const {
        float mag = magnitude();
        return scale((mag == 0) ? (0) : (1 / mag));
    }
    Vector2 Vector2::orthogonal() const {
        return Vector2(y, -1 * x);
    }
    Vector2 Vector2::rotate(float radians) const {
        float sin = sinf(radians);
        float cos = cosf(radians);
        return Vector2(
            cos * x - sin * y,
            sin * x + cos * y
        );
    }
    Vector2 Vector2::scale(float scalar) const {
        return Vector2(x * scalar, y * scalar);
    }
    float Vector2::operator*(const Vector2& other) const {
        return dot(other);
    }
    Vector2 Vector2::operator+(const Vector2& other) const {
        return add(other);
    }
    Vector2 Vector2::operator-(const Vector2& other) const {
        return add(other.scale(-1));
    }
    Vector2 Vector2::operator*(const float scalar) const {
        return scale(scalar);
    }
    ostream& operator<<(ostream& stream, const Vector2& vec) {
        stream << "[ " << vec.x << " " << vec.y << "]";
        return stream;
    }
}
