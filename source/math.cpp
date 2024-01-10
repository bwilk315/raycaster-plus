
#include "../include/math.hpp"

namespace rp {
    
    const float INV_SQRT2 = sqrtf(2.0f);

    /********************************************/
    /********** STRUCTURE: LINEAR FUNC **********/
    /********************************************/

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
    #ifdef DEBUG
    ostream& operator<<(ostream& stream, const LinearFunc& func) {
        stream << "LinearFunc(slope=" << func.slope << ", height=" << func.height << ", xMin=" << func.xMin;
        stream << ", xMax=" << func.xMax << ", yMin=" << func.yMin << ", yMax=" << func.yMax << ")";
        return stream;
    }
    #endif

    /*****************************************/
    /********** STRUCTURE: VECTOR 2 **********/
    /*****************************************/

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
        return mag == 0 ? ZERO : Vector2(x / mag, y / mag);
    }
    Vector2 Vector2::orthogonal() const {
        return Vector2(y, -1 * x);
    }
    Vector2 Vector2::rotate(float radians) const {
        float sin = sinf(radians);
        float cos = cosf(radians);
        return Vector2(cos * x - sin * y, sin * x + cos * y);
    }
    void operator+=(Vector2& a, const Vector2& b) {
        a.x += b.x;
        a.y += b.y;
    }
    void operator-=(Vector2& a, const Vector2& b) {
        a.x -= b.x;
        a.y -= b.y;
    }
    void operator*=(Vector2& vec, float scalar) {
        vec.x *= scalar;
        vec.y *= scalar;
    }
    void operator/=(Vector2& vec, float scalar) {
        vec.x /= scalar;
        vec.y /= scalar;
    }
    bool operator==(const Vector2& a, const Vector2& b) {
        return a.x == b.x && a.y == b.y;
    }
    bool operator!=(const Vector2& a, const Vector2& b) {
        return a.x != b.x || a.y != b.y;
    }
    Vector2 operator+(const Vector2& a, const Vector2& b) {
        return Vector2(a.x + b.x, a.y + b.y);
    }
    Vector2 operator-(const Vector2& a, const Vector2& b) {
        return Vector2(a.x - b.x, a.y - b.y);
    }
    Vector2 operator*(const Vector2& vec, float scalar) {
        return Vector2(vec.x * scalar, vec.y * scalar);
    }
    Vector2 operator*(float scalar, const Vector2& vec) {
        return vec * scalar;
    }
    Vector2 operator/(const Vector2& vec, float scalar) {
        return Vector2(vec.x / scalar, vec.y / scalar);
    }
    #ifdef DEBUG
    ostream& operator<<(ostream& stream, const Vector2& vec) {
        stream << "Vector2(x=" << vec.x << ", y=" << vec.y << ")";
        return stream;
    }
    #endif
}
