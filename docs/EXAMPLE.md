
# RPGE Example

The suggested code structure contains one loop which iteration is dependant on the result of ticking function `Engine.tick()`, which is used to perform all engine-related things. The *hello-worldish* code snipped below can be used to determine if you installed the library correctly, you should be able to move and look around the loaded scene.
```cpp
#include <iostream>
#include <RPGE_engine.hpp>

#define SCREEN_WIDTH  1000
#define SCREEN_HEIGHT 1000
#define MOVE_SPEED 5
#define TURN_SPEED (M_PI * 2/3)

using namespace rpge;

int main()
{
    Engine eng = Engine(SCREEN_WIDTH, SCREEN_HEIGHT);
    Scene sc   = Scene(eng.getRendererHandle());

    // Try to load a scene file and check for errors
    int errLine = sc.loadFromFile("path/to/scene/file.rps");
    int errCode = sc.getError();
    if(errCode)
    {
        std::cout << "Scene load error " << errCode << " (line " << errLine << ")\n";
        return errCode;
    }

    // Create the main camera
    Camera cam = Camera(Vector2(1.0f, 1.0f), 0.0f, M_PI_2);

    // Set up the engine
    eng.setClearColor(70, 100, 160);
    eng.setCursorLock(false);
    eng.setColumnsPerRay(2);
    eng.setCursorVisibility(true);
    eng.setFrameRate(60);
    eng.setLightBehavior(true, 0.0f);
    eng.setMainCamera(&cam);
    eng.setClearArea({ 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT });
    eng.setRenderArea({ 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT });
    eng.setRowsInterval(2);

    // Set up the walker (which sends rays)
    eng.getWalker()->setTargetScene(&sc);
    eng.getWalker()->setMaxTileDistance(32);

    // Let the engine do its job until stopped by error or method
    while(eng.tick())
    {
        // AKA delta time
        float timeElapsed = eng.getElapsedTime();

        // Game management
        if(eng.getKeyState(SDL_SCANCODE_ESCAPE) == KeyState::UP)
            eng.stop();

        // Simple WASD keys camera movement
        Vector2 delta;
        if(eng.getKeyState(SDL_SCANCODE_W) == KeyState::PRESS) delta.y += 1.0f;
        if(eng.getKeyState(SDL_SCANCODE_S) == KeyState::PRESS) delta.y -= 1.0f;
        if(eng.getKeyState(SDL_SCANCODE_D) == KeyState::PRESS) delta.x += 1.0f;
        if(eng.getKeyState(SDL_SCANCODE_A) == KeyState::PRESS) delta.x -= 1.0f;
        if(delta.x != 0 || delta.y != 0)
        {
            Vector2 camDir = cam.getDirection();
            Vector2 change = camDir.orthogonal() * delta.x + camDir * delta.y;
            cam.changePosition(change * MOVE_SPEED * timeElapsed);
        }

        // Simple RL arrow camera rotation
        if(eng.getKeyState(SDL_SCANCODE_RIGHT))
            cam.changeDirection(-1 * TURN_SPEED * timeElapsed);
        if(eng.getKeyState(SDL_SCANCODE_LEFT))
            cam.changeDirection(TURN_SPEED * timeElapsed);
    
        // Render the current frame
        eng.clear();
        eng.render();
    }

    // Check for engine errors, they might stopped it!
    errCode = eng.getError();
    if(errCode)
        std::cout << "Engine stopped with error " << errCode << std::endl;
    
    return errCode;
}
```
