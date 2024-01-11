
#include <iostream>
#include "include/engine.hpp"

using namespace rp;

struct Player {
    float moveSpeed;
    float turnSpeed;
    Camera camera;
};

int main() {

    Engine engine(1000, 1000);
    Scene  scene (engine.getColorFormat());
    Player player;
    player.moveSpeed = 2;
    player.turnSpeed = M_PI * 0.66f;
    player.camera    = Camera(Vector2(1.5f, 1.5f), M_PI_2, M_PI_2);

    int line = scene.loadFromFile("resources/my_world.rps");
    if(scene.getError()) {
        std::cout << "Loading plane error " << scene.getError() << " at line " << line << std::endl;
        return 1;
    }

    engine.setFrameRate     (60);
    engine.setLightBehavior (true, 0);
    engine.setMainCamera    (&player.camera);
    engine.setRenderFitMode (RenderFitMode::SQUARE);
    engine.setColumnsPerRay (2);
    engine.setRowsInterval  (2);
    engine.getWalker()->setTargetScene(&scene);
    engine.getWalker()->setMaxTileDistance(21);
    
    SDL_SetWindowPosition(engine.getWindowHandle(), 0, 0);

    bool isTransforming = false;

    while(engine.tick()) {

        // Game management
        if(isTransforming) {
            engine.clear();
            engine.render();
            
            isTransforming = false;
        }
        if(engine.getKeyState(SDL_SCANCODE_ESCAPE) == KeyState::UP) {
            engine.stop();
        }

        // Camera movement
        Vector2 moveInput;
        if(engine.getKeyState(SDL_SCANCODE_W) == KeyState::PRESS) moveInput.y += 1;
        if(engine.getKeyState(SDL_SCANCODE_S) == KeyState::PRESS) moveInput.y += -1;
        if(engine.getKeyState(SDL_SCANCODE_D) == KeyState::PRESS) moveInput.x += 1;
        if(engine.getKeyState(SDL_SCANCODE_A) == KeyState::PRESS) moveInput.x += -1;
        float mag = (moveInput.x != 0 && moveInput.y != 0) ? INV_SQRT2 : moveInput.magnitude();
        if(mag != 0) {
            Vector2 camDir = player.camera.getDirection();
            Vector2 posChange = camDir.orthogonal() * moveInput.x + camDir * moveInput.y;
            player.camera.changePosition(
                posChange * player.moveSpeed * engine.getElapsedTime() * mag
            );
            isTransforming = true;
        }

        // Keyboard-based camera rotation 
        if(engine.getKeyState(SDL_SCANCODE_RIGHT) == KeyState::PRESS) {
            player.camera.changeDirection(-1 * player.turnSpeed * engine.getElapsedTime());
            isTransforming = true;
        }
        if(engine.getKeyState(SDL_SCANCODE_LEFT) == KeyState::PRESS) {
            player.camera.changeDirection(player.turnSpeed * engine.getElapsedTime());
            isTransforming = true;
        }

    }

    if(engine.getError()) {
        std::cout << "Engine stopped with error code " << engine.getError() << std::endl;
        return 1;
    }
    
    return 0;

}
