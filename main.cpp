
#define DEBUG_MODE

#include <iostream>
#include <chrono>
#include <SDL2/SDL.h>
#include "include/math.hpp"
#include "include/print.hpp"

/********** RAYCASTER CONFIGURATION **********/
// Video settings
#define SCREEN_WIDTH 1280 // O(n)
#define SCREEN_HEIGHT 720 // O(1)
#define LOOP_FPS 100 // O(n)
#define COLS_PER_RAY 2 // O(n^2)
#define MAX_TILE_DIST 20 // O(n^2) (for detailed tiles)
// Player settings
#define MOVE_SPEED 3
#define FOV_ANGLE M_PI * 0.5f
#define TURN_SPEED M_PI * 0.04f  // Radians per unit
#define MOUSE_CONTROLS true
// World settings
#define SUN_ANGLE M_PI / 6

int ensureInteger(float n);
float linePointDist(const Camera& cam, const vec2f& point);

#define EI_TOLERANCE 0.00001f

const int halfWidth = SCREEN_WIDTH / 2;
const int halfHeight = SCREEN_HEIGHT / 2;
const float invSqrt2 = 1.0f / sqrtf(2.0f);

int main() {
    /********** SET UP **********/
    const vec2f sunDir = vec2f::RIGHT.rotate(-1 * SUN_ANGLE);
    const float aspectRatio = SCREEN_HEIGHT / (float)SCREEN_WIDTH;
    const int frameDurationMs = 1000 / LOOP_FPS;
    Plane world;
    int_pair error = world.load("generated.plane");
    if(error.second == Plane::E_PF_CLEAR) std::cout << "Plane file is correct\n";
    else std::cout << "Plane file error " << error.second << " at line " << error.first << std::endl;

    Camera camera = Camera(1.5f, 1.5f, FOV_ANGLE, 0);
    DDA_Algorithm dda = DDA_Algorithm(&world, MAX_TILE_DIST);
    SDL_Window* window;
    SDL_Renderer* renderer;
    std::chrono::time_point<std::chrono::system_clock> tpLastFrame, tpCurrFrame;
    std::chrono::duration<float> durDelta;
    int frame = 0;
    int currMouseX;
    bool keyStates[SDL_NUM_SCANCODES];
    bool runGame = true;

    /********** INITIALIZATION **********/
    SDL_InitSubSystem(SDL_INIT_VIDEO);
    SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN, &window, &renderer);
    SDL_SetWindowTitle(window, "Raycaster");
    #if MOUSE_CONTROLS
    SDL_ShowCursor(SDL_DISABLE);
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_RaiseWindow(window); // focus
    #endif
    memset(keyStates, false, SDL_NUM_SCANCODES);
    tpLastFrame = std::chrono::system_clock::now();
    
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

        bool spriteBeta = true;
        if(spriteBeta) {
            /** THE IDEA BELOW CAN BE USED IN THE FURUTRE TO DO SPRITE STUFF **/
            // Plane is always looking at the player, appears flat
            vec2f ort = camera.getDirection().orthogonal();
            float slope = ort.y / ort.x;
            float intercept = 0.5f * (1.0f - slope);
            float startX, endX;
            // To know where to start and end hitting arguments, so the plane is always the same size,
            // we need to find intersection points of the line defined above and a special circle.
            // line: y = <slope> * x + <intercept>
            // circle: (x - 0.5)^2 + (y - 0.5)^2 = 0.5^2 (circle of radius 0.5 centered at [0.5, 0.5])
            float a = slope * slope + 1;
            float b = 2 * slope * intercept - slope - 1.0f;
            float c = intercept * intercept - intercept + 0.25f;
            float delta = b * b - 4.0f * a * c;
            float x1 = (-1 * b - sqrtf(delta)) / (2.0f * a);
            float x2 = (-1 * b + sqrtf(delta)) / (2.0f * a);
            world.setTileWall(
                6,
                0,
                LineEquation(slope, intercept, x1 < x2 ? x1 : x2, x1 > x2 ? x1 : x2),
                { 128, 128, 128 }
            );
        }

        // Save often-used information
        vec2f camDir = camera.getDirection();
        vec2f planeDir = camera.getPlane();
        vec2f camPos = camera.getPosition();

        // Camera movement
        float moveInputX = 0;
        float moveInputY = 0;
        if(keyStates[SDL_SCANCODE_ESCAPE]) runGame = false;
        if(keyStates[SDL_SCANCODE_W]) moveInputY += 1;
        if(keyStates[SDL_SCANCODE_S]) moveInputY += -1;
        if(keyStates[SDL_SCANCODE_D]) moveInputX += 1;
        if(keyStates[SDL_SCANCODE_A]) moveInputX += -1;
        if(moveInputX || moveInputY) {
            vec2f posChange = camDir.orthogonal() * moveInputX + camDir * moveInputY;
            camera.changePosition(
                posChange * MOVE_SPEED * elapsedTime * (moveInputX && moveInputY ? invSqrt2 : 1.0f)
            );
        }

        // Camera rotation
        #if MOUSE_CONTROLS
        SDL_GetMouseState(&currMouseX, NULL);
        int mouseDeltaX = currMouseX - halfWidth;
        if(mouseDeltaX != 0) {
            camera.changeDirection(TURN_SPEED * mouseDeltaX * elapsedTime);
            SDL_WarpMouseInWindow(window, halfWidth, halfHeight);
        }
        #else
        if(keyStates[SDL_SCANCODE_RIGHT]) {
            camera.changeDirection(TURN_SPEED * elapsedTime);
        }
        if(keyStates[SDL_SCANCODE_LEFT]) {
            camera.changeDirection(-1 * TURN_SPEED * elapsedTime);
        }
        #endif

        // Draw the current frame
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        for(int column = 0; column < SCREEN_WIDTH; column += COLS_PER_RAY) {
            // Position of the ray on the camera plane, from -1 (left) to 1 (right)
            float cameraX = 2 * column / (float)SCREEN_WIDTH - 1;
            vec2f rayDir = (camDir + planeDir * cameraX).normalized();
            // Find line equation describing the ray walk
            LineEquation rayLine;
            // Stays constant during the frame
            rayLine.slope = (rayDir.x == 0) ? (LineEquation::MAX_SLOPE) : (rayDir.y / rayDir.x);
            
            // Perform the ray stepping, at each step hit tile is examined and if some wall is hit,
            // its information is stored in a dedicated structure. 
            dda.init(camPos, rayDir);
            bool wasWallHit = false;
            int wallTileData;   // Number indicating tile properties
            float wallDistance; // Distance from a wall
            vec2f wallNormal;   // Vector telling in which direction a wall is facing
            SDL_Color wallColor;
            while(true) {
                RayHitInfo rayInfo = dda.next();
                // Exit the loop in appropriate moment
                if( (wasWallHit) ||
                    (dda.rayFlag & DDA_Algorithm::RF_TOO_FAR) ||
                    (dda.rayFlag & DDA_Algorithm::RF_OUTSIDE) ) {
                    break;
                // If a ray touched an air tile (with data of 0), skip
                } else if(!(dda.rayFlag & DDA_Algorithm::RF_HIT))
                    continue;

                int tileData = world.getTileData(rayInfo.tile.x, rayInfo.tile.y);
                bool fromSide = dda.rayFlag & DDA_Algorithm::RF_SIDE;
                float minLineDist = 1e30;

                for(const WallInfo& wallInfo : world.getTileWalls(tileData)) {
                    const LineEquation* line = &wallInfo.line; // For convienence
                    // It will not be seen anyway
                    if(line->domainStart == line->domainEnd)
                        continue; 
                    // Update the ray line intercept
                    float rayIntX; // Camera intercepts in both axes, rayIntX moves the line equation
                    float rayIntY; // horizontally, and ray IntY vertically.
                    if(fromSide) {
                        rayIntY = rayInfo.point.y - (int)rayInfo.point.y;
                        rayIntX = (rayDir.x < 0) ? (1) : (0);
                    } else {
                        rayIntX = rayInfo.point.x - (int)rayInfo.point.x;
                        rayIntY = (rayDir.y < 0) ? (1) : (0);
                    }
                    rayLine.intercept = rayIntY - rayLine.slope * rayIntX; // Total intercept

                    // Find intersection point of line defining wall geometry and the ray line.
                    vec2f inter = line->intersection(rayLine);
                    if((inter.x < 0 || inter.x > 1 || inter.y < 0 || inter.y > 1) ||
                       (inter.x < line->domainStart || inter.x > line->domainEnd)) {
                        // Intersection point is out of tile or domain bounds, skip it
                        continue;
                    } else {
                        // Intersection point is defined in specified bounds, process it
                        vec2f tilePos = vec2f(
                            ensureInteger(rayInfo.point.x), // Ensuring integers prevents them from flipping suddenly,
                            ensureInteger(rayInfo.point.y)  // for example: it makes 5.9999 -> 6 and 6.0001 -> 6.
                        );
                        tilePos.x -= ( fromSide && rayDir.x < 0) ? (1) : (0);
                        tilePos.y -= (!fromSide && rayDir.y < 0) ? (1) : (0);
                        float pointDist = linePointDist(camera, tilePos + inter);
                        // The closest line equation defines the wall geometry
                        if(pointDist < minLineDist) {
                            // Find normal vector of the hit wall
                            vec2f normal = (vec2f::RIGHT).rotate(-1 * atanf(1 / line->slope * -1));
                            if(line->slope < 0)
                                normal = normal.scale(-1); // Make it consistent
                            // Save (for now) the nearest wall information
                            wallTileData = tileData;
                            wallDistance = pointDist;
                            wallNormal = normal;
                            wallColor = wallInfo.color;
                            minLineDist = pointDist;
                        }
                        wasWallHit = true;
                    }
                }
            }
            if(!wasWallHit) continue; // Ray hit nothing

            // Draw the wall using information computed above
            if(wallTileData != 6) {  // This condition is temporary, it turns of shading for "sprite"
                // Apply simple normal-based shading
                float bn = -1 * wallNormal.dot(sunDir);
                wallColor.r *= bn;
                wallColor.g *= bn;
                wallColor.b *= bn;
            }
            SDL_SetRenderDrawColor(renderer, wallColor.r, wallColor.g, wallColor.b, 255);
            // Display the ray by drawing an appropriate vertical strip, scaling is applied in
            // form of aspect ratio (to make everything have the same size across all resolutions),
            // and camera plane length (to make squares look always like squares).
            float scale = (1 / aspectRatio) * (1 / (2 * camera.getPlane().magnitude()));
            int lineHeight = (SCREEN_HEIGHT / wallDistance) * scale;
            int drawStart = SCREEN_HEIGHT / 2 - lineHeight / 2;
            int drawEnd = SCREEN_HEIGHT / 2 + lineHeight / 2;
            drawStart = drawStart < 0 ? 0 : drawStart;
            drawEnd = drawEnd >= SCREEN_HEIGHT ? (SCREEN_HEIGHT - 1) : drawEnd;
            for(int i = 0; i < COLS_PER_RAY; i++) // Take columns per row into account
                SDL_RenderDrawLine(renderer, column + i, drawStart, column + i, drawEnd);

            #ifdef DEBUG_MODE
            if(std::abs(column - SCREEN_WIDTH / 2) < 2) {
                if(frame % (LOOP_FPS/4) == 0) {
                    system("clear");
                    std::cout << "fps: " << roundf(1.0f / elapsedTime) << std::endl;
                }
                // Draw a center cross with color being in contrast to the detected one
                wallColor.r ^= 0xFFFFFF;
                wallColor.g ^= 0xFFFFFF;
                wallColor.b ^= 0xFFFFFF;
                SDL_SetRenderDrawColor(renderer, wallColor.r, wallColor.g, wallColor.b, 255);
                SDL_RenderDrawLine(renderer, column, 0, column, SCREEN_HEIGHT);
            }
            #endif
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(frameDurationMs);
        frame++;
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
