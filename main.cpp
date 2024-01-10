
#include <iostream>
#include "include/engine.hpp"

using namespace rp;

int main() {

    Scene scene;
    // int line = scene.loadFromFile("local/debug.rps");
    // int line = scene.loadFromFile("local/generated.rps");
    int line = scene.loadFromFile("resources/my_world.rps");
    std::cout << "Loading plane error " << scene.getError() << " at line " << line << std::endl;
    if(scene.getError())
        return 1;
    scene.loadTexture("resources/steve.png");

    const float invSqrt2 = 1 / sqrt(2);
    const float moveSpeed = 2;
    const float fovAngle = M_PI / 2;
    float turnSpeed = M_PI * 0.66f;
    Camera camera(Vector2(5.5f, 5.5f), M_PI/2-0.001f, fovAngle);
    Engine engine(1000, 1000);
    bool lockCursor = false;

    engine.setCursorLock(lockCursor);
    engine.setCursorVisibility(!lockCursor);
    engine.setFrameRate(60);
    engine.setLightBehavior(true, 0);
    engine.setMainCamera(&camera);
    engine.setRenderFitMode(RenderFitMode::SQUARE);
    engine.setColumnsPerRay(2);
    engine.setRowsInterval(2);
    engine.getWalker()->setTargetScene(&scene);
    engine.getWalker()->setMaxTileDistance(21);
    
    SDL_SetWindowPosition(engine.getWindowHandle(), 0, 0);

    bool efFlying = true;
    bool efBillboard = false;
    bool isTransforming = false;

    float heightChange = 0.0f;
    float lastHC = 0.0f;
    
    while(engine.tick()) {

        // Flying illusion (by changing wall render heights)
        if(efFlying) {
            heightChange = 0.0f;
            if(engine.getKeyState(SDL_SCANCODE_UP) == KeyState::PRESS)
                heightChange -= engine.getElapsedTime();
            if(engine.getKeyState(SDL_SCANCODE_DOWN) == KeyState::PRESS)
                heightChange += engine.getElapsedTime();

            if(lastHC != heightChange) {
                const vector<int>* tileIds = scene.getTileIds();
                for(const int& tid : *tileIds) {
                    const vector<WallData>* tileWalls = scene.getTileWalls(tid);
                    for(int wid = 0; wid < tileWalls->size(); wid++) {
                        WallData wd = tileWalls->at(wid);
                        wd.hMin += heightChange;
                        wd.hMax += heightChange;
                        scene.setTileWall(tid, wid, wd);
                    }
                }

                engine.clear();
                engine.render();
                lastHC = heightChange;
            }
        }

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
                    0.0f,
                    1.0f,
                    scene.getTextureId("resources/steve.png"),
                    0
                )
            );
        }

        /********* BASIC PLAYER MECHANICS **********/

        if(isTransforming) {
            engine.clear();
            engine.render();
            
            isTransforming = false;
        }

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
            isTransforming = true;
        }

        // Camera rotation 
        if(lockCursor) {
            // Mouse based (for now kinda bugged)
            int currMouseX;
            SDL_GetMouseState(&currMouseX, NULL);
            int mouseDeltaX = currMouseX - engine.getScreenWidth() / 2;
            if(mouseDeltaX != 0) {
                camera.changeDirection(-1 * turnSpeed * mouseDeltaX * engine.getElapsedTime());
                isTransforming = true;
            }
        } else {
            // Keyboard based
            if(engine.getKeyState(SDL_SCANCODE_RIGHT) == KeyState::PRESS) {
                camera.changeDirection(-1 * turnSpeed * engine.getElapsedTime());
                isTransforming = true;
            }
            if(engine.getKeyState(SDL_SCANCODE_LEFT) == KeyState::PRESS) {
                camera.changeDirection(turnSpeed * engine.getElapsedTime());
                isTransforming = true;
            }
        }

    }

    std::cout << "Engine stopped with error code " << engine.getError() << std::endl;
    return 0;
}
