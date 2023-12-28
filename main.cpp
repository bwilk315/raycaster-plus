
#include <iostream>
#include "include/engine.hpp"

// Video settings
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720
#define LOOP_FPS 100
#define COLS_PER_RAY 4
#define MAX_TILE_DIST 16
// Player settings
#define MOVE_SPEED 2
#define FOV_ANGLE M_PI * 0.5f
#define TURN_SPEED M_PI * 0.04f

using namespace rp;

int main() {
    Scene scene;
    int_pair error = scene.loadFromFile("generated.plane");
    std::cout << error.first << ", " << error.second << std::endl;
    if(error.second != Scene::E_CLEAR)
        return 1;

    Camera camera(Vector2(3.5f, 3.5f), 0, FOV_ANGLE);
    Engine engine(SCREEN_WIDTH, SCREEN_HEIGHT);
    bool lockCursor = true;

    engine.setFrameRate(LOOP_FPS);
    engine.setColumnsPerRay(COLS_PER_RAY);
    engine.getWalker()->setTargetScene(&scene);
    engine.getWalker()->setMaxTileDistance(MAX_TILE_DIST);
    engine.setMainCamera(&camera);
    engine.setRenderFitMode(RenderFitMode::TRIM_SCREEN);
    engine.setCursorLock(lockCursor);
    engine.setCursorVisibility(!lockCursor);
    engine.setLightBehavior(true, M_PI / 4);
    camera.setDirection(0);

    SDL_SetWindowPosition(engine.getWindowHandle(), 0, 0);

    bool efSunCycle = false;
    bool efBillboard = false;
    bool isTrimMode = true;
    float lightAngle = 0;


    while(engine.tick()) {
        
        /********** EXPERIMENTAL FEATURES **********/

        // Dynamic render fit mode
        if(engine.getKeyState(SDL_SCANCODE_0) == KeyState::DOWN) {
            isTrimMode = !isTrimMode;
            engine.setRenderFitMode(isTrimMode ? RenderFitMode::TRIM_SCREEN : RenderFitMode::CHANGE_FOV);
        }

        // Sun-cycle
        if(efSunCycle) {
            engine.setLightBehavior(true, lightAngle);
            lightAngle += engine.getElapsedTime() / 10;
        }

        // Billboard
        if(efBillboard) {
            // Plane is always looking at the player, appears flat
            Vector2 ort = camera.getDirection().orthogonal();
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
            engine.getWalker()->getTargetScene()->setTileWall(
                6,
                0,
                Wall(
                    LineEquation(slope, intercept, x1 < x2 ? x1 : x2, x1 > x2 ? x1 : x2),
                    { 255, 128, 64}
                )
            );
        }

        /********* BASIC PLAYER MECHANICS **********/

        // Game management
        if(engine.getKeyState(SDL_SCANCODE_ESCAPE) == KeyState::DOWN) {
            engine.stop();
        }
        if(engine.getKeyState(SDL_SCANCODE_C) == KeyState::DOWN) {
            lockCursor = !lockCursor;
            engine.setCursorLock(lockCursor);
            engine.setCursorVisibility(!lockCursor);
        }

        // Camera movement
        Vector2 moveInput;
        if(engine.getKeyState(SDL_SCANCODE_W) == KeyState::PRESS) moveInput.y += 1;
        if(engine.getKeyState(SDL_SCANCODE_S) == KeyState::PRESS) moveInput.y += -1;
        if(engine.getKeyState(SDL_SCANCODE_D) == KeyState::PRESS) moveInput.x += 1;
        if(engine.getKeyState(SDL_SCANCODE_A) == KeyState::PRESS) moveInput.x += -1;
        float mag = moveInput.normalized().magnitude();
        if(mag != 0) {
            Vector2 camDir = camera.getDirection();
            Vector2 posChange = camDir.orthogonal() * moveInput.x + camDir * moveInput.y;
            camera.changePosition(
                posChange * MOVE_SPEED * engine.getElapsedTime() * mag
            );
        }

        // Camera rotation
        if(lockCursor) {
            // Mouse based
            int currMouseX;
            SDL_GetMouseState(&currMouseX, NULL);
            int mouseDeltaX = currMouseX - SCREEN_WIDTH / 2;
            if(mouseDeltaX != 0)
                camera.changeDirection(-1 * TURN_SPEED * mouseDeltaX * engine.getElapsedTime());
        } else {
            // Keyboard based
            if(engine.getKeyState(SDL_SCANCODE_RIGHT) == KeyState::PRESS) {
                camera.changeDirection(-1 * TURN_SPEED * engine.getElapsedTime());
            }
            if(engine.getKeyState(SDL_SCANCODE_LEFT) == KeyState::PRESS) {
                camera.changeDirection(TURN_SPEED * engine.getElapsedTime());
            }
        }
    }

    return 0;
}
