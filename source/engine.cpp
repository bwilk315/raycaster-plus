
#include "../include/engine.hpp"

#ifdef DEBUG
#include <iostream>
#endif

namespace rp {
    #ifdef DEBUG
    using ::std::cout;
    using ::std::endl;
    #endif

    void Engine::updateSurface() {
        sdlSurface = SDL_GetWindowSurface(sdlWindow);
        pixels = (uint32_t*)sdlSurface->pixels;
        SDL_SetSurfaceBlendMode(sdlSurface, SDL_BLENDMODE_BLEND);
    }
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
        this->fSumFPS = 0.0f;
        this->fMaxTileDist = 16.0f;
        this->frameIndex = 0;
        this->vLightDir = Vector2::RIGHT;
        this->tpLast = system_clock::now();
        this->keyStates = map<int, KeyState>();
        this->walker = new DDA();
        setFrameRate(60);
        setWindowResize(true);

        SDL_InitSubSystem(SDL_INIT_VIDEO);
        this->sdlWindow = SDL_CreateWindow("Raycaster Plus Engine", 0, 0, screenWidth, screenHeight, SDL_WINDOW_SHOWN);
        SDL_SetWindowResizable(this->sdlWindow, SDL_TRUE);
        updateSurface();
    }
    Engine::~Engine() {
        SDL_DestroyWindow(sdlWindow);
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
        iColumnsPerRay = clamp(columns, 1, getRenderWidth());
    }
    void Engine::setFrameRate(int framesPerSecond) {
        msFrameDuration = 1000 / clamp(framesPerSecond, 1, 1000);
    }
    void Engine::setLightBehavior(bool enabled, float angle) {
        bLightEnabled = enabled;
        vLightDir = (Vector2::RIGHT).rotate(angle);
    }
    void Engine::setMainCamera(Camera* camera) {
        mainCamera = camera;
    }
    void Engine::setRowsInterval(int interval) {
        iRowsInterval = clamp(interval, 1, getRenderHeight());
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
    bool Engine::tick() {

        // Compute the time duration elapsed since the last method call
        time_point<system_clock> tpCurrent = system_clock::now();
        elapsedTime = tpCurrent - tpLast;
        tpLast = tpCurrent;

        /************************************/
        /************************************/
        /********** GET INPUT DATA **********/
        /************************************/
        /************************************/

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
                        // Refresh the fit mode and the surface to let them adapt to a new resolution
                        setRenderFitMode(renderFitMode);
                        updateSurface();
                    }
                    break;
            }
        }

        /********************************************/
        /********************************************/
        /********** FILL THE SCREEN BUFFER **********/
        /********************************************/
        /********************************************/

        int renderWidth = getRenderWidth();
        int renderHeight = getRenderHeight();
        // Perspective-correct minimum distance; if you stand this distance from the cube looking at it orthogonally,
        // entire vertical view of the camera should be occupied by the cube front wall. This assumes that camera is
        // located at height of 1/2.
        const float pcmDist = 1 / (2 * tanf(mainCamera->getFieldOfView() / 2));
        const Scene* const mainScene = walker->getTargetScene();
        Vector2 camDir = mainCamera->getDirection();
        Vector2 camPos = mainCamera->getPosition();
        Vector2 planeVec = mainCamera->getPlane();

        // Linear function describing the camera plane, it is later used for computing distances to intersection points
        const float planeSlope = planeVec.y / planeVec.x;
        LinearFunc planeLine(planeSlope, camPos.y - planeSlope * camPos.x, 0, 1);

        // Clear the entire screen buffer
        SDL_LockSurface(sdlSurface);
        SDL_memset(pixels, 0, sdlSurface->pitch * sdlSurface->h);
        SDL_UnlockSurface(sdlSurface);

        // Draw the current frame, which consists of pixel columns
        for(int column = 0; column < renderWidth; column += iColumnsPerRay) {

            // Position of the ray on the camera plane, from -1 (left) to 1 (right)
            float cameraX = 2 * column / (float)iColumnsCount - 1;
            Vector2 rayDir = (camDir + planeVec * cameraX).normalized();
            LinearFunc rayLine;  // Line equation describing the ray walk
            // Ray slope stays constant for all hits
            rayLine.slope = (rayDir.x == 0) ? (LinearFunc::MAX_SLOPE) : (rayDir.y / rayDir.x);

            // Perform step-based DDA algorithm to find out which tiles get hit by the ray, find
            // the nearest wall of the nearest tile.
            walker->init(camPos, rayDir);
            bool keepWalking = true;
            bool originDone = false; // Whether walls from the origin were examined
            bool wasAnyHit = false;

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

                // Compute additional information of the walls, store them in a map that will sort them by distance
                vector<WallDetails> details = mainScene->getTileWalls(tileData);
                map<float, pair<int, Vector2>> additional; // Key: distance, Value: ( Key: index, Value: intersection )

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

                    additional.insert(pair<float, pair<int, Vector2>>(planeDist, pair<int, Vector2>(i, inter)));
                }
                
                // Draw buffer
                for(const auto& extra : additional) {

                    const float planeDist = extra.first;
                    const Vector2 inter = extra.second.second;
                    const WallDetails wd = details.at(extra.second.first);

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
                    
                    // Find out range describing how column should be drawn for the current wall
                    int lineHeight = renderHeight * (pcmDist / planeDist);
                    int drawStart  = (renderHeight / 2 - lineHeight / 2);
                    int drawEnd    = (renderHeight / 2 + lineHeight / 2);
                    // Compute starting and ending height according to the wall height range
                    int limStart   = drawStart + (1 - wd.hMax) * lineHeight;
                    int limEnd     = drawEnd - wd.hMin * lineHeight;
                    int limHeight  = drawEnd - drawStart;
                    // Index of the screen buffer at which drawing will start, in upper direction
                    int currIndex = (iVerOffset + limEnd) * sdlSurface->w + iHorOffset + column;

                    const Texture* tex = mainScene->getTexture(wd.texId);
                    int texHeight = tex == nullptr ? 1 : tex->getHeight();
                    // Compute normalized horizontal position on the wall plane
                    float planeNormX = ((isNormalFlipped ? wd.bp1 : wd.bp0) - inter).magnitude() / (wd.bp1 - wd.bp0).magnitude();
                    // Get height of a single pixel on the computed line (if texture is not found, it spans the whole height)
                    float pixelHeight = lineHeight / (float)texHeight;


                    SDL_LockSurface(sdlSurface);
                    int leClip = 0; // Height below which line is invisible
                    if(limEnd >= renderHeight) {
                        leClip = limEnd - renderHeight;
                        currIndex -= leClip * sdlSurface->w;
                        limEnd -= leClip;
                    }
                    for(int h = leClip; h < limHeight; h += iRowsInterval) {
                        //  PROBLEM: BLACK PIXELS ARE SOMEHOW OMITTED DURING THE DRAWING

                        if(limEnd < 0) {
                            break;
                        } else if(limEnd < renderHeight) {
                            uint8_t wr, wg, wb, wa; // Wall color
                            Texture::getNumberAsColor(
                                (tex == nullptr) ? wd.tint : tex->getCoords(planeNormX, h / (float)limHeight),
                                wr, wg, wb, wa
                            );

                            if(wa != 0) {
                                // Apply shading if light source is enabled
                                if(bLightEnabled) {
                                    const float minBn = 0.3f;
                                    float perc = -1 * (normal.dot(vLightDir) - 1) / 2; // Brightness linear interploation percent
                                    float bn = minBn + (1 - minBn) * perc; // Final brightness
                                    wr *= bn;
                                    wg *= bn;
                                    wb *= bn;
                                }

                                uint8_t cr, cg, cb, ca; // Current color
                                Texture::getNumberAsColor(pixels[currIndex], cr, cg, cb, ca);

                                // Set screen buffer pixel if it is not already (meaning its opacity is 0)
                                if(ca == 0)
                                    for(int i = 0; i < iColumnsPerRay; i++) {
                                        if(column + i == renderWidth)
                                            break;
                                        for(int j = 0; j < iRowsInterval; j++) {
                                            if(limEnd - j < 0)
                                                break;
                                            // Use SDL-provided function for converting RGBA color in appropriate way to a single number
                                            pixels[currIndex + i - j * sdlSurface->w] = SDL_MapRGB(sdlSurface->format, wr, wg, wb);
                                        }
                                    }
                            }
                        }

                        currIndex -= iRowsInterval * sdlSurface->w; // Move the buffer index up
                        limEnd -= iRowsInterval; // Move the height coordinate up
                    }
                    SDL_UnlockSurface(sdlSurface);

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

            #ifdef DEBUG
            if(abs(column - iColumnsCount / 2) <= iColumnsPerRay) {
                for(int i = 0; i < renderHeight; i++)
                    pixels[iScreenWidth / 2 + i * sdlSurface->w] = 0xffffff;
            }
            #endif
        }

        if(bIsCursorLocked)
            SDL_WarpMouseInWindow(sdlWindow, iScreenWidth / 2, iScreenHeight / 2);
        
        #ifdef DEBUG
        if(frameIndex % msFrameDuration == 0) {
            system("clear");
            float fps = 1.0f / getElapsedTime();
            cout << "Current FPS: " << (1.0f / getElapsedTime()) << "\n";
        }
        #endif

        /***********************************************/
        /***********************************************/
        /********** DISPLAY THE SCREEN BUFFER **********/
        /***********************************************/
        /***********************************************/

        SDL_UpdateWindowSurface(sdlWindow);
        SDL_Delay(msFrameDuration);
        frameIndex++;
        return bRun;
    }
}
