
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
        NONE,  // Nothing happens
        DOWN,  // Key got pressed (single event)
        PRESS, // Key is pressed
        UP     // Key is not pressed anymore (single event)
    };
    enum RenderFitMode {
        UNKNOWN,
        STRETCH, // Render gets stretched to fill the whole screen area
        SQUARE   // Render is the biggest square possible to fit with the current resolution
    };

    class Engine {
        private:
            bool bAllowWindowResize;
            bool bClear;
            bool bIsCursorLocked;
            bool bLightEnabled;
            bool bRedraw;
            bool bRun;
            int iError;
            int iColumnsPerRay;
            int iFramesPerSecond;
            int iRowsInterval;
            int iScreenWidth;
            int iScreenHeight;
            float fAspectRatio;
            uint64_t frameIndex;
            RenderFitMode            renderFitMode;
            Vector2                  vLightDir;
            time_point<system_clock> tpLast;
            duration<float>          elapsedTime;
            SDL_Rect                 rClearArea;
            SDL_Rect                 rRenderArea;
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

            /* Makes all pixels of the render area black once per frame */
            void clear();

            /* Makes all pixels of the render area included in `rect` area black once, meaning that another
               call to this method alters the previous one's setting. */
            void clear(const SDL_Rect& rect);

            /* Returns an overall error code that in binary form represents whether some error occurred (1) or not (0),
               see `E_<error_name>` constants for more details about individual errors. */ 
            int getError() const;

            /* Returns time in milliseconds telling how long processing of the last frame has taken */
            float getElapsedTime() const;

            /* Returns total amount of processed frames (or `tick` method calls) */
            int getFrameCount() const;
            
            /* Returns state of a keyboard key having scancode `sc` (see `KeyState` for more details) */
            KeyState getKeyState(int sc) const;

            /* Returns area of the screen (in pixels) which is occupied by the render, it depends on the fit mode
               set using `setRenderFitMode` method. */
            SDL_Rect getRenderArea() const;

            /* Returns current height of the screen in pixels */
            int getScreenHeight() const;

            /* Returns current width of the screen in pixels */
            int getScreenWidth() const;

            /* Returns pointer to the DDA algorithm provider, also known as "walker" because it makes ray walking
               possible. You can customize it to your needs through its interface (see `DDA` class for details). */
            DDA* const getWalker();

            /* Returns pointer to the SDL window structure, you can use it to do things not supported by the engine */
            SDL_Window* getWindowHandle();

            /* Allows for drawing process on the entire render area once per frame */
            void render();

            /* Allows for drawing process on the render area included in `rect` area once, meaning that another
               call to this method changes the previous one's setting. */
            void render(const SDL_Rect& rect);

            /* Frees allocated memory, then sets the stop flag which makes `tick` method unavailable */
            void stop();

            /* The `locked` flag enables/disables periodical cursor position reset to the center of screen */
            void setCursorLock(bool locked);

            /* Makes one sent ray provide hit data for next `n` of them, so there will be total of `getScreenWidth() / n` rays */
            void setColumnsPerRay(int n);

            /* The `visible` flag shows/hides the cursor in the window bounds */
            void setCursorVisibility(bool visible);

            /* Makes the next calls to `tick` method try to execute at constant `fps` frames per second either
               by delaying (when execution is too fast) or maxing (when execution is too slow). */
            void setFrameRate(int fps);

            /* Configures the global light source. The `enabled` flag tells whether it should be turned on/off
               and the `angle` value in radians, an angle which right vector ([1, 0]) should be rotated counter-
               -clockwisely by; the source emmits light linearly in that direction. */
            void setLightBehavior(bool enabled, float angle);

            /* Updates the main camera pointer so it points the `camera` instance of class `Camera`. It is used
               in rendering process. */
            void setMainCamera(const Camera* camera);

            /* Sets behavior of the render area to `rfm` (see description of `RenderFitMode` for more details) */
            void setRenderFitMode(const RenderFitMode& rfm);

            /* Makes one column pixel provide data for next `n` of them, so there will be total of `columnHeight / n` pixels */
            void setRowsInterval(int n);

            /* The `enabled` flag specifies if window resizability should be turned on/off */
            void setWindowResize(bool enabled);

            /* Function responsible for handling user input and drawing the render area. You should call it as often
               as possible to make the engine the most responsive, you do not need to delay it by hand in order to save
               CPU power because framerate set using `setFrameRate` ensures limits are met. */
            bool tick();
    };
}

#endif
