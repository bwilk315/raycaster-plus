
#define DEBUG

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
    void Engine::setMainCamera(Camera* camera) {
        mainCamera = camera;
    }
    void Engine::setRenderFitMode(const RenderFitMode& rfm) {

        
        // THIS WORKS FOR THE FIRST TRY, THEN IT BEHAVES WEIRD


        renderFitMode = rfm;
        if(rfm == RenderFitMode::CHANGE_FOV) {
            // Change the camera field of view so look of the walls is perspective-correct
            // I do things using the fact that if you are 1/2 from the wall then with 90deg
            // FOV your screen height would be used completely.
            mainCamera->setFieldOfView(2 * atan(1 / fAspectRatio));
            iHorOffset = 0;
            iVerOffset = 0;
            iColumnsCount = iScreenWidth;
        } else if(rfm == RenderFitMode::TRIM_SCREEN) {
            // Set up horizontal and vertical offsets for drawing columns to make the rendered
            // frame always form a square, which guarantees perspective-correct look.
            if(iScreenWidth > iScreenHeight) {
                iHorOffset = (iScreenWidth - iScreenHeight) / 2;
                iVerOffset = 0;
                iColumnsCount = iScreenHeight;
            } else {
                iHorOffset = 0;
                iVerOffset = (iScreenHeight - iScreenWidth) / 2;
                iColumnsCount = iScreenWidth;
            }
        }
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
        for(int column = 0; column < iColumnsCount; column += iColumnsPerRay) {
            // Position of the ray on the camera plane, from -1 (left) to 1 (right)
            float cameraX = 2 * column / (float)iColumnsCount - 1;
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
            bool originDone = false; // Whether walls from the origin were examined
            while(true) {
                RayHitInfo hit;
                if(originDone) {
                    hit = walker->next();
                } else {
                    // Simulate a ray entering the current (aka origin) tile, to take walls that are
                    // defined for the tile into account.
                    bool side = true;
                    int camTileX = (int)camPos.x;
                    int camTileY = (int)camPos.y;
                    float localCamX = camPos.x - camTileX;
                    float localCamY = camPos.y - camTileY;
                    Vector2 backPoint;
                    if(rayDir.x >= 0) {
                        backPoint.x = 0;
                        backPoint.y = -1 * rayLine.slope * localCamX + localCamY;
                    } else {
                        backPoint.x = 1;
                        backPoint.y = rayLine.slope * (1 - localCamX) + localCamY;
                    }
                    if(backPoint.y > 1) {
                        backPoint.x = (1 + rayLine.slope * localCamX - localCamY) / rayLine.slope;
                        backPoint.y = 1;
                        side = false;
                    } else if(backPoint.y < 0) {
                        backPoint.x = (rayLine.slope * localCamX - localCamY) / rayLine.slope;
                        backPoint.y = 0;
                        side = false;
                    }

                    walker->rayFlag = DDA::RF_HIT | (side ? (DDA::RF_SIDE) : (DDA::RF_CLEAR));
                    hit = RayHitInfo(0, Vector2(camTileX, camTileY), Vector2(backPoint.x + camTileX, backPoint.y + camTileY));
                }
                
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

                        // Find normal vector that always points out of the wall plane
                        float normAngle = (line->slope == 0) ? (-1 * M_PI_2) : (atanf(1 / line->slope * -1));
                        float globalPosInterY = line->slope * (camPos.x - hit.tile.x) + line->height + hit.tile.y;
                        Vector2 normal = (Vector2::RIGHT).rotate(normAngle);
                        if( (line->slope >= 0 && camPos.y > globalPosInterY) ||
                            ((line->slope < 0 && camPos.y < globalPosInterY)))
                            // This is based on the fact of player being either above or below the wall line equation
                            normal = normal * -1;

                        if(!originDone && rayDir.dot(normal) > 0)
                            continue;

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
                            nwTileData = tileData;
                            nwDistance = pointDist;
                            nwColor = wallInfo.color;
                            nwNormal = normal;
                        }
                        nwExists = true;
                    }
                }
                
                originDone = true;
            }
            if(!nwExists) continue; // Ray hit nothing
            

            // Apply shading if light source is enabled
            if(bLightEnabled) {
                const float minBn = 0.3f;
                float perc = -1 * (nwNormal.dot(lightDir) - 1) / 2; // Brightness linear interploation percent
                float bn = minBn + (1 - minBn) * perc; // Final brightness
                nwColor.r *= bn;
                nwColor.g *= bn;
                nwColor.b *= bn;
            }

            // Display the ray by drawing an appropriate vertical strip, with settings appropriate
            // for the current render fit mode.
            int renderHeight = (renderFitMode == RenderFitMode::CHANGE_FOV) ? (iScreenHeight) : (iColumnsCount);
            float refDist = 1 / (2 * tanf(mainCamera->getFieldOfView() / 2));
            int lineHeight = (renderHeight / nwDistance * refDist) / ((renderFitMode == RenderFitMode::CHANGE_FOV) ? (fAspectRatio) : (1));

            int drawStart = renderHeight / 2 - lineHeight / 2;
            int drawEnd = renderHeight / 2 + lineHeight / 2;
            drawStart = (drawStart < 0) ? (0) : (drawStart);
            drawEnd = (drawEnd >= renderHeight) ? (renderHeight - 1) : (drawEnd);

            SDL_SetRenderDrawColor(sdlRenderer, nwColor.r, nwColor.g, nwColor.b, SDL_ALPHA_OPAQUE);
            for(int i = 0; i < iColumnsPerRay; i++)
                SDL_RenderDrawLine(
                    sdlRenderer,
                    iHorOffset + column + i,
                    iVerOffset + drawStart,
                    iHorOffset + column + i,
                    iVerOffset + drawEnd
                );

            #ifdef DEBUG
            if(abs(column - iColumnsCount / 2) <= iColumnsPerRay) {
                SDL_SetRenderDrawColor(sdlRenderer, 255, 255, 255, 255);
                SDL_RenderDrawLine(
                    sdlRenderer,
                    iScreenWidth / 2,
                    iVerOffset,
                    iScreenWidth / 2,
                    iScreenHeight - iVerOffset
                );
            }
            #endif
        }

        #ifdef DEBUG
        // system("clear");
        // cout << "Frames per second: " << (1.0f / getElapsedTime()) << "\n";
        #endif

        if(bIsCursorLocked)
            SDL_WarpMouseInWindow(sdlWindow, iScreenWidth / 2, iScreenHeight / 2);

        SDL_RenderPresent(sdlRenderer);
        SDL_Delay(msFrameDuration);
        frameIndex++;
        return bRun;
    }
}
