
#include "../include/math.hpp"

bool isFloat(const std::string& str) {
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
    return (n < 0 ? 1 : 0) + digits;
}
float clamp(float value, float min, float max) {
    return value < min ? min : (value > max ? max : value);
}

vec2i::vec2i() {
    this->x = 0;
    this->y = 0;
}
vec2i::vec2i(int x, int y) {
    this->x = x;
    this->y = y;
}
vec2f vec2i::toFloat() const {
    return vec2f((float)this->x, (float)this->y);
}

const vec2f vec2f::ZERO = vec2f(0, 0);
const vec2f vec2f::UP = vec2f(0, 1);
const vec2f vec2f::RIGHT = vec2f(1, 0);
const vec2f vec2f::DOWN = vec2f(0, -1);
const vec2f vec2f::LEFT = vec2f(-1, 0);

vec2f::vec2f() {
    this->x = 0;
    this->y = 0;
}
vec2f::vec2f(float x, float y) {
    this->x = x;
    this->y = y;
}
vec2f vec2f::add(const vec2f& other) const {
    return vec2f(this->x + other.x, this->y + other.y);
}
float vec2f::dot(const vec2f& other) const {
    return this->x * other.x + this->y * other.y;
}
float vec2f::magnitude() const {
    return sqrtf(this->x * this->x + this->y * this->y);
}
vec2f vec2f::normalized() const {
    return this->scale(1 / this->magnitude());
}
vec2f vec2f::orthogonal() const {
    return vec2f(this->y, -1 * this->x);
}
vec2f vec2f::rotate(float radians) const {
    // Angle is inverted to rotate clockwisely by default
    float sin = sinf(-1 * radians);
    float cos = cosf(-1 * radians);
    return vec2f(
        cos * this->x - sin * this->y,
        sin * this->x + cos * this->y
    );
}
vec2f vec2f::scale(float scalar) const {
    return vec2f(this->x * scalar, this->y * scalar);
}
vec2i vec2f::toInt() const {
    return vec2i((int)this->x, (int)this->y);
}
vec2f vec2f::operator+(const vec2f& other) const {
    return this->add(other);
}
vec2f vec2f::operator-(const vec2f& other) const {
    return this->add(other.scale(-1));
}
vec2f vec2f::operator*(const float scalar) const {
    return this->scale(scalar);
}
float vec2f::operator*(const vec2f& other) const {
    return this->dot(other);
}
