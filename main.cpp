
//#define DEBUG_MODE

#include <iostream>
#include <chrono>
#include <set>
#include <SDL2/SDL.h>
#include "include/math.hpp"
#include "include/print.hpp"

#define EI_TOLERANCE 0.00001f

/********** RAYCASTER CONFIGURATION **********/
// Video settings
#define SCREEN_WIDTH 1200
#define SCREEN_HEIGHT 700
#define LOOP_FPS 60
#define COLS_PER_RAY 4
#define MAX_TILE_DIST 30
// Player settings
#define MOVE_SPEED 3
#define FOV_ANGLE M_PI / 2
#define TURN_SPEED M_PI / 1.5
// World settings
#define SUN_ANGLE M_PI / 6

int ensureInteger(float n);
float linePointDist(const Camera& cam, const vec2f& point);

int main() {    
    /********** SET UP **********/
    const vec2f sunDir = vec2f::RIGHT.rotate(-1 * SUN_ANGLE);
    const float aspectRatio = SCREEN_HEIGHT / (float)SCREEN_WIDTH;
    const int frameDurationMs = 1e3 / LOOP_FPS;
    Plane world;
    int_pair error = world.load("world.plane");
    std::cout << "Plane file ERROR " << error.second << " at line " << error.first << std::endl;

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
        world.setLine(9, 0, slope, 0.5f * (1.0f - slope));

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

            //TODO: CREATE ORDER-BASED DRAWING BY TAKING LOCAL INTERSECTION DISTANCE AS METERISTIC
            // Create walls information given rays information
            int hitCount = dda.sendRay(camPos, rayDir);
            std::set<WallStripeInfo, std::greater<WallStripeInfo>> walls;
            for(int h = 0; h < hitCount; h++) {
                RayHitInfo hi = dda.hits[h];
                int tileId = world.getTile(hi.tile.x, hi.tile.y);

                // Find line equation describing the ray walk
                LineEquation rayLine;
                rayLine.slope = rayDir.x == 0 ? 1e30 : (rayDir.y / rayDir.x);
                float rayIntX = 0; // Camera intercepts in both axes, IntX moves the line equation
                float rayIntY = 0; // horizontally, and IntY vertically.
                if(hi.side) {
                    rayIntY = hi.point.y - (int)hi.point.y;
                    if(rayDir.x < 0) {
                        rayIntX = 1;
                    }
                } else {
                    rayIntX = hi.point.x - (int)hi.point.x;
                    if(rayDir.y < 0) {
                        rayIntY = 1;
                    } 
                }
                rayLine.intercept = rayIntY - rayLine.slope * rayIntX; // Total intercept

                bool isAnyLine = false;
                bool missedGeometry = true; // Has a ray missed some tile (has not hit its geometry)?
                for(const LineEquation& le : world.getLines(tileId)) {
                    // WARNING! Skip flat lines, which indicate that you want to use cubes instead
                    if(le.slope == 0 && le.intercept == 0) {
                        walls.insert({
                            tileId,
                            le.id,
                            linePointDist(camera, hi.point),
                            hi.side ? (vec2f::RIGHT * (rayDir.x < 0 ? 1 : -1)) : (vec2f::UP * (rayDir.y < 0 ? 1 : -1))
                        });
                        missedGeometry = false;
                        break;
                    }

                    isAnyLine = true;
                    // Find normal vector of the hit wall
                    vec2f normal = (vec2f::RIGHT).rotate(-1 * atanf(1 / le.slope * -1));
                    if(le.slope < 0)
                        normal = normal.scale(-1); // Make it consistent
            
                    // Line is defined, find the intersection point with the ray line equation,
                    // notice that Y-intercepts of both equations are relative to the hit tile
                    // integer position.
                    // Finally, having both equations, obtain intersection point, then make it global
                    // and compute the point-line distance being the projected distance.
                    vec2f inter = le.intersection(rayLine);
                    if(inter.x < 0 || inter.x > 1 || inter.y < 0 || inter.y > 1) {
                        missedGeometry = true;
                        continue; // Skip wall which is not hit
                    } else {
                        vec2f tile = vec2f(
                            ensureInteger(hi.point.x), // Ensuring integers prevents them from flipping suddenly,
                            ensureInteger(hi.point.y)  // for example: it makes 5.9999 -> 6 and 6.0001 -> 6.
                        );
                        if(hi.side) {
                            if(rayDir.x < 0)
                                tile.x -= 1;
                        } else {
                            if(rayDir.y < 0)
                                tile.y -= 1;
                        }
                        missedGeometry = false;
                        // Save the wall properties
                        walls.insert({
                            tileId,
                            le.id,
                            linePointDist(camera, tile + inter),
                            normal,
                        });
                    }
                }
                if(!missedGeometry)
                    break;
            }

            // DRAW STRIPES
            for(const WallStripeInfo& wall : walls) {
                // Choose a color for the wall
                SDL_Color color;
                color = world.getLineColor(wall.lineId);
                
                // Apply simple normal-based brightness if possible, otherwise just side-based
                float bn = -1 * wall.normal.dot(sunDir);
                color.r *= bn;
                color.g *= bn;
                color.b *= bn;
                SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);

                // Display the ray by drawing an appropriate vertical strip, scaling is applied in
                // form of aspectRatio (to make everything have the same size across all resolutions),
                // and camera plane length (to make squares look always like squares).
                float scale = (1 / aspectRatio) * (1 / (2 * camera.getPlane().magnitude()));
                int lineHeight = (SCREEN_HEIGHT / wall.distance) * scale;
                int drawStart = SCREEN_HEIGHT / 2 - lineHeight / 2;
                int drawEnd = SCREEN_HEIGHT / 2 + lineHeight / 2;
                drawStart = drawStart < 0 ? 0 : drawStart;
                drawEnd = drawEnd >= SCREEN_HEIGHT ? (SCREEN_HEIGHT - 1) : drawEnd;
                for(int i = 0; i < COLS_PER_RAY; i++) {
                    SDL_RenderDrawLine(renderer, column + i, drawStart, column + i, drawEnd);
                }

                #ifdef DEBUG_MODE
                // This section is for debugging-purposes only
                if(column == SCREEN_WIDTH / 2) {
                    system("clear");
                    //printf("frame time: %f\n", elapsedTime);
                    //printVector(wall.normal);
                    // Draw a center cross with color being in contrast to the detected one
                    color.r ^= 0xFFFFFF;
                    color.g ^= 0xFFFFFF;
                    color.b ^= 0xFFFFFF;
                    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);
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