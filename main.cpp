
//#define DEBUG_MODE

#include <iostream>
#include <chrono>
#include <SDL2/SDL.h>
#include "include/math.hpp"
#include "include/print.hpp"

#define EI_TOLERANCE 0.00001f

/********** RAYCASTER CONFIGURATION **********/
// Video settings
#define SCREEN_WIDTH 1200
#define SCREEN_HEIGHT 700
#define LOOP_FPS 60
#define COLS_PER_RAY 2
#define MAX_TILE_DIST 30
// Player settings
#define MOVE_SPEED 3
#define FOV_ANGLE M_PI / 2
#define TURN_SPEED M_PI / 1.5
// World settings
#define SUN_ANGLE M_PI / 2

int ensureInteger(float n);
float linePointDist(const Camera& cam, const vec2f& point);

int main() {    
    /********** SET UP **********/
    const vec2f sunDir = vec2f::RIGHT.rotate(-1 * SUN_ANGLE);
    const float aspectRatio = SCREEN_HEIGHT / (float)SCREEN_WIDTH;
    const int frameDurationMs = 1e3 / LOOP_FPS;
    Plane world = Plane("world.plane");

    std::cout << world.getError() << std::endl;
    //return 0;

    Camera camera = Camera(5.5f, 2.5f, FOV_ANGLE, 0);
    DDA_Algorithm dda = DDA_Algorithm(world, MAX_TILE_DIST);
    SDL_Window* window;
    SDL_Renderer* renderer;
    std::chrono::time_point<std::chrono::system_clock> tpLastFrame, tpCurrFrame;
    std::chrono::duration<float> durDelta;
    bool keyStates[SDL_NUM_SCANCODES];
    bool runGame = true;

    /********** INITIALIZATION **********/
    SDL_InitSubSystem(SDL_INIT_VIDEO);
    SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN, &window, &renderer);
    SDL_SetWindowTitle(window, "Raycaster");
    memset(keyStates, false, SDL_NUM_SCANCODES);
    tpLastFrame = std::chrono::system_clock::now();

    float TEMP = 0.0f;
    
    /********** MAIN LOOP **********/
    while(runGame) {
        // Compute time that elapsed since the last frame (AKA delta time)
        tpCurrFrame = std::chrono::system_clock::now();
        durDelta = tpCurrFrame - tpLastFrame;
        tpLastFrame = tpCurrFrame;
        float elapsedTime = durDelta.count();
        // Interpret SDL events if there are any
        SDL_Event event;
        while(SDL_PollEvent(&event)) {
            switch(event.type) {
                case SDL_QUIT:
                    runGame = false;
                    break;
                case SDL_KEYDOWN:
                    keyStates[event.key.keysym.scancode] = true;
                    break;
                case SDL_KEYUP:
                    keyStates[event.key.keysym.scancode] = false;
                    break;
            }
        }

        // Rotation!
        TEMP += elapsedTime;
        vec2f v = vec2f::RIGHT.rotate(TEMP);
        float slope = v.y / v.x;
        world.setLine(9, slope, 0.5f * (1.0f - slope));

        // Keyboard actions
        vec2f camDir = camera.getDirection(); // It is used later anyways
        if(keyStates[SDL_SCANCODE_ESCAPE]) {
            runGame = false;
        }
        if(keyStates[SDL_SCANCODE_W]) {
            camera.changePosition(camDir * MOVE_SPEED * elapsedTime);
        }
        if(keyStates[SDL_SCANCODE_S]) {
            camera.changePosition(camDir * -1 * MOVE_SPEED * elapsedTime);
        }
        if(keyStates[SDL_SCANCODE_D]) {
            camera.changePosition(camDir.orthogonal() * MOVE_SPEED * elapsedTime);
        }
        if(keyStates[SDL_SCANCODE_A]) {
            camera.changePosition(camDir.orthogonal() * -1 * MOVE_SPEED * elapsedTime);
        }
        if(keyStates[SDL_SCANCODE_RIGHT]) {
            camera.changeDirection(TURN_SPEED * elapsedTime);
        }
        if(keyStates[SDL_SCANCODE_LEFT]) {
            camera.changeDirection(-1 * TURN_SPEED * elapsedTime);
        }
        // Draw the current frame
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        for(int column = 0; column < SCREEN_WIDTH; column += COLS_PER_RAY) {
            // Position of the ray on the camera plane, from -1 (left) to 1 (right)
            float cameraX = 2 * column / (float)SCREEN_WIDTH - 1;
            vec2f planeDir = camera.getPlane();
            // Not normalizing the ray direction allows for later use of "side-delta dist" trick proposed by
            // the lodev, personally I do not understand it fully so I decided to use point-line distance instead.
            vec2f rayDir = (camDir + planeDir * cameraX).normalized();
            vec2f camPos = camera.getPosition();
            // Send a ray, which will collect information about tiles it met during stepping,
            // then process gathered information.
            int hits = dda.sendRay(camPos, rayDir);
            bool missed = true; // Has a ray missed some tile line equation (has not hit its geometry)?
            int hitIndex = 0;
            while(missed && hitIndex != hits) {
                DDA_RayHitInfo hitInfo = dda.hits[hitIndex++];
                // The point of statement below is to first find exact point where ray hit the wall,
                // second is to calculate a distance of projection onto the camera plane.
                float perpHitDist;
                LineEquation wall = world.getLine(hitInfo.data);
                LineEquation ray;
                if(wall.data == -1) {
                    // No line is defined for the tile, assume it is a cube by default
                    perpHitDist = linePointDist(camera, hitInfo.point);
                    missed = false;
                } else {
                    // Line is defined, find the intersection point with the ray line equation,
                    // notice that Y-intercepts of both equations are relative to the hit tile
                    // integer position.
                    ray.slope = rayDir.x == 0 ? 1e30 : (rayDir.y / rayDir.x);
                    float rayIntX = 0; // Camera intercepts in both axes, IntX moves the line equation
                    float rayIntY = 0; // horizontally, and IntY vertically.
                    if(hitInfo.side) {
                        rayIntY = hitInfo.point.y - (int)hitInfo.point.y;
                        if(rayDir.x < 0) {
                            rayIntX = 1;
                        }
                    } else {
                        rayIntX = hitInfo.point.x - (int)hitInfo.point.x;
                        if(rayDir.y < 0) {
                            rayIntY = 1;
                        } 
                    }
                    ray.intercept = rayIntY - ray.slope * rayIntX; // Total intercept
                    // Finally, having both equations, obtain intersection point, then make it global
                    // and compute the point-line distance being the projected distance.
                    vec2f inter = wall.intersection(ray);
                    if(inter.x < 0 || inter.x > 1 || inter.y < 0 || inter.y > 1) {
                        missed = true;
                        continue;
                    } else {
                        vec2f tile = vec2f(
                            ensureInteger(hitInfo.point.x), // Ensuring integers prevents them from flipping suddenly,
                            ensureInteger(hitInfo.point.y)  // for example: it makes 5.9999 -> 6 and 6.0001 -> 6.
                        );
                        if(hitInfo.side) {
                            if(rayDir.x < 0)
                                tile.x -= 1;
                        } else {
                            if(rayDir.y < 0)
                                tile.y -= 1;
                        }
                        perpHitDist = linePointDist(camera, tile + inter);
                        missed = false;
                    }
                }

                // Find normal vector of the hit wall
                vec2f normal = (vec2f::RIGHT).rotate(-1 * atanf(1 / wall.slope * -1));
                if(wall.slope < 0) {
                    // Make it point outside from the player prespective even now
                    normal = normal.scale(-1);
                }
                // Choose a color for the wall
                uint8_t r, g, b;
                switch(hitInfo.data) {
                    case 1:
                        r = 255; g = 0; b = 0;
                        break;
                    case 2:
                        r = 0; g = 255; b = 0;
                        break;
                    case 3:
                        r = 0; g = 0; b = 255;
                        break;
                    default:
                        r = 0; g = 0; b = 0;
                        break;
                }
                if(hitInfo.data > 3) {
                    r = 255; g = 255; b = 0;
                }
                // Apply simple normal-based shading if possible, otherwise just side-based
                if(wall.data == -1) {
                    if(hitInfo.side) {
                        r *= 0.7;
                        g *= 0.7;
                        b *= 0.7;
                    }
                } else {
                    float dot = normal.dot(sunDir) * -1;
                    r *= dot;
                    g *= dot;
                    b *= dot;
                }
                SDL_SetRenderDrawColor(renderer, r, g, b, 255);

                // Display the ray by drawing an appropriate vertical strip, scaling is applied in
                // form of aspectRatio (to make everything have the same size across all resolutions),
                // and camera plane length (to make squares look always like squares).
                float scale = (1 / aspectRatio) * (1 / (2 * camera.getPlane().magnitude()));
                int lineHeight = (SCREEN_HEIGHT / perpHitDist) * scale;
                int drawStart = SCREEN_HEIGHT / 2 - lineHeight / 2;
                int drawEnd = SCREEN_HEIGHT / 2 + lineHeight / 2;
                drawStart = drawStart < 0 ? 0 : drawStart;
                drawEnd = drawEnd >= SCREEN_HEIGHT ? (SCREEN_HEIGHT - 1) : drawEnd;
                for(int i = 0; i < COLS_PER_RAY; i++) {
                    SDL_RenderDrawLine(renderer, column + i, drawStart, column + i, drawEnd);
                }

                #ifdef DEBUG_MODE
                // This section is for debugging-purposes only
                if(column == SCREEN_WIDTH / 2 && hitIndex == 1) {
                    system("clear");
                    printf("frame time: %f\n", elapsedTime);
                    // Draw a center cross with color being in contrast to the detected one
                    r ^= 0xFFFFFF;
                    g ^= 0xFFFFFF;
                    b ^= 0xFFFFFF;
                    SDL_SetRenderDrawColor(renderer, r, g, b, 255);
                    SDL_RenderDrawLine(renderer, column, 0, column, SCREEN_HEIGHT);
                }
                #endif
            }
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(frameDurationMs);
    }

    /********** TERMINATE **********/
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    SDL_Quit();
    return 0;
}

int ensureInteger(float n) {
    // This simple function kills all demons that output integer while it is not a one, for example
    // take number 4.999999(...), C++ tells me that it is 5, so I evaluate (int)5 and get 4 ... bruh!
    return (int)(n + EI_TOLERANCE) > (int)(n) ? ceilf(n) : floorf(n);
}

float linePointDist(const Camera& cam, const vec2f& point) {
    // Camera plane line equation: y = (pD.y/pD.x)*(x-cP.x)+cP.y; pD -> planeDir, cP -> camPos
    // Transform it to general form: 0 = slope*x - 1*y + cP.y-slope*cP.x
    // Coefficients: A=slope, B=-1, C=cP.y-slope*cP.x
    float A = cam.getPlane().y / cam.getPlane().x;
    float B = -1.0f;
    float C = cam.getPosition().y - cam.getPosition().x * A;
    // Distance from a point (x0, y0) to a line with coefficients A, B and C is given to
    // be abs(A*x0+B*y0+C)/sqrt(A^2+B^2), apply it.
    return std::abs(A * point.x + B * point.y + C) / sqrtf(A * A + B * B);
}
