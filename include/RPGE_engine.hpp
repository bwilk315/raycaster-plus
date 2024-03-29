
#ifndef _RPGE_ENGINE_HPP
#define _RPGE_ENGINE_HPP

#include <chrono>
#include <cmath>
#include <map>
#include <memory>
#include <SDL2/SDL.h>
#include "RPGE_camera.hpp"
#include "RPGE_dda.hpp"
#include "RPGE_globals.hpp"
#include "RPGE_math.hpp"
#include "RPGE_scene.hpp"

namespace rpge {
    using ::std::chrono::time_point;
    using ::std::chrono::system_clock;
    using ::std::chrono::duration;
    using ::std::map;
    using ::std::unique_ptr;
    using ::std::pair;
    using ::std::make_pair;
    using ::std::sqrt;
    using ::std::tan;

    enum KeyState {
        NONE,
        DOWN,  // Key got pressed (single event)
        PRESS, // Key is pressed
        UP     // Key is not pressed anymore (single event)
    };

    class Engine {
        private:
            bool                     bClear;
            bool                     bIsCursorLocked;
            bool                     bLightEnabled;
            bool                     bRedraw;
            bool                     bRun;
            int                      iError;
            int                      iColumnsPerRay;
            int                      iFramesPerSecond;
            int                      iRowsInterval;
            int                      iScreenWidth;
            int                      iScreenHeight;
            float                    fAspectRatio;
            SDL_Color                cClearColor;
            uint64_t                 frameIndex;
            Vector2                  vLightDir;
            time_point<system_clock> tpLast;
            duration<float>          elapsedTime;
            SDL_Rect                 rClearArea;
            SDL_Rect                 rRenderArea;
            map<int, KeyState>       keyStates; // SDL Scancode -> State of that key

            const Camera* mainCamera;
            DDA*          walker;
            SDL_Renderer* sdlRend;
            SDL_Window*   sdlWindow;

        public:
            static const float SAFE_LINE_HEIGHT;
            enum {
                E_CLEAR               = 0,
                E_SDL                 = 1 << 1,  // When SDL reports some error
                E_MAIN_CAMERA_NOT_SET = 1 << 2
            };

            Engine(int screenWidth, int screenHeight);
            ~Engine();

            /* Sets all render area pixels' color to the one set before using `setClearColor` method */
            void                   clear();

            /* Returns an overall error code that in binary form represents whether some error occurred (1) or not (0),
               see `E_<error_name>` constants for more details about individual errors. */ 
            int                    getError() const;

            /* Returns time in seconds telling how long processing of the last frame has taken */
            float                  getElapsedTime() const;

            /* Returns total amount of processed frames (or `tick` method calls) */
            int                    getFrameCount() const;
            
            /* Returns state of a keyboard key having scancode `sc` (see `KeyState` for more details) */
            KeyState               getKeyState(int sc) const;

            /* Returns mouse position in the screen coordinates */
            Vector2                getMousePosition() const;

            /* Returns area of the screen (in pixels) which is occupied by the render, it depends on the fit mode
               set using `setRenderFitMode` method. */
            SDL_Rect               getRenderArea() const;

            /* Returns pointer to the SDL renderer structure */
            SDL_Renderer*          getRendererHandle();

            /* Returns current height of the screen in pixels */
            int                    getScreenHeight() const;

            /* Returns current width of the screen in pixels */
            int                    getScreenWidth() const;

            /* Returns pointer to the DDA algorithm provider, also known as "walker" because it makes ray walking
               possible. You can customize it to your needs through its interface (see `DDA` class for details). */
            DDA*                   getWalker();

            /* Returns pointer to the SDL window structure, you can use it to do things not supported by the engine */
            SDL_Window*            getWindowHandle();

            /* Allows for drawing process on the entire render area once per frame */
            void                   render();
            
            /* Frees allocated memory, then sets the stop flag which makes `tick` method unavailable */
            void                   stop();

            /* Screen pixels' will have their RGB channels set to these on clearing with `clear` method */
            void                   setClearColor(uint8_t r, uint8_t g, uint8_t b);

            /* The `locked` flag enables/disables periodical cursor position reset to the center of screen */
            void                   setCursorLock(bool locked);

            /* Makes one sent ray provide hit data for next `n` of them, so there will be total of `getScreenWidth() / n` rays */
            void                   setColumnsPerRay(int n);

            /* The `visible` flag shows/hides the cursor in the window bounds */
            void                   setCursorVisibility(bool visible);

            /* Makes the next calls to `tick` method try to execute at constant `fps` frames per second either
               by delaying (when execution is too fast) or maxing (when execution is too slow). */
            void                   setFrameRate(int fps);

            /* Configures the global light source. The `enabled` flag tells whether it should be turned on/off
               and the `angle` value in radians, an angle which right vector ([1, 0]) should be rotated counter-
               -clockwisely by; the source emmits light linearly in that direction. */
            void                   setLightBehavior(bool enabled, float angle);

            /* Updates the main camera pointer so it points the `camera` instance of class `Camera`. It is used
               in rendering process. */
            void                   setMainCamera(const Camera* camera);

            /* Specifies which part of the screen should be cleared when calling `clear` method */
            void                   setClearArea(const SDL_Rect& rect);

            /* Specifies which part of the screen is used to render frames, when calling `render` method.
             * This method resets clear area set previously using `setClearArea` method to the whole render area. */
            void                   setRenderArea(const SDL_Rect& rect);

            /* Makes one column pixel provide data for next `n` of them, so there will be total of `columnHeight / n` pixels */
            void                   setRowsInterval(int n);

            /* Function responsible for handling user input and drawing the render area. You should call it as often
               as possible to make the engine the most responsive, you do not need to delay it by hand in order to save
               CPU power because framerate set using `setFrameRate` ensures limits are met. */
            bool                   tick();
    };
}

#endif
