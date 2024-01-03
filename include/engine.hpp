
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
#include "globals.hpp"
#include "math.hpp"
#include "scene.hpp"
#include "texture.hpp"

namespace rp {
    #ifdef DEBUG
    using ::std::cout;
    using ::std::endl;
    #endif
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
        UNKNOWN,
        STRETCH, // Render gets stretched to fill the whole screen
        SQUARE   // Render is the biggest square possible to fit with the current resolution
    };

    /**
     * It is too early for describing this class, it is dynamically changing right now.
     * One important thing to notice: be aware of calling order, e.g. if you want to set
     * rows interval, set the render fit mode and resolution first.
     */
    class Engine {
        private:
            bool bAllowWindowResize;
            bool bLimitClear;
            bool bIsCursorLocked;
            bool bLightEnabled;
            bool bRedraw;
            bool bRun;
            int iError;
            int iColumnsPerRay;
            int iFramesPerSecond;
            int iRenderWidth;
            int iRenderHeight;
            int iRowsInterval;
            int iHorOffset;
            int iScreenWidth;
            int iScreenHeight;
            int iVerOffset;
            float fAspectRatio;
            uint64_t frameIndex;
            RenderFitMode            renderFitMode;
            Vector2                  vLightDir;
            time_point<system_clock> tpLast;
            duration<float>          elapsedTime;
            SDL_Rect                 rRedrawArea;
            map<int, KeyState>       keyStates; // SDL Scancode -> State of that key

            uint32_t*     pixels; // Array of window surface pixels
            const Camera* mainCamera;
            DDA*          walker;
            SDL_Surface*  sdlSurface;
            SDL_Window*   sdlWindow;

            // This method is called when screen size changes, because pixels array must match the actual size
            void updateSurface();
            // It is used once per ray to render walls of the tile in which it starts stepping
            RayHitInfo simulateBoundaryEnter(const Vector2& pos, const Vector2& dir);
        public:
            static const float MAX_LINE_SLOPE;

            enum {
                E_CLEAR               = 0,
                E_MAIN_CAMERA_NOT_SET = 1 << 1,
                E_WRONG_CALL_ORDER    = 1 << 2, // Happens when you perform action that is dependant on other one (not called yet)
                E_SDL                 = 1 << 3  // When SDL reports some error
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
            void setRowsInterval(int interval);
            void setRenderFitMode(const RenderFitMode& rfm);
            void setWindowResize(bool enabled);
            void requestRedraw();
            void requestRedraw(int x, int y, int w, int h, bool limited = false);
            int getError() const;
            int getFrameCount() const;
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
