
#include "../include/engine.hpp"

namespace rp {    

    /***********************************/
    /********** CLASS: ENGINE **********/
    /***********************************/

    const float Engine::MAX_LINE_SLOPE = 1e4f;

    void Engine::updateSurface() {
        sdlSurface = SDL_GetWindowSurface(sdlWindow);
        pixels = (uint32_t*)sdlSurface->pixels;
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
        this->bClear             = false;
        this->bIsCursorLocked    = false;
        this->bLightEnabled      = false;
        this->bRedraw            = false;
        this->bRun               = true;
        this->iError             = E_CLEAR;
        this->iColumnsPerRay     = 0;
        this->iFramesPerSecond   = 0;
        this->iRowsInterval      = 0;
        this->iScreenWidth       = screenWidth;
        this->iScreenHeight      = screenHeight;
        this->fAspectRatio       = screenHeight / (float)screenWidth;
        this->frameIndex         = 0;
        this->renderFitMode      = RenderFitMode::UNKNOWN;
        this->vLightDir          = Vector2::RIGHT;
        this->tpLast             = system_clock::now();
        this->elapsedTime        = duration<float>(0);
        this->rRenderArea        = {};
        this->keyStates          = map<int, KeyState>();

        if(SDL_InitSubSystem(SDL_INIT_VIDEO) == 0) {
            this->pixels     = nullptr;
            this->mainCamera = nullptr;
            this->walker     = new DDA();
            this->sdlSurface = nullptr;
            this->sdlWindow  = SDL_CreateWindow("Raycaster Plus Engine", 0, 0, screenWidth, screenHeight, SDL_WINDOW_SHOWN);
            if(sdlWindow != nullptr) {
                SDL_SetWindowResizable(this->sdlWindow, SDL_FALSE);
                updateSurface();
                return;
            }
        }
        iError |= E_SDL;
    }
    Engine::~Engine() {
        if(walker != nullptr)
            delete walker;

        // SDL owns pixel and surface data, so no need to free it by hand
        if(iError != E_SDL) {
            SDL_DestroyWindowSurface(sdlWindow);
            SDL_DestroyWindow(sdlWindow);
            SDL_QuitSubSystem(SDL_INIT_VIDEO);
            SDL_Quit();
        }
    }
    void Engine::clear() {
        bClear = true;
    }
    void Engine::stop() {
        bRun = false;
    }
    void Engine::setCursorLock(bool locked) {
        bIsCursorLocked = locked;
    }
    void Engine::setCursorVisibility(bool visible) {
        SDL_ShowCursor(visible);
    }
    void Engine::setColumnsPerRay(int n) {
        if(rRenderArea.w == 0) {
            iError |= E_WRONG_CALL_ORDER;
            return;
        }
        iColumnsPerRay = clamp(n, 1, rRenderArea.w);
    }
    void Engine::setFrameRate(int fps) {
        iFramesPerSecond = fps < 1 ? 1 : fps;
    }
    void Engine::setLightBehavior(bool enabled, float angle) {
        bLightEnabled = enabled;
        vLightDir = (Vector2::RIGHT).rotate(angle);
    }
    void Engine::setMainCamera(const Camera* camera) {
        mainCamera = camera;
    }
    void Engine::setRowsInterval(int n) {
        if(rRenderArea.h == 0) {
            iError |= E_WRONG_CALL_ORDER;
            return;
        }
        iRowsInterval = clamp(n, 1, rRenderArea.h);
    }
    void Engine::setRenderFitMode(const RenderFitMode& rfm) {
        if(mainCamera == nullptr && rfm != RenderFitMode::UNKNOWN) {
            iError |= E_MAIN_CAMERA_NOT_SET;
            return;
        }
        renderFitMode = rfm;
        switch(rfm) {
            case RenderFitMode::STRETCH:
                rRenderArea = { 0, 0, iScreenWidth, iScreenHeight };
                break;
            case RenderFitMode::SQUARE:
                // Set up horizontal and vertical offsets for drawing columns to make the rendered
                // frame always form a square.
                if(iScreenWidth > iScreenHeight)
                    rRenderArea = {
                        (iScreenWidth - iScreenHeight) / 2,
                        0,
                        iScreenHeight,
                        iScreenHeight
                    };
                else
                    rRenderArea = {
                        0,
                        (iScreenHeight - iScreenWidth) / 2,
                        iScreenWidth,
                        iScreenWidth
                    };
                break;
        }
    }
    void Engine::render() {
        bRedraw = true;
    }
    int Engine::getError() const {
        return iError;
    }
    int Engine::getFrameCount() const {
        return frameIndex;
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
    SDL_Rect Engine::getRenderArea() const {
        return rRenderArea;
    }
    KeyState Engine::getKeyState(int sc) const {
        return keyStates.count(sc) == 0 ? KeyState::NONE : keyStates.at(sc);
    }
    DDA* const Engine::getWalker() {
        return walker;
    }
    SDL_Window* Engine::getWindowHandle() {
        return sdlWindow;
    }
    bool Engine::tick() {
        if(iError) {
            stop();
            return false;
        }

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
            }
        }

        /********************************************/
        /********************************************/
        /********** FILL THE SCREEN BUFFER **********/
        /********************************************/
        /********************************************/

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

        // Clear the entire screen buffer if requested
        if(bClear) {
            SDL_LockSurface(sdlSurface);
            SDL_FillRect(sdlSurface, NULL, 0);
            SDL_UnlockSurface(sdlSurface);
            bClear = false;
        }
        // Skip drawing process if redrawing is not requested

        // WRONG STH WITH COLUMNS?!!?!?!!?

        int column = bRedraw ? rRenderArea.x : (rRenderArea.x + rRenderArea.w);

        // Draw the current frame, which consists of pixel columns
        for( ; column < (rRenderArea.x + rRenderArea.w); column += iColumnsPerRay) {

            // Position of the ray on the camera plane, from -1 (left) to 1 (right)
            float cameraX = 2 * (column - rRenderArea.x) / (float)rRenderArea.w - 1;
            Vector2 rayDir = (camDir + planeVec * cameraX).normalized();
            LinearFunc rayLine;  // Line equation describing the ray walk
            // Ray slope stays constant for all hits
            rayLine.slope = (rayDir.x == 0) ? (MAX_LINE_SLOPE) : (rayDir.y / rayDir.x);

            // Perform step-based DDA algorithm to find out which tiles get hit by the ray, find
            // the nearest wall of the nearest tile.
            walker->init(camPos, rayDir);
            bool keepWalking = true;
            bool originDone = false; // Whether walls from the origin were examined
            bool wasAnyHit = false;

            while(keepWalking) {
                if(walker->rayFlag == DDA::RF_FAIL)
                    break;

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
                int tileData = mainScene->getTileId(hit.tile.x, hit.tile.y);

                // IT WORKS BUT ... THIS HAVE TO BE CHANGED IN THE FUTURE FOR KNOWN REASONS ...
                // Compute additional information of the walls, store them in a map that will sort them by distance
                vector<WallData> details = mainScene->getTileWalls(tileData);

                map<float, pair<int, Vector2>> additional; // Key: distance, Value: ( Key: index, Value: intersection )

                int reps = 0; // Wall line repetitions
                for(int i = 0; i < details.size(); i++) {
                    WallData* wdp = &details.at(i);
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
                    planeDist = (int)(planeDist * 10000) / 10000.0f; // simple rounding

                    // Plane distance must be unique, because it is a key of map
                    if(additional.count(planeDist) != 0)
                        planeDist += 1e-5f * ++reps;

                    additional.insert(pair<float, pair<int, Vector2>>(planeDist, pair<int, Vector2>(i, inter)));
                }

                // Draw buffer
                for(const auto& extra : additional) {

                    const float planeDist = extra.first;
                    const Vector2 inter = extra.second.second;
                    const WallData wd = details.at(extra.second.first);

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
                    int lineHeight  = rRenderArea.h * (pcmDist / planeDist);
                    int drawStart   = (rRenderArea.h + lineHeight) / 2 - lineHeight * wd.hMin;
                    int drawEnd     = (rRenderArea.h - lineHeight) / 2 + lineHeight * (1 - wd.hMax);
                    int startClip   = drawStart > rRenderArea.h ? (drawStart - rRenderArea.h) : 0;
                    int totalHeight = drawStart - drawEnd;

                    const Texture* tex = mainScene->getTextureSource(wd.texId);
                    int texHeight = tex == nullptr ? 1 : tex->getHeight();
                    // Compute normalized horizontal position on the wall plane
                    float planeNormX = ((isNormalFlipped ? wd.bp1 : wd.bp0) - inter).magnitude() / (wd.bp1 - wd.bp0).magnitude();

                    SDL_LockSurface(sdlSurface);
                    for(int h = startClip; h < totalHeight; h += iRowsInterval) {
                        int currHeight = drawStart - h;
                        if(currHeight < 0) // Rows interval may lead to this being real
                            break;

                        uint8_t wr, wg, wb, wa; // Wall color
                        decodeRGBA(
                            (tex == nullptr) ? wd.tint : tex->getCoords(planeNormX, h / (float)totalHeight),
                            wr, wg, wb, wa
                        );

                        if(wa != 0) {
                            // column does not need to be shifted by `rRedrawArea.x` because it is already
                            int currIndex = column + (currHeight + rRenderArea.y) * sdlSurface->w;
                            uint8_t cr, cg, cb, ca; // Current color
                            decodeRGBA(pixels[currIndex], cr, cg, cb, ca);

                            // Set screen buffer pixel if it is not already (meaning it is pure black),
                            // This trick is only possible because texture loader is made to alter black to not be so pure.
                            if(cr + cg + cb < MIN_CHANNEL) {

                                // Apply shading if light source is enabled
                                if(bLightEnabled) {
                                    const float minBn = 0.2f;
                                    float perc = -1 * (normal.dot(vLightDir) - 1) / 2; // Brightness linear interploation percent
                                    float bn = minBn + (1 - minBn) * perc; // Final brightness
                                    wr *= bn;
                                    wg *= bn;
                                    wb *= bn;
                                }

                                for(int i = 0; i < iColumnsPerRay; i++) {
                                    if(column + i == rRenderArea.h)
                                        break;
                                    for(int j = 0; j < iRowsInterval; j++) {
                                        if(currHeight - j == -1)
                                            break;
                                        // Use SDL-provided function for converting RGBA color in appropriate way to a single number
                                        pixels[currIndex + i - j * sdlSurface->w] = SDL_MapRGB(
                                            sdlSurface->format,
                                            clamp(wr, MIN_CHANNEL, 255),
                                            clamp(wg, MIN_CHANNEL, 255),
                                            clamp(wb, MIN_CHANNEL, 255)
                                        );
                                    }
                                }
                            }
                        }
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
            if(abs(column - iScreenWidth / 2) <= iColumnsPerRay) {
                for(int i = rRenderArea.y; i < rRenderArea.h + rRenderArea.y; i++)
                    pixels[iScreenWidth / 2 + i * sdlSurface->w] = 0xffffff;
            }
            #endif
        }

        if(bIsCursorLocked)
            SDL_WarpMouseInWindow(sdlWindow, iScreenWidth / 2, iScreenHeight / 2);
        
        /***********************************************/
        /***********************************************/
        /********** DISPLAY THE SCREEN BUFFER **********/
        /***********************************************/
        /***********************************************/

        int delay = (1.0f / iFramesPerSecond - elapsedTime.count()) * 1000;
        delay = delay < 0 ? 0 : delay;

        #ifdef DEBUG
        if(frameIndex % iFramesPerSecond == 0) {
            // system("clear");
            // cout << "Delay [ms]: " << delay << "\n";
        }
        #endif
        
        SDL_Delay(delay); // Maybe this causes the lag when unfreezing?

        if(bRedraw) {
            SDL_UpdateWindowSurface(sdlWindow);
            bRedraw = false;
        }
        frameIndex++;
        return bRun;
    }
}
