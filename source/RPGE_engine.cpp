
#include <RPGE_engine.hpp>

namespace rpge {    

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

    Engine::Engine(int screenWidth, int screenHeight) {
        this->bClear             = false;
        this->bIsCursorLocked    = false;
        this->bLightEnabled      = false;
        this->bRedraw            = false;
        this->bRun               = true;
        this->iError             = E_CLEAR;
        this->iColumnsPerRay     = 1;
        this->iFramesPerSecond   = 60;
        this->iRowsInterval      = 1;
        this->iScreenWidth       = screenWidth < 1 ? 1 : screenWidth;
        this->iScreenHeight      = screenHeight < 1 ? 1 : screenHeight;
        this->fAspectRatio       = iScreenHeight / (float)iScreenWidth;
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
                this->sdlSurface = SDL_GetWindowSurface(sdlWindow);
                if(sdlSurface != nullptr) {
                    pixels = (uint32_t*)sdlSurface->pixels;
                    return;
                }
            }
        }
        iError |= E_SDL;
    }
    Engine::~Engine() {
        if(walker != nullptr)
            delete walker;
        if(sdlWindow != nullptr)
            SDL_DestroyWindow(sdlWindow);
        if(iError != E_SDL) {
            SDL_QuitSubSystem(SDL_INIT_VIDEO);
            SDL_Quit();
        }
    }
    void Engine::clear() {
        bClear = true;
    }
    const SDL_PixelFormat* Engine::getColorFormat() const {
        return sdlSurface == nullptr ? nullptr : sdlSurface->format;
    }
    void Engine::stop() {
        bRun = false;
    }
    void Engine::setCursorLock(bool locked) {
        bIsCursorLocked = locked;
    }
    void Engine::setCursorVisibility(bool visible) {
        if(SDL_ShowCursor(visible) < 0)
            iError |= E_SDL;
    }
    void Engine::setColumnsPerRay(int n) {
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
    DDA* Engine::getWalker() {
        return walker;
    }
    SDL_Window* Engine::getWindowHandle() {
        return sdlWindow;
    }
    bool Engine::tick() {
        if(iError) {
            stop();
            return bRun;
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


        const int rhStart = rRenderArea.y;
        const int rhEnd   = rRenderArea.y + rRenderArea.h;
        // Perspective-correct minimum distance; if you stand this distance from the cube looking at it orthogonally,
        // entire vertical view of the camera should be occupied by the cube front wall. This assumes that camera is
        // located at height of 1/2.
        const float pcmDist = 1 / (2 * tan(mainCamera->getFieldOfView() / 2));
        Scene* mainScene = walker->getTargetScene();
        Vector2 camDir   = mainCamera->getDirection();
        Vector2 camPos   = mainCamera->getPosition();
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
        #ifdef DEBUG
        bool centerDebugDone = false;
        #endif
        for( ; column < (rRenderArea.x + rRenderArea.w); column += iColumnsPerRay) {

            // Drawing exclusions for the current pixel column encoded in key-value pair (start-end heights in screen coordinates)
            vector<pair<int, int>> drawExcls;
            bool keepWalking = true;

            // Position of the ray on the camera plane, from -1 (leftmost) to 1 (rightmost)
            float cameraX  = 2 * (column - rRenderArea.x) / (float)rRenderArea.w - 1;
            Vector2 rayDir = (camDir + planeVec * cameraX).normalized();

            walker->init(camPos, rayDir);
            while(keepWalking) {


                /****************************************************/
                /********** DDA-BASED RAY WALK PERFORMANCE **********/
                /****************************************************/


                RayHitInfo hit = walker->next();
                if( walker->rayFlag & (DDA::RF_TOO_FAR | DDA::RF_OUTSIDE | DDA::RF_FAIL) )
                    break;
                else if( !(walker->rayFlag & DDA::RF_HIT) )
                    continue;

                // Compute the ray-tile intersection point in local tile coordinates, keep it pivoted to the bottom-left corner
                // of a tile when looking at it from the top. If hit distance is exactly 0 it indicates that hit occurred inside
                // the origin tile.
                Vector2 localEnter;
                float localX = hit.point.x - (int)hit.point.x;
                float localY = hit.point.y - (int)hit.point.y;
                if(walker->rayFlag & DDA::RF_SIDE) {
                    localEnter.x = !hit.distance ? localX : (rayDir.x < 0);
                    localEnter.y = localY;
                } else {
                    localEnter.x = localX;
                    localEnter.y = !hit.distance ? localY : (rayDir.y < 0);
                }

                // Obtain collection of walls defined for the hit tile, if there are any
                int tileId = mainScene->getTileId(hit.tile.x, hit.tile.y);
                const vector<WallData>* wallData = mainScene->getTileWalls(tileId);
                if(wallData == nullptr)
                    continue;


                /************************************************************************/
                /********** PREPARATION OF INFORMATION REQUIRED TO DRAW COLUMN **********/
                /************************************************************************/


                // Collect draw information as pointers to dedicated structure
                int wallCount = wallData->size();
                // Tells how many first items of `drawInfoPtrs` array are not null pointers
                int dipLen    = 0;
                // Draw information is stored as pointers because they are more efficient to sort later
                ColumnDrawInfo* drawInfoPtrs[wallCount];
                for(int i = 0; i != wallCount; i++) {
                    ColumnDrawInfo* cdi = new ColumnDrawInfo;
                    cdi->wallDataPtr = &wallData->at(i);

                    // The formula below was derived by parts, and compressed into one long computation
                    // NOTE: this formula works even when enter point is actually inside a tile.
                    float a         = cdi->wallDataPtr->func.slope;
                    float h         = cdi->wallDataPtr->func.height;
                    float interDist = ( localEnter.y - a * localEnter.x - ( h == 0 ? SAFE_LINE_HEIGHT : h ) ) / ( rayDir.x * a - rayDir.y );

                    // Distance is negative when a wall is not reached by the ray, this and the fact that the longest
                    // distance in tile boundary is 1/sqrt(2), can be used to perform early classification.
                    if(interDist >= 0 && interDist <= SQRT2) {
                        cdi->localInter = interDist * rayDir + localEnter;

                        // Check if point is included in arguments and values range defined
                        if((cdi->localInter.x >= cdi->wallDataPtr->func.xMin && cdi->localInter.x <= cdi->wallDataPtr->func.xMax) &&
                           (cdi->localInter.y >= cdi->wallDataPtr->func.yMin && cdi->localInter.y <= cdi->wallDataPtr->func.yMax)) {

                            cdi->perpDist = rayDir.dot(camDir) * ( hit.distance + interDist );
                            drawInfoPtrs[dipLen++] = cdi;
                            continue;
                        }
                    }

                    // Ray missed the wall, therefore its information is garbage
                    delete cdi;
                }
                if(dipLen == 0)
                    continue;

                // Perform bubble sort on the drawing information in ascending order, sort by distance from the camera plane
                for(int i = 0; i != dipLen; i++) {
                    for(int j = 1; j != dipLen; j++) {
                        if(drawInfoPtrs[j - 1]->perpDist > drawInfoPtrs[j]->perpDist) {
                            ColumnDrawInfo* temp = drawInfoPtrs[j];
                            drawInfoPtrs[j]      = drawInfoPtrs[j - 1];
                            drawInfoPtrs[j - 1]  = temp;
                        }
                    }
                }


                /******************************************************************************/
                /********** COLUMN DRAWING USING COLLECTED WALLS DRAWING INFORMATION **********/
                /******************************************************************************/


                for(int i = 0; i != dipLen; i++) {
                    ColumnDrawInfo* cdi = drawInfoPtrs[i];

                    // Calculate a normal vector of the wall, it always points outwards
                    bool flipped = false;
                    float a      = cdi->wallDataPtr->func.slope;
                    float h      = cdi->wallDataPtr->func.height;
                    float coef   = 1 / sqrt( a * a + 1 );
                    Vector2 normal(a * coef, -1 * coef);
                    if(camPos.y >= a * (camPos.x - hit.tile.x) + hit.tile.y + h) {
                        normal *= -1;
                        flipped = true;
                    }

                    // Find out range describing how column should be drawn for the current wall
                    int lineHeight = rRenderArea.h * (pcmDist / cdi->perpDist);
                    int drawStart  = rhStart + (rRenderArea.h - lineHeight) / 2 + lineHeight * (1 - cdi->wallDataPtr->hMax);
                    int drawEnd    = rhStart + (rRenderArea.h + lineHeight) / 2 - lineHeight * cdi->wallDataPtr->hMin;

                    // Prepare for examining exclusion ranges, find drawable start and height coordinates (meaning they
                    // are able to be displayed in the render area).
                    bool isVisible = true;
                    int dbStart    = clamp(drawStart, rhStart, rhEnd);
                    int dbEnd      = clamp(drawEnd,   rhStart, rhEnd);
                    int exclCount  = drawExcls.size();
                    int jumpIndex  = -1;     // Index of exclusion that is going to limit drawing first
                    pair<int, int> jumpExcl; // Initial content at the index `jumpIndex`, it gets dynamically updated later

                    // Find index of the first exclusion limiting the drawing range, if there is any
                    for(int e = 0; e < exclCount; e++) {
                        const pair<int, int>& excl = drawExcls.at(e);
                        bool startIn = excl.first  > dbStart && excl.first  < dbEnd;
                        bool endIn   = excl.second > dbStart && excl.second < dbEnd;

                        if(startIn && endIn) {
                            // Exclusion is included in drawing range.
                            // Collect its index and finish the loop, it is later used for checking next ones only because
                            // the exclusions vector is sorted.
                            jumpIndex = e;
                            jumpExcl  = excl;
                            break;
                        } else if(startIn) {
                            // Only exclusion start is included.
                            // Shrink the bottom coordinate and perform the loop over again to make sure new coordinate does
                            // not interfere with anything.
                            dbEnd = excl.first;
                            e = 0;
                        } else if(endIn) {
                            // Only exclusion end is included.
                            // Similarly to the `startIn` only (above), shrink the top coordinate and loop over again.
                            dbStart = excl.second;
                            e = 0;
                        } else if(excl.first <= dbStart && excl.second >= dbEnd) {
                            // Exclusion eats the whole drawing range.
                            // Therefore the column is not visible by any mean, so skip it.
                            isVisible = false;
                            break;
                        }
                    }

                    if(isVisible) {

                        const Texture* tex = mainScene->getTextureSource(cdi->wallDataPtr->texId);
                        int totalHeight    = drawEnd - drawStart;
                        bool isSolidColor  = tex == nullptr;

                        // Compute normalized horizontal position on the wall plane
                        float planeHorizontal = (cdi->localInter - cdi->wallDataPtr->pivot).magnitude() / cdi->wallDataPtr->length;
                        if(flipped)
                            planeHorizontal = 1 - planeHorizontal;

                        SDL_LockSurface(sdlSurface);
                        for(int h = dbStart; h < dbEnd; h++) {

                            // Perform jumping if neccessary, if it is then find the next exclusion range below the current one
                            if(jumpIndex != -1 && h >= jumpExcl.first && h <= jumpExcl.second) {
                                h = jumpExcl.second;
                                while(++jumpIndex < exclCount && jumpExcl.second <= h)
                                    jumpExcl = drawExcls.at(jumpIndex);
                                continue;
                            }
                            // Actual drawing is done according to rows interval setting, however it is implemented in this specific
                            // form to properly recognize when loop enters exclusion range (if statement above evaluates to true).
                            if(h % iRowsInterval != 0)
                                continue;

                            // Obtain the current texture pixel color if there is any texture, otherwise use the wall `tint`
                            uint8_t tr, tg, tb, ta;
                            if(isSolidColor) {
                                SDL_GetRGBA(cdi->wallDataPtr->tint, sdlSurface->format, &tr, &tg, &tb, &ta);
                            } else {
                                float planeVertical = 1.0f - (h - drawStart) / (float)totalHeight;
                                SDL_GetRGBA(tex->getCoords(planeHorizontal, planeVertical), sdlSurface->format, &tr, &tg, &tb, &ta);
                            }

                            // Apply lightning to the color
                            if(bLightEnabled) {
                                const float minBn = 0.2f;
                                float perc = -1 * (normal.dot(vLightDir) - 1) / 2; // Brightness linear interploation percent
                                float bn = minBn + (1 - minBn) * perc;             // Final brightness
                                tr *= bn;
                                tg *= bn;
                                tb *= bn;
                            }

                            // Draw pixel(s) to the buffer. Amount of drawn pixels depends on columns-per-ray and rows interval settings
                            uint32_t color = SDL_MapRGB(sdlSurface->format, tr, tg, tb);
                            for(int c = 0; c != iColumnsPerRay; c++) {
                                int hor = c + column;
                                if(hor < 0 || hor >= sdlSurface->w)
                                    break;
                                for(int r = 0; r != iRowsInterval; r++) {
                                    int ver = h + r - 1;
                                    if(ver < 0 || ver >= sdlSurface->h)
                                        break;
                                    
                                    pixels[hor + ver * sdlSurface->w] = color;
                                }
                            }
                        }

                        SDL_UnlockSurface(sdlSurface);

                        // Append new exclusion, do it so vector remains sorted according to exclusions' start coordinate
                        if(dbStart != dbEnd) {
                            int index = exclCount == 0 ? 0 : exclCount;
                            for(int e = 0; e < exclCount; e++) {
                                if(dbStart <= drawExcls.at(e).first) {
                                    index = e;
                                    break;
                                }
                            }
                            drawExcls.insert(drawExcls.begin() + index, make_pair(dbStart, dbEnd));
                            exclCount++;
                        }
                    }

                    // Decide if ray should keep on walking, free column drawing information because it was already used
                    if(cdi->wallDataPtr->stopsRay)
                        keepWalking = false;
                    delete cdi;
                    if(!keepWalking)
                        break;
                }
            }

            #ifdef DEBUG
            
            // Draw exclusions start and end coordinates in debugging purposes
            SDL_UnlockSurface(sdlSurface);
            for(const pair<int, int>& excl : drawExcls) {
                for(int c = 0; c < iColumnsPerRay; c++) {
                    int hor = c + column;
                    if(hor < 0 || hor >= sdlSurface->w)
                        break;
                    for(int r = 0; r < iRowsInterval; r++) {

                        int verS = excl.first + r - 1;
                        if(verS < 0 || verS >= sdlSurface->h)
                            break;
                        pixels[hor + verS * sdlSurface->w] = SDL_MapRGB(sdlSurface->format, 0, 255, 0);

                        int verE = excl.second + r - 1;
                        if(verE < 0 || verE >= sdlSurface->h)
                            break;
                        pixels[hor + verE * sdlSurface->w] = SDL_MapRGB(sdlSurface->format, 255, 0, 0);

                    }
                }
            }
            SDL_LockSurface(sdlSurface);

            // Perform one-time actions when column is nearly at the center of the screen
            if(!centerDebugDone && abs(column - iScreenWidth / 2) <= iColumnsPerRay) {
                for(int i = rRenderArea.y; i < rRenderArea.h + rRenderArea.y; i++)
                    pixels[iScreenWidth / 2 + i * sdlSurface->w] = 0xffffff;
                centerDebugDone = true;
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
            //system("clear");
            //cout << "Delay [ms]: " << delay << "\n";
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
