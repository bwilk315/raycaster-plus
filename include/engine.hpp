
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
    enum RenderFitMode {
        STRETCH, // Render gets stretched to fill the whole screen
        SQUARE   // Render is the biggest square possible to fit with the current resolution
    };

    class Engine {
        private:
            bool bAllowWindowResize;
            bool bIsCursorLocked;
            bool bLightEnabled;
            bool bRun;
            int iColumnsPerRay;
            int iColumnsCount;
            int msFrameDuration;
            int iHorOffset;
            int iScreenWidth;
            int iScreenHeight;
            int iVerOffset;
            float fAspectRatio;
            float fLightAngle;
            float fMaxTileDist;
            uint64_t frameIndex;
            RenderFitMode renderFitMode;
            time_point<system_clock> tpLast;
            duration<float> elapsedTime;
            map<int, KeyState> keyStates;

            const Camera* mainCamera = nullptr;
            DDA* walker = nullptr;
            SDL_Window* sdlWindow = nullptr;
            SDL_Renderer* sdlRenderer = nullptr;
        public:
            enum {
                E_CLEAR,
                E_MAIN_CAMERA_NOT_SET
            };

            Engine(int screenWidth, int screenHeight);
            ~Engine();
            void stop();
            void setCursorLock(bool locked);
            void setCursorVisibility(bool visible);
            void setColumnsPerRay(int columns);
            void setFrameRate(int framesPerSecond);
            void setLightBehavior(bool enabled, float angle);
            void setMainCamera(const Camera* camera);
            void setWindowResize(bool enabled);
            int setRenderFitMode(const RenderFitMode& rfm);
            int getScreenWidth() const;
            int getScreenHeight() const;
            int getRenderWidth();
            int getRenderHeight();
            float getElapsedTime() const;
            KeyState getKeyState(int scanCode) const;
            DDA* getWalker();
            SDL_Window* getWindowHandle();
            SDL_Renderer* getRendererHandle();
            bool tick();
    };
}

#endif
