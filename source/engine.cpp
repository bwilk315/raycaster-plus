
#include "../include/engine.hpp"

namespace rp {    

    /*************************************************/
    /********** STRUCTURE: COLUMN DRAW INFO **********/
    /*************************************************/

    ColumnDrawInfo::ColumnDrawInfo() {
        this->perpDist = -1;
        this->localInter = Vector2::ZERO;
        this->wallDataPtr = nullptr;
    }
    ColumnDrawInfo::ColumnDrawInfo(float perpDist, const Vector2& localInter, const WallData* wallDataPtr) {
        this->perpDist = perpDist;
        this->localInter = localInter;
        this->wallDataPtr = wallDataPtr;
    }
    #ifdef DEBUG
    ostream& operator<<(ostream& stream, const ColumnDrawInfo& cdi) {
        stream << "ColumnDrawInfo(perpDist=" << cdi.perpDist << ", localInter=" << cdi.localInter;
        stream << ", wallData=" << *cdi.wallDataPtr << ")";
        return stream;
    }
    #endif

    /***********************************/
    /********** CLASS: ENGINE **********/
    /***********************************/

    const float Engine::SAFE_LINE_HEIGHT = 0.0001f;

    void Engine::updateSurface() {
        sdlSurface = SDL_GetWindowSurface(sdlWindow);
        pixels = (uint32_t*)sdlSurface->pixels;
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
        const float pcmDist = 1 / (2 * tan(mainCamera->getFieldOfView() / 2));
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
            memset(pixels, 0, sdlSurface->pitch * sdlSurface->h);
            SDL_UnlockSurface(sdlSurface);
            bClear = false;
        }
        // Skip drawing process if redrawing is not requested
        int column = bRedraw ? rRenderArea.x : (rRenderArea.x + rRenderArea.w);

        // Draw the current frame, which consists of pixel columns
        for( ; column < (rRenderArea.x + rRenderArea.w); column += iColumnsPerRay) {

            // Position of the ray on the camera plane, from -1 (left) to 1 (right)
            float cameraX = 2 * (column - rRenderArea.x) / (float)rRenderArea.w - 1;
            Vector2 rayDir = (camDir + planeVec * cameraX).normalized();

            // Perform step-based DDA algorithm to find out which tiles get hit by the ray, find
            // the nearest wall of the nearest tile.
            walker->init(camPos, rayDir);
            bool keepWalking = true;

            #ifdef DEBUG
            if(column == iScreenWidth / 2) {
               system("clear");
            }
            #endif
            
            // Occupied Height Ranges vector, used for determining if pixels can be drawn
            vector<pair<int, int>> exclRanges;

            while(keepWalking) {
                if(walker->rayFlag == DDA::RF_FAIL)
                    break;

                RayHitInfo hit = walker->next();

                if((walker->rayFlag & DDA::RF_TOO_FAR) || (walker->rayFlag & DDA::RF_OUTSIDE))
                    break;
                else if(!(walker->rayFlag & DDA::RF_HIT))
                    continue;

                // Compute the ray-tile intersection point, in local tile coordinates.
                // If hit distance is exactly 0 it indicates hit occurred inside the origin tile.
                Vector2 rayEnter;
                float localX = hit.point.x - (int)hit.point.x;
                float localY = hit.point.y - (int)hit.point.y;
                if(walker->rayFlag & DDA::RF_SIDE) {
                    rayEnter.x = !hit.distance ? localX : (rayDir.x < 0);
                    rayEnter.y = localY;
                } else {
                    rayEnter.x = localX;
                    rayEnter.y = !hit.distance ? localY : (rayDir.y < 0);
                }

                // Sort walls by their distance to the camera plane in ascending order (not done yet)

                int tileData = mainScene->getTileId(hit.tile.x, hit.tile.y);
                const vector<WallData>* wallData = mainScene->getTileWalls(tileData);
                if(wallData == nullptr)
                    continue;

                // Collect draw information as pointers to dedicated structure
                int wallCount = wallData->size();
                int dipLen = 0; // drawInfoPtrs length
                ColumnDrawInfo* drawInfoPtrs[wallCount]; // Contains nullptrs if ray misses a wall
                for(int i = 0; i != wallCount; i++) {
                    ColumnDrawInfo* cdi = new ColumnDrawInfo;
                    cdi->wallDataPtr = &wallData->at(i);

                    // COMPUTATION WITHOUT USING SQUARE ROOT (can be simplified further)
                    // The formula below was derived by parts, and compressed into one long computation
                    // NOTE: this formula works even when enter point is actually inside a tile
                    float a = cdi->wallDataPtr->func.slope;
                    float h = cdi->wallDataPtr->func.height;
                    float interDist = ( rayEnter.y - a * rayEnter.x - ( h == 0 ? SAFE_LINE_HEIGHT : h ) ) / ( rayDir.x * a - rayDir.y );

                    // Distance is negative when a wall is not reached by the ray, this and the fact that the longest
                    // distance in tile boundary is 1/sqrt(2), can be used to perform early classification.
                    if(interDist >= 0 && interDist <= INV_SQRT2) {
                        cdi->localInter = interDist * rayDir + rayEnter;

                        // Check if point is included in arguments and values range
                        if((cdi->localInter.x >= cdi->wallDataPtr->func.xMin && cdi->localInter.x <= cdi->wallDataPtr->func.xMax) &&
                           (cdi->localInter.y >= cdi->wallDataPtr->func.yMin && cdi->localInter.y <= cdi->wallDataPtr->func.yMax)) {

                            cdi->perpDist = rayDir.dot(camDir) * ( hit.distance + interDist );
                            drawInfoPtrs[i] = cdi;
                            dipLen++;
                            continue;
                        }
                    }

                    // Ray missed that wall, therefore its information is garbage, leave it unset
                    drawInfoPtrs[i] = nullptr;
                    delete cdi;
                }

                // Sort the information in ascending order, perform it on pointers to avoid copying
                for(int i = 0; i != wallCount; i++) {
                    for(int j = 1; j != wallCount; j++) {
                        if(drawInfoPtrs[j] != nullptr && (drawInfoPtrs[j - 1] == nullptr || drawInfoPtrs[j - 1]->perpDist > drawInfoPtrs[j]->perpDist)) {
                            ColumnDrawInfo* temp = drawInfoPtrs[j];
                            drawInfoPtrs[j] = drawInfoPtrs[j - 1];
                            drawInfoPtrs[j - 1] = temp;
                        } else {
                            break;
                        }
                    }
                }

                // First walls are valid pointers, nullptrs are left far away
                for(int i = 0; i != dipLen; i++) {
                    ColumnDrawInfo* cdi = drawInfoPtrs[i];

                    // Calculate a normal vector of the wall, it always points outwards it
                    bool flipped = false;
                    float a = cdi->wallDataPtr->func.slope;
                    float h = cdi->wallDataPtr->func.height;
                    float coef = 1 / sqrt( a * a + 1 );
                    Vector2 normal(a * coef, -1 * coef);
                    if(camPos.y >= a * (camPos.x - hit.tile.x) + hit.tile.y + h) {
                        normal *= -1;
                        flipped = true;
                    }

                    // Find out range describing how column should be drawn for the current wall
                    int lineHeight  = rRenderArea.h * (pcmDist / cdi->perpDist);
                    int drawStart   = rRenderArea.y + (rRenderArea.h + lineHeight) / 2 - lineHeight * cdi->wallDataPtr->hMin;
                    int drawEnd     = rRenderArea.y + (rRenderArea.h - lineHeight) / 2 + lineHeight * (1 - cdi->wallDataPtr->hMax);
                    int startClip   = drawStart > rRenderArea.y + rRenderArea.h ? (drawStart - rRenderArea.y - rRenderArea.h) : 0;
                    int totalHeight = drawStart - drawEnd;

                    SDL_LockSurface(sdlSurface);

                    const Texture* tex = mainScene->getTextureSource(cdi->wallDataPtr->texId);
                    bool isSolidColor = tex == nullptr;

                    int texHeight = isSolidColor ? 1 : tex->getHeight();
                    // Compute normalized horizontal position on the wall plane
                    float planeHorizontal = (cdi->localInter - cdi->wallDataPtr->pivot).magnitude() / cdi->wallDataPtr->length;
                    if(flipped)
                        planeHorizontal = 1 - planeHorizontal;

                    // Draw column using drawable information
                    int dbStart = drawStart - startClip;
                    int dbEnd = drawEnd < rRenderArea.y ? rRenderArea.y : drawEnd;
                    int exclCount = exclRanges.empty() ? 0 : exclRanges.size();
                    int exclStart = -1;  // Next exclusion range start coordinate
                    int exclEnd   = -1;
                    int exclIndex = 0;

                    // Find the next exclusion range index, set up initial free drawable height coordinates
                    for(int e = 0; e < exclCount; e++) {
                        const pair<int, int>& range = exclRanges.at(e);
                        // If drawable height is in some exclusion, move it
                        if(dbStart <= range.first && dbStart > range.second) {
                            dbStart = range.second;
                        } else if(dbEnd <= range.first && (dbEnd > range.second || range.second == 0)) {
                            dbEnd = range.first;
                        }
                        // If this range is above the drawable height coordinate
                        if(range.first <= dbStart) {
                            exclStart = range.first;
                            exclEnd   = range.second;
                            exclIndex = e;
                            // Make sure starting coordinate is not included inside the previous exclusion range
                            if(e != 0) {
                                int change = dbStart - exclRanges.at(e - 1).second;
                                dbStart -= clamp(change, 0, rRenderArea.h);
                                // Loop must go over them all again to ensure that moved coordinate did not fell into
                                // other exclusion range.
                                if(change > 0) {
                                    e = 0;
                                    continue;
                                }
                            }
                            break;
                        }
                    }
                    
                    // THIS IS NOT WORKING PROPERLY, IT IS MESSED UP ...
                    for(int h = dbStart; h > dbEnd; h -= iRowsInterval) {

                        // Check for touching exclusion range
                        if(h == exclStart) {
                            h = exclEnd; // Jump over it
                            // Update next exclusion start and end coordinate, if there is any
                            if(++exclIndex < exclCount) {
                                const pair<int, int>& range = exclRanges.at(exclIndex);
                                exclStart = range.first;
                                exclEnd   = range.second;
                            } else {
                                exclStart = -1;
                                exclEnd = -1;
                            }
                        }

                        // Obtain a current pixel color
                        uint8_t r, g, b, a;
                        uint32_t color;
                        if(isSolidColor) {
                            decodeRGBA(cdi->wallDataPtr->tint, r, g, b, a);
                        } else {
                            float planeVertical = (drawStart - h) / (float)totalHeight;
                            decodeRGBA(tex->getCoords(planeHorizontal, planeVertical), r, g, b, a);
                        }

                        // Apply lightning to the color
                        if(bLightEnabled) {
                            const float minBn = 0.2f;
                            float perc = -1 * (normal.dot(vLightDir) - 1) / 2; // Brightness linear interploation percent
                            float bn = minBn + (1 - minBn) * perc; // Final brightness
                            r *= bn;
                            g *= bn;
                            b *= bn;
                        }

                        // Draw pixel to the buffer
                        color = SDL_MapRGB(sdlSurface->format, r, g, b);
                        for(int c = 0; c != iColumnsPerRay; c++)
                            for(int r = 0; r != iRowsInterval; r++)
                                pixels[c + column + (h + r) * sdlSurface->h] = color;

                    }

                    // Invalid ranges are not allowed
                    if(dbStart > dbEnd) {
                        auto newRange = pair<int, int>(dbStart, dbEnd);
                        if(exclCount == 0) {
                            exclRanges.push_back(newRange);
                        } else {
                            bool gotIt = false;
                            for(int e = 0; e < exclCount; e++) {
                                if(newRange.first >= exclRanges.at(e).first) {
                                    exclRanges.insert(exclRanges.begin() + e, newRange);
                                    gotIt = true;
                                    break;
                                }
                            }
                            if(!gotIt)
                                exclRanges.push_back(newRange);
                        }
                    }

                    #ifdef DEBUG
                    if(column == iScreenWidth / 2) {
                        exclCount = exclRanges.size();
                        cout << "For wall " << i << endl;
                        for(int e = 0; e < exclCount; e++) {
                            auto range = exclRanges.at(e);
                            cout << "At " << e << ": From " << range.first << " to " << range.second << endl;
                        }
                    }
                    #endif

                    SDL_UnlockSurface(sdlSurface);

                    delete cdi;
                    if(cdi->wallDataPtr->stopsRay) {
                        keepWalking = false;
                        break;
                    }

                }

            }

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
