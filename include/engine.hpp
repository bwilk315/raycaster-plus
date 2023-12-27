
#ifndef _ENGINE_HPP
#define _ENGINE_HPP

#include <map>
#include <chrono>
#include <SDL2/SDL.h>
#include "camera.hpp"
#include "dda.hpp"
#include "math.hpp"
#include "scene.hpp"

namespace rp {
    using ::std::chrono::time_point;
    using ::std::chrono::system_clock;
    using ::std::chrono::duration;
    using ::std::pair;
    using ::std::map;

    enum KeyState {
        NONE,
        DOWN,
        PRESS,
        UP
    };

    class Engine {
        private:
            bool bRun;
            bool bIsCursorLocked;
            bool bLightEnabled;
            int iColumnsPerRay;
            int iScreenWidth;
            int iScreenHeight;
            int msFrameDuration;
            float fAspectRatio;
            float fMaxTileDist;
            float fLightAngle;
            uint64_t frameIndex;
            time_point<system_clock> tpLast;
            duration<float> elapsedTime;
            map<int, KeyState> keyStates;

            const Camera* mainCamera;
            DDA* walker = nullptr;
            SDL_Window* sdlWindow;
            SDL_Renderer* sdlRenderer;
        public:

            Engine(int screenWidth, int screenHeight);
            ~Engine();
            void stop();
            void setCursorLock(bool locked);
            void setCursorVisibility(bool visible);
            void setColumnsPerRay(int columns);
            void setFrameRate(int framesPerSecond);
            void setLightBehavior(bool enabled, float angle);
            void setMainCamera(const Camera* camera);
            int getScreenWidth() const;
            int getScreenHeight() const;
            float getElapsedTime() const;
            KeyState getKeyState(int scanCode) const;
            DDA* getWalker();
            SDL_Window* getWindowHandle();
            SDL_Renderer* getRendererHandle();
            bool tick();
    };
}

#endif
