
#include "../include/engine.hpp"

#ifdef DEBUG
#include <iostream>
#endif

namespace rp {
    #ifdef DEBUG
    using ::std::cout;
    using ::std::endl;
    #endif

    RayHitInfo Engine::simulateBoundaryEnter(const Vector2& pos, const Vector2& dir) {
        // Form a simulated hit structure and fake the ray flag value
        RayHitInfo hit;
        hit.tile = Vector2((int)pos.x, (int)pos.y);

        // Position in the local tile space
        float Xl = pos.x - hit.tile.x;
        float Yl = pos.y - hit.tile.y;
        // Potential values for both axes
        float Xp = dir.x < 0;
        float Yp = dir.y < 0;
        // Actual values given the potential values
        float Xa = (dir.x / dir.y) * (Yp - Yl) + Xl;
        float Ya = (dir.y / dir.x) * (Xp - Xl) + Yl;
        hit.point = Vector2(
            // Chose this pair of coordinates which respects the tile boundaries
            ( (Xa < 0 || Xa > 1) ? (Xp) : (Xa) ),
            ( (Ya < 0 || Ya > 1) ? (Yp) : (Ya) ) + hit.tile.y
        );
        walker->rayFlag = DDA::RF_HIT | ( (hit.point.x == 0 || hit.point.x == 1) ? (DDA::RF_SIDE) : (0) );
        hit.point.x += hit.tile.x;
        return hit;
    }
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

        SDL_SetRenderDrawBlendMode(sdlRenderer, SDL_BLENDMODE_BLEND); // For alpha channel
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
        const Scene* const mainScene = walker->getTargetScene();
        Vector2 camDir = mainCamera->getDirection();
        Vector2 camPos = mainCamera->getPosition();
        Vector2 planeVec = mainCamera->getPlane();
        Vector2 lightDir;
        const float refDist = 1 / (2 * tanf(mainCamera->getFieldOfView() / 2)); // Perspective-correct
        if(bLightEnabled)
            lightDir = (Vector2::RIGHT).rotate(fLightAngle);

        // Obtain a line describing the camera plane
        const float planeSlope = planeVec.y / planeVec.x;
        LinearFunc planeLine(planeSlope, camPos.y - planeSlope * camPos.x, 0, 1);

        // Clear the screen
        SDL_SetRenderDrawColor(sdlRenderer, 0, 0, 0, 255);
        SDL_RenderClear(sdlRenderer);

        // Draw the current frame, which consists of pixel columns
        for(int column = 0; column < iColumnsCount; column += iColumnsPerRay) {
            // Position of the ray on the camera plane, from -1 (left) to 1 (right)
            float cameraX = 2 * column / (float)iColumnsCount - 1;
            Vector2 rayDir = (camDir + planeVec * cameraX).normalized();
            LinearFunc rayLine;  // Line equation describing the ray walk
            // Ray slope stays constant for all hits
            rayLine.slope = (rayDir.x == 0) ? (LinearFunc::MAX_SLOPE) : (rayDir.y / rayDir.x);

            // Properties of the nearest wall, which will get drawn if found
            // bool nwExists        = false;
            // int nwTileData       = -1;
            // float nwPlaneNormX     = -1; // Normalized horizontal position on the wall plane
            // float nwDistance     = 1e30;
            // Vector2 nwNormal     = Vector2::ZERO;
            // Color nwColor    = {0, 0, 0, 0};
            // int nwTexId = 0;

            // Perform step-based DDA algorithm to find out which tiles get hit by the ray, find
            // the nearest wall of the nearest tile.
            walker->init(camPos, rayDir);
            bool keepWalking = true;
            bool originDone = false; // Whether walls from the origin were examined
            bool wasAnyHit = false;
            int renderHeight = getRenderHeight();
            
            Color pixelsBuffer[renderHeight];
            memset(pixelsBuffer, 0, renderHeight);

            while(keepWalking) {
                RayHitInfo hit = originDone ? walker->next() : simulateBoundaryEnter(camPos, rayDir);

                // If next tiles are unreachable, or nearest wall was found exit the loop
                // (nwExists) ||
                if((walker->rayFlag & DDA::RF_TOO_FAR) || (walker->rayFlag & DDA::RF_OUTSIDE))
                    break;
                // If a ray touched an air tile (with data of 0), skip it
                else if(!(walker->rayFlag & DDA::RF_HIT))
                    continue;           

                // Update the ray line intercept according to the ray hit point
                float rayIntX;
                float rayIntY;
                if(walker->rayFlag & DDA::RF_SIDE) {
                    rayIntY = hit.point.y - (int)hit.point.y;
                    rayIntX = rayDir.x < 0;
                } else {
                    rayIntX = hit.point.x - (int)hit.point.x;
                    rayIntY = rayDir.y < 0;
                }
                rayLine.height = rayIntY - rayLine.slope * rayIntX;

                // Find the nearest wall in the obtained non-air tile
                int tileData = mainScene->getTileData(hit.tile.x, hit.tile.y);

                // #ifdef DEBUG
                // if(column == iColumnsCount / 2) {
                //     system("clear");
                // }
                // #endif

                vector<WallDetails> details = mainScene->getTileWalls(tileData);
                map<float, pair<int, Vector2>> dict; // Key: distance, Value: ( Key: index, Value: intersection )

                /* THIS LOOP IS INTENTED TO SORT THE WALLS BY DISTANCE (KEY OF THE MAP, which does it automaticaly) */
                for(int i = 0; i < details.size(); i++) {
                    WallDetails* wdp = &details.at(i);
                    // Find intersection point of line defining current wall geometry and the ray line
                    Vector2 inter = wdp->func.getCommonPoint(rayLine);
                    if((inter.x < 0 || inter.x > 1 || inter.y < 0 || inter.y > 1) ||
                       (inter.x < wdp->func.xMin || inter.x > wdp->func.xMax) ||
                       (inter.y < wdp->func.yMin || inter.y > wdp->func.yMax)) {
                        // Intersection point is out of tile or domain bounds, skip it then
                        continue;
                    }
                    // Perpendicular distance from wall intersection point (global) to the camera plane
                    float planeDist = planeLine.getDistanceFromPoint(hit.tile + inter);

                    dict.insert(pair<float, pair<int, Vector2>>(planeDist, pair<int, Vector2>(i, inter)));
                }

                // Draw buffer
                for(const auto& de : dict) {

                    const float planeDist = de.first;
                    const Vector2 inter = de.second.second;
                    const WallDetails wd = details.at(de.second.first);

                    /***** SECTION: FIND WALL INTERSECTION AND NORMAL *****/

                    // // // Find intersection point of line defining current wall geometry and the ray line
                    // Vector2 inter = wd.func.getCommonPoint(rayLine);
                    // if((inter.x < 0 || inter.x > 1 || inter.y < 0 || inter.y > 1) ||
                    //    (inter.x < wd.func.xMin || inter.x > wd.func.xMax) ||
                    //    (inter.y < wd.func.yMin || inter.y > wd.func.yMax)) {
                    //     // Intersection point is out of tile or domain bounds, skip it then
                    //     continue;
                    // }
                    // // Perpendicular distance from wall intersection point (global) to the camera plane
                    // float planeDist = planeLine.getDistanceFromPoint(hit.tile + inter);

                    // Find normal vector that always points out of the wall plane
                    bool isNormalFlipped = false;
                    float normAngle = (wd.func.slope == 0) ? (-1 * M_PI_2) : (atanf(1 / wd.func.slope * -1));
                    float globalPosInterY = wd.func.getValue(camPos.x - hit.tile.x) + hit.tile.y;
                    Vector2 normal = (Vector2::RIGHT).rotate(normAngle);
                    if( (wd.func.slope >= 0 && camPos.y > globalPosInterY) || // If camera is above the line or below
                        (wd.func.slope  < 0 && camPos.y < globalPosInterY)) { // it, normal needs to be flipped.
                        normal = normal * -1;
                        isNormalFlipped = true;
                    }
                    // Prevent drawing things that are behind the camera, it is caused by finding intersections in the
                    // origin tile, there are always two but only one is in front of the camera (detected using dot product).
                    if(!originDone && rayDir.dot(normal) > 0)
                        continue;

                    /***** SECTION: UPDATE PIXELS BUFFER *****/

                    
                    // Find out range describing column start and end height
                    float lineHeight = renderHeight * (refDist / planeDist);
                    float drawStart  = (renderHeight / 2 - lineHeight / 2);
                    float drawEnd    = (renderHeight / 2 + lineHeight / 2);

                    // if(drawEnd < 0) {
                    //     drawStart += drawEnd;
                    //     drawEnd = 0;
                    // }
                    // #ifdef DEBUG
                    // if(column == iColumnsCount / 2) {
                    //     cout << drawStart << " to " << drawEnd << "\n";
                    // }
                    // #endif
                    // lineHeight = clamp(lineHeight, 0, renderHeight);
                    // drawStart  = clamp(drawStart,  0, renderHeight);
                    // drawEnd    = clamp(drawEnd,    0, renderHeight);


                    const Texture* tex = mainScene->getTexture(wd.texId);
                    // Compute normalized horizontal position on the wall plane
                    float planeNormX = ((isNormalFlipped ? wd.bp1 : wd.bp0) - inter).magnitude() / (wd.bp1 - wd.bp0).magnitude();
                    //float texHeight = (float)tex->getHeight();

                    // Draw pixels to the buffer, with source being either texture or solid color
                    int limitedStart = drawStart + (1 - wd.hMax) * lineHeight;
                    int limitedEnd = drawEnd - wd.hMin * lineHeight;
                    // CLAMP AFTERWARDS NOT JUST AFTER COMPUTING START AND HEIGHT BEFORE
                    limitedStart = clamp(limitedStart, 0, renderHeight - 1);
                    limitedEnd = clamp(limitedEnd, 0, renderHeight - 1);

                    for(int h = limitedStart; h != limitedEnd + 1; h++) {

                        Color wc = (tex == nullptr) ? wd.tint : tex->getCoords(planeNormX, 1 - (h - limitedStart) / (float)lineHeight);

                        // #ifdef DEBUG
                        // if(column == iColumnsCount / 2) {
                        //     cout << planeNormX, ((h - limitedStart) / lineHeight) << "\n";
                        // }
                        // #endif

                        // Apply shading if light source is enabled
                        if(bLightEnabled) {
                            const float minBn = 0.3f;
                            float perc = -1 * (normal.dot(lightDir) - 1) / 2; // Brightness linear interploation percent
                            float bn = minBn + (1 - minBn) * perc; // Final brightness
                            wc.red *= bn;
                            wc.green *= bn;
                            wc.blue *= bn;
                        }

                        // IN THE FUTURE MIXING OPACITY COLOR WITH NEXT ONES CAN BE IMPLEMENTED
                        if(pixelsBuffer[h].alpha == 0) {
                            //wc.alpha -= pixelsBuffer[h].alpha;
                            pixelsBuffer[h] = wc;
                        }
                    }

                    wasAnyHit = true;
                    if(wd.stopsRay) {
                        keepWalking = false;
                        break;
                    }
                }
                
                originDone = true;
            }
            if(!wasAnyHit)
                continue; // Ray ended his walk and hit nothing, skip the iteration

            const int iRowsPerColor = 1;
            Color lc;
            for(int h = 0; h <= renderHeight; h += iRowsPerColor) {
                Color wc = pixelsBuffer[h];
                if(wc.alpha == 0)
                    continue;
                
                if(wc.red != lc.red || wc.green != lc.green || wc.blue != lc.blue || wc.alpha != lc.alpha) {
                    lc = wc;
                    SDL_SetRenderDrawColor(sdlRenderer, lc.red, lc.green, lc.blue, lc.alpha);
                }


                for(int i = 0; i < iColumnsPerRay; i++)
                    for(int j = 0; j < iRowsPerColor; j++)
                    SDL_RenderDrawPoint(
                        sdlRenderer,
                        iHorOffset + column + i,
                        h + j
                        // h,
                        // iHorOffset + column + iColumnsPerRay,
                        // h
                    );
            }
        
            // // Use texture if existent, otherwise just a solid color
            // const Texture* tex = mainScene->getTexture(nwTexId);
            // if(tex == nullptr) {
            //     SDL_SetRenderDrawColor(sdlRenderer, nwColor.red, nwColor.green, nwColor.blue, SDL_ALPHA_OPAQUE);
            //     for(int i = 0; i < iColumnsPerRay; i++)
            //         SDL_RenderDrawLine(
            //             sdlRenderer,
            //             iHorOffset + column + i,
            //             iVerOffset + drawStart,
            //             iHorOffset + column + i,
            //             iVerOffset + drawEnd
            //         );
            // } else {
            //     // Draw part of the texture from top to bottom using normalized plane position information
                // int texHeight = tex->getHeight();
                // float pixelHeight = lineHeight / (float)texHeight;

                // // if(column == iColumnsCount / 2) {
                // //     system("clear");
                // //     std::cout << nwPlaneNormX << "\n";
                // // }

                // for(int y = 0; y < texHeight; y++) {
                //     Color px = tex->getCoords(nwPlaneNormX, y / (float)texHeight);
                //     SDL_SetRenderDrawColor(sdlRenderer, px.red, px.green, px.blue, 255);
                //     for(int i = 0; i < iColumnsPerRay; i++)
                //         SDL_RenderDrawLine(
                //             sdlRenderer,
                //             iHorOffset + column + i,
                //             iVerOffset + drawStart - pixelHeight * y,
                //             iHorOffset + column + i,
                //             iVerOffset + drawStart - pixelHeight * (y + 1)
                //         );
                // }
            // }

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
        if(frameIndex % msFrameDuration == 0) {
            system("clear");
            cout << "Frames per second: " << (1.0f / getElapsedTime()) << "\n";
        }
        #endif

        if(bIsCursorLocked)
            SDL_WarpMouseInWindow(sdlWindow, iScreenWidth / 2, iScreenHeight / 2);

        SDL_RenderPresent(sdlRenderer);
        SDL_Delay(msFrameDuration);
        frameIndex++;
        return bRun;
    }
}
