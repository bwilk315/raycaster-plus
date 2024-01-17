
#ifndef _RPGE_CAMERA_HPP
#define _RPGE_CAMERA_HPP

#include "RPGE_globals.hpp"
#include "RPGE_math.hpp"

namespace rpge {
    using ::std::abs;

    /**
     * Provides theoretical two-dimensional camera functionality.
     * 
     * Camera is built by composing three vectors: `position`, `direction` and `plane`. `position` vector
     * tells the camera position, `direction` obviously its looking direction, and `plane` the one always
     * clockwisely-perpendicular to the `direction` vector.
     * `plane` vector is especially important: it simulates right half of the projection line, you can think
     * of it as half of your monitor appearing from the top.
     */
    class Camera {
        private:
            float   fieldOfView;
            float   planeMagnitude;
            Vector2 plane;
            Vector2 position;
            Vector2 direction;
            
        public:
            static const float DIR_BIAS;
            static const float MIN_FOV;
            static const float MAX_FOV;

            Camera();
            Camera(Vector2 position, float viewAngle, float fieldOfView);

            /* Rotates the looking direction vector by `radians` rad counter-clockwisely */
            void    changeDirection(float radians);
            
            /* Moves the camera using given vector `change` */
            void    changePosition(Vector2 change);

            /* Returns camera looking direction vector */
            Vector2 getDirection() const;

            /* Returns camera field of view angle */
            float   getFieldOfView() const;

            /* Returns camera projection plane vector */
            Vector2 getPlane() const;
            
            /* Returns camera position */
            Vector2 getPosition() const;

            /* Sets the looking direction to angle of `radians` rad counter-clockwise */
            void    setDirection(float radians);

            /* Sets camera field of view angle to `radians` rad */
            void    setFieldOfView(float radians);

            /* Sets camera position using given vector `position` */
            void    setPosition(Vector2 position);
    };
    #ifdef DEBUG
    ostream& operator<<(ostream& stream, const Camera& cam);
    #endif
}

#endif
