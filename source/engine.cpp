
#include "../include/engine.hpp"

namespace rp {
    Engine::Engine(int screenWidth, int screenHeight) {
        this->screenWidth = screenWidth;
        this->screenHeight = screenHeight;
    }
    bool Engine::tick() {
        return false;
    }
    int Engine::getScreenWidth() const {
        return this->screenWidth;
    }
    int Engine::getScreenHeight() const {
        return this->screenHeight;
    }
}
