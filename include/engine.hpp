
#ifndef _ENGINE_HPP
#define _ENGINE_HPP

#include "camera.hpp"
#include "dda.hpp"
#include "math.hpp"
#include "plane.hpp"

namespace rp {
    class Engine {
        private:
            int screenWidth;
            int screenHeight;
        public:
            Plane scene;

            Engine(int screenWidth, int screenHeight);
            bool tick();
            int getScreenWidth() const;
            int getScreenHeight() const;
    };
}

#endif
