
//#define DEBUG

#include "../include/engine.hpp"

#ifdef DEBUG
#include <iostream>
#endif

namespace rp {
    #ifdef DEBUG
    using ::std::cout;
    #endif

    Engine::Engine(int screenWidth, int screenHeight) {
        this->run = true;
        this->isCursorLocked = false;
        this->iColumnsPerRay = 4;
        this->iScreenWidth = screenWidth;
        this->iScreenHeight = screenHeight;
        this->fAspectRatio = screenHeight / (float)screenWidth;
        this->fMaxTileDist = 16.0f;
        this->frameIndex = 0;
        this->tpLast = system_clock::now();
        this->keyStates = map<int, KeyState>();
        this->walker = new DDA();
        setFrameRate(60);

        SDL_InitSubSystem(SDL_INIT_VIDEO);
        SDL_CreateWindowAndRenderer(screenWidth, screenHeight, SDL_WINDOW_SHOWN, &this->sdlWindow, &this->sdlRenderer);
        SDL_SetWindowTitle(this->sdlWindow, "Raycaster Plus Engine");
    }
    Engine::~Engine() {
        SDL_DestroyWindow(sdlWindow);
        SDL_DestroyRenderer(sdlRenderer);
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        SDL_Quit();
        if(walker != nullptr)
            delete walker;
    }
    void Engine::stop() {
        run = false;
    }
    void Engine::setCursorLock(bool locked) {
        isCursorLocked = locked;
    }
    void Engine::setCursorVisibility(bool visible) {
        SDL_ShowCursor(visible ? SDL_ENABLE : SDL_DISABLE);
    }
    void Engine::setColumnsPerRay(int columns) {
        iColumnsPerRay = columns;
    }
    void Engine::setFrameRate(int framesPerSecond) {
        msFrameDuration = 1000 / framesPerSecond;
    }
    void Engine::setMainCamera(const Camera* camera) {
        mainCamera = camera;
    }
    int Engine::getScreenWidth() const {
        return iScreenWidth;
    }
    int Engine::getScreenHeight() const {
        return iScreenHeight;
    }
    float Engine::getElapsedTime() const {
        return elapsedTime.count();
    }
    KeyState Engine::getKeyState(int scanCode) const {
        return keyStates.count(scanCode) == 0 ? KeyState::NONE : keyStates.at(scanCode);
    }
    DDA* Engine::getWalker() {
        return walker;
    }
    SDL_Window* Engine::getWindowHandle() {
        return sdlWindow;
    }
    SDL_Renderer* Engine::getRendererHandle() {
        return sdlRenderer;
    }
    bool Engine::tick() {
        // Compute time elapsed since the last method call
        time_point<system_clock> tpCurrent = system_clock::now();
        elapsedTime = tpCurrent - tpLast;
        tpLast = tpCurrent;

        // Complete the key states
        auto ksCopy = keyStates; // To avoid iteration through dynamicly-sized map
        for(auto& p : ksCopy) {
            if(p.second == KeyState::DOWN) {
                keyStates.at(p.first) = KeyState::PRESS;
            }
            else if(p.second == KeyState::UP) {
                keyStates.erase(p.first);
            }
        }

        // Interpret SDL events if there are any
        SDL_Event event;
        while(SDL_PollEvent(&event)) {
            int sc;
            switch(event.type) {
                case SDL_QUIT:
                    run = false;
                    break;
                case SDL_KEYDOWN:
                    sc = event.key.keysym.scancode;
                    if(keyStates.count(sc) == 0)
                        keyStates.insert(pair<int, KeyState>(sc, KeyState::DOWN));
                    break;
                case SDL_KEYUP:
                    sc = event.key.keysym.scancode;
                    if(keyStates.count(sc) != 0)
                        keyStates.at(sc) = KeyState::UP;
                    break;
            }
        }

        // Save often-used information
        Vector2 camDir = mainCamera->getDirection();
        Vector2 planeDir = mainCamera->getPlane();
        Vector2 camPos = mainCamera->getPosition();

        // Draw the current frame
        SDL_SetRenderDrawColor(sdlRenderer, 0, 0, 0, 255);
        SDL_RenderClear(sdlRenderer);

        for(int column = 0; column < iScreenWidth; column += iColumnsPerRay) {
            // Position of the ray on the camera plane, from -1 (left) to 1 (right)
            float cameraX = 2 * column / (float)iScreenWidth - 1;
            Vector2 rayDir = (camDir + planeDir * cameraX).normalized();
            // Find line equation describing the ray walk
            LineEquation rayLine;
            // Stays constant during the frame
            rayLine.slope = (rayDir.x == 0) ? (LineEquation::MAX_SLOPE) : (rayDir.y / rayDir.x);

            // Perform the ray stepping, at each step hit tile is examined and if some wall is hit,
            // its information is stored in a dedicated structure. 
            walker->init(camPos, rayDir);
            bool wasWallHit = false;
            int wallTileData;   // Number indicating tile properties
            float wallDistance; // Distance from a wall
            Vector2 wallNormal;   // Vector telling in which direction a wall is facing
            SDL_Color wallColor;

            while(true) {
                RayHitInfo rayInfo = walker->next();
                // Exit the loop in appropriate moment
                if( (wasWallHit) ||
                    (walker->rayFlag & DDA::RF_TOO_FAR) ||
                    (walker->rayFlag & DDA::RF_OUTSIDE) ) {
                    break;
                // If a ray touched an air tile (with data of 0), skip
                } else if(!(walker->rayFlag & DDA::RF_HIT))
                    continue;

                int_pair p = walker->getTargetScene()->getTileData(rayInfo.tile.x, rayInfo.tile.y);
                // std::cout << "us\n";
                if(p.second != Scene::E_CLEAR)
                    break;
                
                int tileData = p.first;
                bool fromSide = walker->rayFlag & DDA::RF_SIDE;
                float minLineDist = 1e30;

                for(const Wall& wallInfo : walker->getTargetScene()->getTileWalls(tileData)) {
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
                    rayLine.height = rayIntY - rayLine.slope * rayIntX; // Total intercept

                    // Find intersection point of line defining wall geometry and the ray line.
                    Vector2 inter = line->intersection(rayLine);
                    if((inter.x < 0 || inter.x > 1 || inter.y < 0 || inter.y > 1) ||
                       (inter.x < line->domainStart || inter.x > line->domainEnd)) {
                        // Intersection point is out of tile or domain bounds, skip it
                        continue;
                    } else {
                        // Intersection point is defined in specified bounds, process it
                        Vector2 tilePos = Vector2(
                            ensureInteger(rayInfo.point.x), // Ensuring integers prevents them from flipping suddenly,
                            ensureInteger(rayInfo.point.y)  // for example: it makes 5.9999 -> 6 and 6.0001 -> 6.
                        );
                        tilePos.x -= ( fromSide && rayDir.x < 0) ? (1) : (0);
                        tilePos.y -= (!fromSide && rayDir.y < 0) ? (1) : (0);

                        // TEMPORARY COMPUTATION OF CAMERA PLANE LINE
                        LineEquation plane(
                            mainCamera->getPlane().y / mainCamera->getPlane().x,
                            mainCamera->getPosition().y - (mainCamera->getPlane().y / mainCamera->getPlane().x) * mainCamera->getPosition().x,
                            0, 1
                        );

                        float pointDist = plane.pointDistance(tilePos + inter);
                        // The closest line equation defines the wall geometry
                        if(pointDist < minLineDist) {
                            // Find normal vector of the hit wall
                            Vector2 normal = (Vector2::RIGHT).rotate(-1 * atanf(1 / line->slope * -1));
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
                Vector2 sunDir = Vector2::RIGHT.rotate(-1 * M_PI / 6);
                float bn = -1 * wallNormal.dot(sunDir);
                wallColor.r *= bn;
                wallColor.g *= bn;
                wallColor.b *= bn;
            }
            SDL_SetRenderDrawColor(sdlRenderer, wallColor.r, wallColor.g, wallColor.b, 255);
            // Display the ray by drawing an appropriate vertical strip, scaling is applied in
            // form of aspect ratio (to make everything have the same size across all resolutions),
            // and camera plane length (to make squares look always like squares).
            float scale = (1 / fAspectRatio) * (1 / (2 * mainCamera->getPlane().magnitude()));
            int lineHeight = (iScreenHeight / wallDistance) * scale;
            int drawStart = iScreenHeight / 2 - lineHeight / 2;
            int drawEnd = iScreenHeight / 2 + lineHeight / 2;
            drawStart = drawStart < 0 ? 0 : drawStart;
            drawEnd = drawEnd >= iScreenHeight ? (iScreenHeight - 1) : drawEnd;
            for(int i = 0; i < iColumnsPerRay; i++) // Take columns per row into account
                SDL_RenderDrawLine(sdlRenderer, column + i, drawStart, column + i, drawEnd);
        }

        if(isCursorLocked)
            SDL_WarpMouseInWindow(sdlWindow, iScreenWidth / 2, iScreenHeight / 2);

        SDL_RenderPresent(sdlRenderer);
        SDL_Delay(msFrameDuration);
        frameIndex++;
        return run;
    }
}
