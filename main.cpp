
#include <iostream>
#include "include/engine.hpp"

using namespace rp;

int main() {
    Scene scene;
    int line = scene.loadFromFile("resources/my_world.rps");
    std::cout << scene.getError() << " at " << line << std::endl;
    if(scene.getError())
        return 1;
    scene.loadTexture("resources/steve.png");

    const float invSqrt2 = 1 / sqrt(2);
    const float moveSpeed = 2;
    const float fovAngle = M_PI / 2;
    float turnSpeed = M_PI * 0.66f;
    Camera camera(Vector2(1.5f, 1.5f), M_PI/2, fovAngle);
    Engine engine(1000, 1000);
    bool lockCursor = false;

    engine.setCursorLock(lockCursor);
    engine.setCursorVisibility(!lockCursor);
    engine.setColumnsPerRay(4);
    engine.setFrameRate(60);
    engine.setLightBehavior(true, 0);
    engine.setMainCamera(&camera);
    engine.setRowsInterval(4);
    engine.setWindowResize(true);
    engine.setRenderFitMode(RenderFitMode::SQUARE);
    engine.getWalker()->setTargetScene(&scene);
    engine.getWalker()->setMaxTileDistance(21);
    
    SDL_SetWindowPosition(engine.getWindowHandle(), 0, 0);

    bool efBillboard = true;
    while(engine.tick()) {

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
            scene.setTileWall(
                10,
                0,
                WallData(
                    LinearFunc(
                        slope,
                        intercept,
                        x1 < x2 ? x1 : x2,
                        x1 > x2 ? x1 : x2
                    ),
                    encodeRGBA(255, 128, 64, 255),
                    0,
                    1,
                    scene.getTextureId("resources/steve.png"),
                    0
                )
            );
        }

        /********* BASIC PLAYER MECHANICS **********/

        // Game management
        if(engine.getKeyState(SDL_SCANCODE_ESCAPE) == KeyState::UP) {
            engine.stop();
        }
        if(engine.getKeyState(SDL_SCANCODE_C) == KeyState::UP) {
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
        float mag = (moveInput.x != 0 && moveInput.y != 0) ? invSqrt2 : moveInput.magnitude();
        if(mag != 0) {
            Vector2 camDir = camera.getDirection();
            Vector2 posChange = camDir.orthogonal() * moveInput.x + camDir * moveInput.y;
            camera.changePosition(
                posChange * moveSpeed * engine.getElapsedTime() * mag
            );
            engine.requestRedraw();
        }

        // Camera rotation 
        if(lockCursor) {
            // Mouse based (for now kinda bugged)
            int currMouseX;
            SDL_GetMouseState(&currMouseX, NULL);
            int mouseDeltaX = currMouseX - engine.getScreenWidth() / 2;
            if(mouseDeltaX != 0) {
                camera.changeDirection(-1 * turnSpeed * mouseDeltaX * engine.getElapsedTime());
                engine.requestRedraw();
            }
        } else {
            // Keyboard based
            if(engine.getKeyState(SDL_SCANCODE_RIGHT) == KeyState::PRESS) {
                camera.changeDirection(-1 * turnSpeed * engine.getElapsedTime());
                engine.requestRedraw();
            }
            if(engine.getKeyState(SDL_SCANCODE_LEFT) == KeyState::PRESS) {
                camera.changeDirection(turnSpeed * engine.getElapsedTime());
                engine.requestRedraw();
            }
        }

    }

    return 0;
}
