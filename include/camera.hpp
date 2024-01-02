
#ifndef _RP_CAMERA_HPP
#define _RP_CAMERA_HPP

#ifdef DEBUG
#include <iostream>
#endif
#include "globals.hpp"
#include "math.hpp"

namespace rp {
    #ifdef DEBUG
    using ::std::ostream;
    #endif
    using ::std::abs;

    /**
     * Provides theoretical camera functionality (practical part is done by engine).
     * 
     * Camera is built by composing three vectors: `position`, `direction` and `plane`. `position` vector
     * tells the camera position, `direction` obviously its looking direction, and `plane` the one
     * clockwisely-perpendicular to the `direction` vector.
     * `plane` vector (and its magnitude) is especially important: it simulates a screen space in form of
     * a line (as if you were looking at your monitor from the top).
     */
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
            float getFieldOfView() const;
            Vector2 getPlane() const;
            Vector2 getPosition() const;
            Vector2 getDirection() const;
            void changeDirection(float radians);
            void changePosition(Vector2 change);
            void setDirection(float radians);
            void setFieldOfView(float radians);
            void setPosition(Vector2 position);
    };
    #ifdef DEBUG
    ostream& operator<<(ostream& stream, const Camera& cam);
    #endif
}

#endif
