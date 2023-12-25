
#ifndef _CAMERA_HPP
#define _CAMERA_HPP

#include "math.hpp"

namespace rp {
    using ::std::abs;

    class Camera {
        private:
            float fieldOfView;
            float planeMagnitude;
            Vector2 plane;
            Vector2 position;
            Vector2 direction;
        public:
            static const float DIR_BIAS;
            static const float MIN_FOV;
            static const float MAX_FOV;

            Camera();
            Camera(Vector2 position, float viewAngle, float fieldOfView);
            void changeDirection(float radians);
            void changePosition(Vector2 change);
            void setDirection(float radians);
            void setFieldOfView(float radians);
            void setPosition(Vector2 position);
            float getFieldOfView() const;
            Vector2 getPlane() const;
            Vector2 getPosition() const;
            Vector2 getDirection() const;
    };
}

#endif
