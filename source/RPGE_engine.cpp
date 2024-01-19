
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
        #ifdef DEBUG
        bool centerDebugDone = false;
        #endif
        for( ; column < (rRenderArea.x + rRenderArea.w); column += iColumnsPerRay) {

            // Position of the ray on the camera plane, from -1 (left) to 1 (right)
            float cameraX = 2 * (column - rRenderArea.x) / (float)rRenderArea.w - 1;
            Vector2 rayDir = (camDir + planeVec * cameraX).normalized();

            // Perform step-based DDA algorithm to find out which tiles get hit by the ray, find
            // the nearest wall of the nearest tile.
            walker->init(camPos, rayDir);
            bool keepWalking = true;
            
            // YOU MUST DEPENDIZE THIS BELOW ON TILE ORDER, FOR NOW IT DOES NOT TAKE IT INTO ACCOUNT,
            // THEREFORE TILES THAT ARE NEAR ARE NOT DRAWN BC FARER ONE IS CONSIDERED FIRST INSTEAD

            // Vector of pairs indicating draw height exclusions as global pixel coordinate (key: start, value: end)
            vector<pair<int, int>> drawExcls;

            #ifdef DEBUG ///////////////////////////////////////////
            if(column == iScreenWidth/2) {
                system("clear");
            }
            #endif

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
                int dipLen = 0; // drawInfoPtrs length (used also as index)
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
                    if(interDist >= 0 && interDist <= SQRT2) {
                        cdi->localInter = interDist * rayDir + rayEnter;


                        // Check if point is included in arguments and values range
                        if((cdi->localInter.x >= cdi->wallDataPtr->func.xMin && cdi->localInter.x <= cdi->wallDataPtr->func.xMax) &&
                           (cdi->localInter.y >= cdi->wallDataPtr->func.yMin && cdi->localInter.y <= cdi->wallDataPtr->func.yMax)) {

                            cdi->perpDist = rayDir.dot(camDir) * ( hit.distance + interDist );
                            drawInfoPtrs[dipLen++] = cdi;

                            continue;
                        }
                    }

                    // Ray missed that wall, therefore its information is garbage
                    delete cdi;
                }

                if(dipLen == 0)
                    continue;

                // SOLUTION: MAKE STRUCTURE THAT TAKES TILE INTO ACCOUNT (I.E. NOT BY DETECTION OF THE LOWER BOUND)
                // Sort the information in ascending order, perform it on pointers to avoid copying
                for(int i = 0; i != dipLen; i++) {
                    for(int j = 1; j != dipLen; j++) {
                        if(drawInfoPtrs[j - 1]->perpDist > drawInfoPtrs[j]->perpDist) {
                            ColumnDrawInfo* temp = drawInfoPtrs[j];
                            drawInfoPtrs[j] = drawInfoPtrs[j - 1];
                            drawInfoPtrs[j - 1] = temp;
                        }
                    }
                }
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
                    int drawStart   = rhStart + (rRenderArea.h - lineHeight) / 2 + lineHeight * (1 - cdi->wallDataPtr->hMax);
                    int drawEnd     = rhStart + (rRenderArea.h + lineHeight) / 2 - lineHeight * cdi->wallDataPtr->hMin;
                    int startClip   = drawEnd > rhEnd ? (drawEnd - rhEnd) : 0;
                    int totalHeight = drawEnd - drawStart;

                    SDL_LockSurface(sdlSurface);
                    
                    const Texture* tex = mainScene->getTextureSource(cdi->wallDataPtr->texId);
                    bool isSolidColor = tex == nullptr;

                    int texHeight = isSolidColor ? 1 : tex->getHeight();
                    // Compute normalized horizontal position on the wall plane
                    float planeHorizontal = (cdi->localInter - cdi->wallDataPtr->pivot).magnitude() / cdi->wallDataPtr->length;
                    if(flipped)
                        planeHorizontal = 1 - planeHorizontal;

                    // Draw column using drawable information
                    int exclCount = drawExcls.empty() ? 0 : drawExcls.size();
                    int dbStart = clamp(drawStart, rhStart, rhEnd);
                    int dbEnd   = clamp(drawEnd - startClip, rhStart, rhEnd);

                    // Find  out jumping information
                    map<int, int> exclJumps; // insane but works
                    bool isVisible = true;
                    for(int e = 0; e < exclCount; e++) {
                        const pair<int, int>& excl = drawExcls.at(e);
                        bool inclStart = excl.first  >= dbStart && excl.first  <= dbEnd;
                        bool inclEnd   = excl.second >= dbStart && excl.second <= dbEnd;
                        if(inclStart && inclEnd) {
                            // Exclusion is included in drawing range
                            exclJumps.insert(make_pair(excl.first, excl.second));
                        } else if(inclStart) {
                            // Only start is included
                            dbEnd = excl.first;
                        } else if(inclEnd) {
                            // Only end is included
                            dbStart = excl.second;
                        } else if(excl.first <= dbStart && excl.second >= dbEnd) {
                            // Exclusion eats the whole drawing range
                            isVisible = false;
                            break;
                        } else {
                            // Exclusion is not touching the drawing range at all
                        }
                    }
                    
                    if(isVisible) {
                        #ifdef DEBUG ///////////////////////////////////////////
                        if(column == iScreenWidth / 2) {
                        }
                        #endif

                        for(int h = dbStart; h < dbEnd; h += iRowsInterval) {

                            // THIS IS CRAZY APPROACH BUT IT WORKS, IT MUST BE OPTIMIZED
                            // Perform jumping if neccessary
                            for(int omg = 0; omg < iRowsInterval; omg++) {
                                if(exclJumps.count(h + omg) != 0) {
                                    h = exclJumps[h + omg];
                                }
                            }

                            // Obtain a current pixel color
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
                                float bn = minBn + (1 - minBn) * perc; // Final brightness
                                tr *= bn;
                                tg *= bn;
                                tb *= bn;
                            }

                            // Draw pixel to the buffer
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

                        // Check whether new exclusion can be merged with other one(s), if yes then do that
                        bool append = false;
                        for(int e = 0; e < exclCount; e++) {
                            pair<int, int>& excl = drawExcls.at(e);

                            if(dbStart >= excl.first && dbStart <= excl.second) {
                                // Only Start is inside
                                // Lengthten the exclusion in the bottom direction
                                append = false;
                                if(dbEnd > excl.second) {
                                    excl.second = dbEnd;
                                    e = 0;
                                } else {
                                    break;
                                }
                            } else if(dbEnd >= excl.first && dbEnd <= excl.second) {
                                // Only End is inside
                                // Lengthen the exclusion in the top direction
                                append = false;
                                if(dbStart < excl.first) {
                                    excl.first = dbStart;
                                    e = 0;
                                } else {
                                    break;
                                }
                            } else {
                                // Neither Start nor End is inside
                                // Append it as new unique exclusion
                                append = true;
                            }
                        }
                        if(append || exclCount == 0) {
                            drawExcls.push_back(make_pair(dbStart, dbEnd));
                            exclCount++;
                        }

                        SDL_UnlockSurface(sdlSurface);

                    }

                    if(cdi->wallDataPtr->stopsRay)
                        keepWalking = false;

                    delete cdi;

                    if(!keepWalking)
                        break;
                }
            }

            #ifdef DEBUG
            /* DRAW START AND END COORDINATE POINTS OVER THE CURRENT COLUMN, WHICH IN CONTINOUS COLUMNS DRAWING
             * WILL RESULT IN DRAW START(GREEN)/END(RED) LINES */
            int exclCount = drawExcls.empty() ? 0 : drawExcls.size();

            SDL_UnlockSurface(sdlSurface);
            for(int e = 0; e < exclCount; e++) {
                const pair<int, int>& range = drawExcls.at(e);

                // This draws line ranges
                for(int c = 0; c < iColumnsPerRay; c++) {
                    int hor = c + column;
                    if(hor < 0 || hor >= sdlSurface->w)
                        break;
                    for(int r = 0; r < iRowsInterval; r++) {

                        int verS = range.first + r - 1;
                        if(verS < 0 || verS >= sdlSurface->h)
                            break;
                        pixels[hor + verS * sdlSurface->w] = SDL_MapRGB(sdlSurface->format, 0, 255, 0);

                        int verE = range.second + r - 1;
                        if(verE < 0 || verE >= sdlSurface->h)
                            break;
                        pixels[hor + verE * sdlSurface->w] = SDL_MapRGB(sdlSurface->format, 255, 0, 0);

                    }
                }
            }
            SDL_LockSurface(sdlSurface);

            #endif

            #ifdef DEBUG
            /* DRAW CENTRAL VERTICAL LINE, IT IS KINDA BUGGED FOR NOW */

            if(!centerDebugDone && abs(column - iScreenWidth / 2) <= iColumnsPerRay) {
                
                // system("clear");
                // for(const auto& sus : drawExcls) {
                //     cout << "Start " << sus.first << ", End " << sus.second << endl;
                // }

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
