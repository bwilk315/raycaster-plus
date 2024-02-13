
#include <RPGE_camera.hpp>

namespace rpge
{
    const float Camera::DIR_BIAS = 0.0001f;
    const float Camera::MIN_FOV = 0.01f;
    const float Camera::MAX_FOV = M_PI - 0.01f;

    Camera::Camera()
    {
        setFieldOfView(M_PI_2);
        setPosition(Vector2::ZERO);
        setDirection(0);
    }
    Camera::Camera(Vector2 position, float viewAngle, float fieldOfView)
    {
        setFieldOfView(fieldOfView);
        setPosition(position);
        setDirection(viewAngle);
    }
    void Camera::changeDirection(float radians)
    {
        direction = direction.rotate(radians);
        plane = plane.rotate(radians);
    }
    void Camera::changePosition(Vector2 change)
    {
        position += change;
    }
    void Camera::setDirection(float radians)
    {
        // Applying small errors sometimes guarantee infinite-slope-immune behaviour
        direction = (Vector2::RIGHT).rotate(
            radians - (abs(radians) == M_PI_2 ? DIR_BIAS : 0)
        );
        plane = (Vector2::DOWN).rotate(
            (radians == 0 || radians == M_PI) ? (radians - DIR_BIAS) : (radians)
        ) * planeMagnitude;
    }
    void Camera::setFieldOfView(float radians)
    {
        fieldOfView = clamp(radians, Camera::MIN_FOV, Camera::MAX_FOV);
        plane = plane / planeMagnitude; // Make the plane a unit vector
        planeMagnitude = tanf(radians / 2);
        plane = plane * planeMagnitude; // Apply new magnitude of the plane vector
    }
    void Camera::setPosition(Vector2 position)
    {
        this->position = position;
    }
    float Camera::getFieldOfView() const
    {
        return fieldOfView;
    }
    Vector2 Camera::getPlane() const
    {
        return plane;
    }
    Vector2 Camera::getPosition() const
    {
        return position;
    }
    Vector2 Camera::getDirection() const
    {
        return direction;
    }
    #ifdef DEBUG
    ostream& operator<<(ostream& stream, const Camera& cam)
    {
        stream << "Camera(fieldOfView=" << cam.getFieldOfView() << ", plane=" << cam.getPlane();
        stream << ", position=" << cam.getPosition() << ", direction=" << cam.getDirection() << ")";
        return stream;
    }
    #endif
}
