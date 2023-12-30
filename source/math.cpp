
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
    float clamp(float value, float min, float max) {
        return (value < min) ? (min) : ((value > max) ? (max) : (value));
    }

    /********************************************/
    /********** STRUCTURE: LINEAR FUNCTION ******/
    /********************************************/

    const float LinearFunc::MAX_SLOPE = 1e4;

    LinearFunc::LinearFunc() {
        this->slope = 0;
        this->height = 0;
        this->xMin = 0;
        this->xMax = 1;
        this->yMin = 0;
        this->yMax = 1;
    }
    LinearFunc::LinearFunc(float slope, float height) : LinearFunc() {
        this->slope = slope;
        this->height = height;
    }
    LinearFunc::LinearFunc(float slope, float height, float xMin, float xMax) : LinearFunc(slope, height) {
        this->xMin = xMin;
        this->xMax = xMax;
    }
    LinearFunc::LinearFunc(float slope, float height, float xMin, float xMax, float yMin, float yMax) : LinearFunc(slope, height, xMin, xMax) {
        this->yMin = yMin;
        this->yMax = yMax;
    }
    float LinearFunc::getValue(float argument) const {
        return slope * argument + height;
    }
    float LinearFunc::getDistanceFromPoint(const Vector2& point) const {
        return abs(slope * point.x - point.y + height) / sqrt(slope * slope + 1);
    }
    Vector2 LinearFunc::getCommonPoint(const LinearFunc& other) const {
        float x = (this->height - other.height) / (other.slope - this->slope);
        return Vector2(x, getValue(x));
    }
    #ifdef DEBUG
    ostream& operator<<(ostream& stream, const LinearFunc& func) {
        stream << "LinearFunc(slope=" << func.slope << ", height=" << func.height << ", xMin=" << func.xMin;
        stream << ", xMax=" << func.xMax << ", yMin=" << func.yMin << ", yMax=" << func.yMax << ")";
        return stream;
    }
    #endif

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
        return this->x * other.x + this->y * other.y;
    }
    float Vector2::magnitude() const {
        return sqrtf(x * x + y * y);
    }
    Vector2 Vector2::normalized() const {
        float mag = magnitude();
        return mag == 0 ? Vector2::ZERO : Vector2(x / mag, y / mag);
    }
    Vector2 Vector2::orthogonal() const {
        return Vector2(y, -1 * x);
    }
    Vector2 Vector2::rotate(float radians) const {
        float sin = sinf(radians);
        float cos = cosf(radians);
        return Vector2(cos * x - sin * y, sin * x + cos * y);
    }
    Vector2 Vector2::operator+(const Vector2& other) const {
        return Vector2(this->x + other.x, this->y + other.y);
    }
    Vector2 Vector2::operator-(const Vector2& other) const {
        return Vector2(this->x - other.x, this->y - other.y);
    }
    Vector2 Vector2::operator*(const float scalar) const {
        return Vector2(x * scalar, y * scalar);
    }
    Vector2 Vector2::operator/(const float scalar) const {
        return Vector2(x / scalar, y / scalar);
    }
    #ifdef DEBUG
    ostream& operator<<(ostream& stream, const Vector2& vec) {
        stream << "Vector2(x=" << vec.x << ", y=" << vec.y << ")";
        return stream;
    }
    #endif
}
