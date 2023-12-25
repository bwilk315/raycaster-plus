
#include "../include/camera.hpp"

namespace rp {
    const float Camera::MIN_FOV = 0.01f;
    const float Camera::MAX_FOV = M_PI - 0.01f;

    Camera::Camera() {
        setFieldOfView(M_PI_2);
        setPosition(Vector2::ZERO);
        setDirection(0);
    }
    Camera::Camera(Vector2 position, float viewAngle, float fieldOfView) {
        setFieldOfView(fieldOfView);
        setPosition(position);
        setDirection(viewAngle);
    }
    void Camera::changeDirection(float radians) {
        direction = direction.rotate(radians);
        plane = plane.rotate(radians);
    }
    void Camera::changePosition(Vector2 change) {
        position = position + change;
    }
    void Camera::setDirection(float radians) {
        direction = (Vector2::RIGHT).rotate(radians);
        plane = (Vector2::DOWN).rotate(radians) * planeMagnitude;
    }
    void Camera::setFieldOfView(float radians) {
        fieldOfView = clamp(radians, Camera::MIN_FOV, Camera::MAX_FOV);
        planeMagnitude = tanf(radians / 2);
    }
    void Camera::setPosition(Vector2 position) {
        this->position = position;
    }
    float Camera::getFieldOfView() const {
        return fieldOfView;
    }
    Vector2 Camera::getPlane() const {
        return plane;
    }
    Vector2 Camera::getPosition() const {
        return position;
    }
    Vector2 Camera::getDirection() const {
        return direction;
    }
}
