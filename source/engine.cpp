
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
        this->bIsCursorLocked = false;
        this->bLightEnabled = false;
        this->bRun = true;
        this->iColumnsPerRay = 4;
        this->iScreenWidth = screenWidth;
        this->iScreenHeight = screenHeight;
        this->fAspectRatio = screenHeight / (float)screenWidth;
        this->fLightAngle = 0.0f;
        this->fMaxTileDist = 16.0f;
        this->frameIndex = 0;
        this->tpLast = system_clock::now();
        this->keyStates = map<int, KeyState>();
        this->walker = new DDA();
        setFrameRate(60);
        setWindowResize(true);

        SDL_InitSubSystem(SDL_INIT_VIDEO);
        SDL_CreateWindowAndRenderer(screenWidth, screenHeight, SDL_WINDOW_SHOWN, &this->sdlWindow, &this->sdlRenderer);
        SDL_SetWindowTitle(this->sdlWindow, "Raycaster Plus Engine");
        SDL_SetWindowResizable(this->sdlWindow, SDL_TRUE);
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
        iColumnsPerRay = clamp(columns, 1, iScreenWidth);
    }
    void Engine::setFrameRate(int framesPerSecond) {
        msFrameDuration = 1000 / clamp(framesPerSecond, 1, 1000);
    }
    void Engine::setLightBehavior(bool enabled, float angle) {
        bLightEnabled = enabled;
        fLightAngle = angle;
    }
    void Engine::setMainCamera(const Camera* camera) {
        mainCamera = camera;
    }
    void Engine::setWindowResize(bool enabled) {
        bAllowWindowResize = enabled;
        SDL_SetWindowResizable(sdlWindow, (SDL_bool)enabled);
    }
    int Engine::setRenderFitMode(const RenderFitMode& rfm) {
        if(mainCamera == nullptr)
            return Engine::E_MAIN_CAMERA_NOT_SET;
        
        renderFitMode = rfm;
        switch(rfm) {
            case RenderFitMode::STRETCH:
                iHorOffset = 0;
                iVerOffset = 0;
                iColumnsCount = iScreenWidth;
                break;
            case RenderFitMode::SQUARE:
                // Set up horizontal and vertical offsets for drawing columns to make the rendered
                // frame always form a square.
                if(iScreenWidth > iScreenHeight) {
                    iHorOffset = (iScreenWidth - iScreenHeight) / 2;
                    iVerOffset = 0;
                    iColumnsCount = iScreenHeight;
                } else {
                    iHorOffset = 0;
                    iVerOffset = (iScreenHeight - iScreenWidth) / 2;
                    iColumnsCount = iScreenWidth;
                }
                break;
        }
        return Engine::E_CLEAR;
    }
    int Engine::getScreenWidth() const {
        return iScreenWidth;
    }
    int Engine::getScreenHeight() const {
        return iScreenHeight;
    }
    int Engine::getRenderWidth() {
        return (renderFitMode == RenderFitMode::STRETCH) ? (iScreenWidth) : (iColumnsCount);
    }
    int Engine::getRenderHeight() {
        return (renderFitMode == RenderFitMode::STRETCH) ? (iScreenHeight) : (iColumnsCount);
    }
    float Engine::getElapsedTime() const {
        return elapsedTime.count();
    }
    KeyState Engine::getKeyState(int scanCode) const {
        return keyStates.count(scanCode) == 0 ? KeyState::NONE : keyStates.at(scanCode);
    }
    DDA* const Engine::getWalker() {
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
                case SDL_WINDOWEVENT:
                    if(event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                        SDL_GetWindowSizeInPixels(sdlWindow, &iScreenWidth, &iScreenHeight);
                        setRenderFitMode(renderFitMode); // Refresh fit mode to let it adapt to new resolution
                    }
                    break;
            }
        }

        // Cache often-used information
        Scene* const mainScene = walker->getTargetScene();
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
            bool nwExists        = false;
            int nwTileData       = -1;
            float nwPlaneNormX     = -1; // Normalized horizontal position on the wall plane
            float nwDistance     = 1e30;
            Vector2 nwNormal     = Vector2::ZERO;
            SDL_Color nwColor    = {0, 0, 0, 0};
            string nwTextureFile = "";

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
                int_pair p = mainScene->getTileData(hit.tile.x, hit.tile.y);
                if(p.second != Scene::E_CLEAR)
                    break;
                int tileData = p.first;
                
                // It tells whether ray hit the tile from left or right direction
                bool fromSide = walker->rayFlag & DDA::RF_SIDE;


                // if(column == iColumnsCount / 2) {
                //     system("clear");
                // }


                // Find the nearest wall in the obtained non-air tile
                for(const Wall& wallInfo : mainScene->getTileWalls(tileData)) {
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
                        bool isNormalFlipped = false;
                        float normAngle = (line->slope == 0) ? (-1 * M_PI_2) : (atanf(1 / line->slope * -1));
                        float globalPosInterY = line->slope * (camPos.x - hit.tile.x) + line->height + hit.tile.y;
                        Vector2 normal = (Vector2::RIGHT).rotate(normAngle);
                        if( (line->slope >= 0 && camPos.y > globalPosInterY) || // If camera is above the line or below
                            (line->slope  < 0 && camPos.y < globalPosInterY)) { // it, normal needs to be flipped.
                            normal = normal * -1;
                            isNormalFlipped = true;
                        }


                        // THERE IS A PROBLRM EITH PERFECT LINE EQUATIONS!!!
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

                            // Find two places where the line intersects with the tile boundary, it is later
                            // used to know distance of the intersection point along the line.
                            Vector2 lh0, lh1;
                            lh0.x = (line->slope == 0) ? (0) : (-1 * line->height / line->slope);
                            if(lh0.x < 0) {
                                lh0.x = 0;
                                lh0.y = line->height;
                            } else if(lh0.x > 1) {
                                lh0.x = 1;
                                lh0.y = line->slope + line->height;
                            } else {
                                lh0.y = 0;
                            }
                            lh1.x = (line->slope == 0) ? (1) : ((1 - line->height) / line->slope);
                            if(lh1.x < 0) {
                                lh1.x = 0;
                                lh1.y = line->height;
                            } else if(lh1.x > 1) {
                                lh1.x = 1;
                                lh1.y = line->slope + line->height;
                            } else {
                                lh1.y = 1;
                            }
                            
                            // Save the new nearest wall information
                            nwTileData    = tileData;
                            nwDistance    = pointDist;
                            nwColor       = wallInfo.color;
                            nwNormal      = normal;
                            nwTextureFile = wallInfo.textureFile;

                            // Compute normalized horizontal position on the wall plane
                            float wallPlaneX = ((isNormalFlipped ? lh1 : lh0) - inter).magnitude();
                            nwPlaneNormX = wallPlaneX / (lh1 - lh0).magnitude();


                            // if(column == iColumnsCount / 2) {
                            //     system("clear");
                            //     std::cout << *line << ": " << inter << "\n";
                            // }


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
            int renderHeight = getRenderHeight();
            float refDist = 1 / (2 * tanf(mainCamera->getFieldOfView() / 2));
            int lineHeight = renderHeight * (1 / (nwDistance / refDist));
            int drawStart = renderHeight / 2 + lineHeight / 2;
            int drawEnd = renderHeight / 2 - lineHeight / 2;
            drawStart = (drawStart < 0) ? (0) : (drawStart);
            drawEnd = (drawEnd >= renderHeight) ? (renderHeight - 1) : (drawEnd);

            // Use texture if existent, otherwise just a solid color
            const Texture* tex = mainScene->getTexture(nwTextureFile);
            if(tex == nullptr) {
                SDL_SetRenderDrawColor(sdlRenderer, nwColor.r, nwColor.g, nwColor.b, SDL_ALPHA_OPAQUE);
                for(int i = 0; i < iColumnsPerRay; i++)
                    SDL_RenderDrawLine(
                        sdlRenderer,
                        iHorOffset + column + i,
                        iVerOffset + drawStart,
                        iHorOffset + column + i,
                        iVerOffset + drawEnd
                    );
            } else {
                // Draw part of the texture from top to bottom using normalized plane position information
                int texHeight = tex->getHeight();
                float pixelHeight = lineHeight / (float)texHeight;

                // if(column == iColumnsCount / 2) {
                //     system("clear");
                //     std::cout << nwPlaneNormX << "\n";
                // }

                for(int y = 0; y < texHeight; y++) {
                    SDL_Color px = tex->getPixelNorm(nwPlaneNormX, y / (float)texHeight);
                    SDL_SetRenderDrawColor(sdlRenderer, px.r, px.g, px.b, 255);
                    for(int i = 0; i < iColumnsPerRay; i++)
                        SDL_RenderDrawLine(
                            sdlRenderer,
                            iHorOffset + column + i,
                            iVerOffset + drawStart - pixelHeight * y,
                            iHorOffset + column + i,
                            iVerOffset + drawStart - pixelHeight * (y + 1)
                        );
                }
            }

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
