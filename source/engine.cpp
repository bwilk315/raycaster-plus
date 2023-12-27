
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
        this->bRun = true;
        this->bIsCursorLocked = false;
        this->bLightEnabled = false;
        this->iColumnsPerRay = 4;
        this->iScreenWidth = screenWidth;
        this->iScreenHeight = screenHeight;
        this->fAspectRatio = screenHeight / (float)screenWidth;
        this->fMaxTileDist = 16.0f;
        this->fLightAngle = 0.0f;
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
        bRun = false;
    }
    void Engine::setCursorLock(bool locked) {
        bIsCursorLocked = locked;
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
    void Engine::setLightBehavior(bool enabled, float angle) {
        bLightEnabled = enabled;
        fLightAngle = angle;
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
            if(p.second == KeyState::DOWN)
                keyStates.at(p.first) = KeyState::PRESS;
            else if(p.second == KeyState::UP)
                keyStates.erase(p.first);
        }

        // Interpret SDL events
        SDL_Event event;
        while(SDL_PollEvent(&event)) {
            int sc;
            switch(event.type) {
                case SDL_QUIT:
                    bRun = false;
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

        // Cache often-used information
        Vector2 camDir = mainCamera->getDirection();
        Vector2 camPos = mainCamera->getPosition();
        Vector2 planeVec = mainCamera->getPlane();
        Vector2 lightDir;
        if(bLightEnabled)
            lightDir = (Vector2::RIGHT).rotate(fLightAngle);

        // Obtain a line describing the camera plane
        float planeSlope = planeVec.y / planeVec.x;
        LineEquation planeLine(planeSlope, camPos.y - planeSlope * camPos.x, 0, 1);

        // Clear the screen
        SDL_SetRenderDrawColor(sdlRenderer, 0, 0, 0, 255);
        SDL_RenderClear(sdlRenderer);

        // Draw the current frame, which consists of pixel columns
        for(int column = 0; column < iScreenWidth; column += iColumnsPerRay) {
            // Position of the ray on the camera plane, from -1 (left) to 1 (right)
            float cameraX = 2 * column / (float)iScreenWidth - 1;
            Vector2 rayDir = (camDir + planeVec * cameraX).normalized();
            LineEquation rayLine;  // Line equation describing the ray walk
            // Ray slope stays constant for all hits
            rayLine.slope = (rayDir.x == 0) ? (LineEquation::MAX_SLOPE) : (rayDir.y / rayDir.x);

            // Properties of the nearest wall, which will get drawn if found
            bool nwExists     = false;
            int nwTileData    = -1;
            float nwDistance  = 1e30;
            SDL_Color nwColor = {0, 0, 0, 0};
            Vector2 nwNormal  = Vector2::ZERO;

            // Perform step-based DDA algorithm to find out which tiles get hit by the ray, find
            // the nearest wall of the nearest tile.
            walker->init(camPos, rayDir);
            while(true) {
                RayHitInfo hit = walker->next();

                
                // If next tiles are unreachable, or nearest wall was found exit the loop
                if( (nwExists) ||
                    (walker->rayFlag & DDA::RF_TOO_FAR) ||
                    (walker->rayFlag & DDA::RF_OUTSIDE) ) {
                    break;
                // If a ray touched an air tile (with data of 0), skip it
                } else if(!(walker->rayFlag & DDA::RF_HIT))
                    continue;

                // Obtain the hit tile data
                int_pair p = walker->getTargetScene()->getTileData(hit.tile.x, hit.tile.y);
                if(p.second != Scene::E_CLEAR)
                    break;
                int tileData = p.first;
                
                // It tells whether ray hit the tile from left or right direction
                bool fromSide = walker->rayFlag & DDA::RF_SIDE;

                // Find the nearest wall in the obtained non-air tile
                for(const Wall& wallInfo : walker->getTargetScene()->getTileWalls(tileData)) {
                    const LineEquation* line = &wallInfo.line; // For convienence
                    if(line->domainStart == line->domainEnd) // It will not be seen anyway
                        continue;
                    // Update the ray line intercept according to the ray hit point
                    float rayIntX;
                    float rayIntY;
                    if(fromSide) {
                        rayIntY = hit.point.y - (int)hit.point.y;
                        rayIntX = (rayDir.x < 0) ? (1) : (0);
                    } else {
                        rayIntX = hit.point.x - (int)hit.point.x;
                        rayIntY = (rayDir.y < 0) ? (1) : (0);
                    }
                    rayLine.height = rayIntY - rayLine.slope * rayIntX; // Total intercept

                    // Find intersection point of line defining current wall geometry and the ray line
                    Vector2 inter = line->intersection(rayLine);
                    if((inter.x < 0 || inter.x > 1 || inter.y < 0 || inter.y > 1) ||
                       (inter.x < line->domainStart || inter.x > line->domainEnd)) {
                        // Intersection point is out of tile or domain bounds, skip it then
                        continue;
                    } else {
                        // Intersection point is defined in specified bounds, process it
                        Vector2 tilePos = Vector2(
                            ensureInteger(hit.point.x), // Ensuring integers prevents them from flipping suddenly,
                            ensureInteger(hit.point.y)  // for example: it makes 5.9999 -> 6 and 6.0001 -> 6.
                        );
                        tilePos.x -= ( fromSide && rayDir.x < 0) ? (1) : (0);
                        tilePos.y -= (!fromSide && rayDir.y < 0) ? (1) : (0);
                        // Perpendicular distance from wall intersection point (global) to the camera plane
                        float pointDist = planeLine.pointDistance(tilePos + inter);
                        if(pointDist < nwDistance) {
                            // New nearest wall found, save its information
                            float normAngle = line->slope == 0 ? M_PI_2 : atanf(1 / line->slope * -1);
                            nwTileData = tileData;
                            nwDistance = pointDist;
                            nwColor = wallInfo.color;
                            nwNormal = (Vector2::RIGHT).rotate(normAngle);
                        }
                        nwExists = true;
                    }
                }
            }
            if(!nwExists) continue; // Ray hit nothing

            // Apply shading if light source is enabled
            if(bLightEnabled) {
                float bn = clamp(std::abs(nwNormal.dot(lightDir)) + 0.5f, 0.5f, 1.0f);
                nwColor.r *= bn;
                nwColor.g *= bn;
                nwColor.b *= bn;
            }

            // Display the ray by drawing an appropriate vertical strip, scaling is applied in form of
            // aspect ratio (to make everything have the same size across all resolutions), and camera
            // plane length (to make squares look always like squares).
            //loat persCorrector = 1 / (2 * tanf(mainCamera->getFieldOfView() / 2));
            float scale = (1 / fAspectRatio) * (1 / (2 * planeVec.magnitude()));
            int lineHeight = (iScreenHeight / nwDistance) * scale;
            int drawStart = iScreenHeight / 2 - lineHeight / 2;
            int drawEnd = iScreenHeight / 2 + lineHeight / 2;
            drawStart = drawStart < 0 ? 0 : drawStart;
            drawEnd = drawEnd >= iScreenHeight ? (iScreenHeight - 1) : drawEnd;

            SDL_SetRenderDrawColor(sdlRenderer, nwColor.r, nwColor.g, nwColor.b, SDL_ALPHA_OPAQUE);
            for(int i = 0; i < iColumnsPerRay; i++)
                SDL_RenderDrawLine(sdlRenderer, column + i, drawStart, column + i, drawEnd);

            #ifdef DEBUG
            if(column == iScreenWidth / 2) {
                SDL_SetRenderDrawColor(sdlRenderer, 255, 255, 255, 255);
                SDL_RenderDrawLine(sdlRenderer, iScreenWidth / 2, 0, iScreenWidth / 2, iScreenHeight);
            }
            #endif
        }

        if(bIsCursorLocked)
            SDL_WarpMouseInWindow(sdlWindow, iScreenWidth / 2, iScreenHeight / 2);

        SDL_RenderPresent(sdlRenderer);
        SDL_Delay(msFrameDuration);
        frameIndex++;
        return bRun;
    }
}
