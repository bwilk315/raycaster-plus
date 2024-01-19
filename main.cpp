
#include <iostream>
#include <RPGE_engine.hpp>

using namespace rpge;

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
    engine.setColumnsPerRay (4);
    engine.setRowsInterval  (4);
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

        // Fake vertical movement
        float vChange = 0.0f;
        if(engine.getKeyState(SDL_SCANCODE_E) == KeyState::PRESS) vChange -= engine.getElapsedTime();
        if(engine.getKeyState(SDL_SCANCODE_Q) == KeyState::PRESS) vChange += engine.getElapsedTime();
        if(vChange != 0) {
            for(const int& tid : *scene.getTileIds()) {
                const vector<WallData>* data = scene.getTileWalls(tid);
                if(data->empty())
                    break;
                for(int wi = 0; wi < data->size(); wi++) {
                    WallData wd = data->at(wi);
                    wd.hMin += vChange;
                    wd.hMax += vChange;
                    scene.setTileWall(tid, wi, wd);
                }
            }
            isTransforming = true;
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
