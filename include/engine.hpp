
#ifndef _RP_ENGINE_HPP
#define _RP_ENGINE_HPP

#ifdef DEBUG
#include <iostream>
#endif
#include <map>
#include <chrono>
#include <SDL2/SDL.h>
#include "camera.hpp"
#include "dda.hpp"
#include "math.hpp"
#include "scene.hpp"
#include "texture.hpp"

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
            float fSumFPS;
            float fMaxTileDist;
            uint64_t frameIndex;
            Vector2  vLightDir;
            RenderFitMode            renderFitMode;
            time_point<system_clock> tpLast;
            duration<float>          elapsedTime;
            map<int, KeyState>       keyStates;

            uint32_t*     pixels      = nullptr;  // Array of window surface pixels
            Camera*       mainCamera  = nullptr;
            DDA*          walker      = nullptr;
            SDL_Surface*  sdlSurface  = nullptr;
            SDL_Window*   sdlWindow   = nullptr;

            void updateSurface();
            RayHitInfo simulateBoundaryEnter(const Vector2& pos, const Vector2& dir);
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
            void setMainCamera(Camera* camera);
            void setWindowResize(bool enabled);
            int setRenderFitMode(const RenderFitMode& rfm);
            int getScreenWidth() const;
            int getScreenHeight() const;
            int getRenderWidth();
            int getRenderHeight();
            float getElapsedTime() const;
            KeyState getKeyState(int scanCode) const;
            DDA* const getWalker();
            SDL_Window* getWindowHandle();
            bool tick();
    };
}

#endif
