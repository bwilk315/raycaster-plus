
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
        this->cClearColor        = { 0, 0, 0, 255 };
        this->frameIndex         = 0;
        this->vLightDir          = Vector2::RIGHT;
        this->tpLast             = system_clock::now();
        this->elapsedTime        = duration<float>(0);
        this->rClearArea         = { 0, 0, iScreenWidth, iScreenHeight };
        this->rRenderArea        = rClearArea;
        this->keyStates          = map<int, KeyState>();

        if(SDL_InitSubSystem(SDL_INIT_VIDEO) == 0) {
            this->mainCamera = nullptr;
            this->walker     = new DDA();
            this->sdlWindow  = SDL_CreateWindow("Raycaster Plus Engine", 0, 0, screenWidth, screenHeight, SDL_WINDOW_SHOWN);
            if(sdlWindow != nullptr) {
                this->sdlRend = SDL_CreateRenderer(sdlWindow, -1, SDL_RENDERER_ACCELERATED);
                if(sdlRend != nullptr) {
                    // Everything is OK
                    SDL_SetRenderDrawColor(sdlRend, 0, 0, 0, 255);
                    SDL_RenderClear(sdlRend);

                    SDL_SetWindowResizable(this->sdlWindow, SDL_FALSE);
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
    void Engine::stop() {
        bRun = false;
    }
    void Engine::setClearColor(uint8_t r, uint8_t g, uint8_t b) {
        cClearColor.r = r;
        cClearColor.g = g;
        cClearColor.b = b;
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
    void Engine::setClearArea(const SDL_Rect& rect) {
        rClearArea.w = clamp(rect.w, 0, rRenderArea.w);
        rClearArea.h = clamp(rect.h, 0, rRenderArea.h);
        rClearArea.x = clamp(rect.x, rRenderArea.x, rRenderArea.x + rRenderArea.w - rClearArea.w);
        rClearArea.y = clamp(rect.y, rRenderArea.y, rRenderArea.y + rRenderArea.h - rClearArea.h);
    }
    void Engine::setRenderArea(const SDL_Rect& rect) {
        rRenderArea.w = clamp(rect.w, 0, iScreenWidth);
        rRenderArea.h = clamp(rect.h, 0, iScreenHeight);
        rRenderArea.x = clamp(rect.x, 0, iScreenWidth - rRenderArea.w);
        rRenderArea.y = clamp(rect.y, 0, iScreenHeight - rRenderArea.h);
        setClearArea(rRenderArea);
    }
    void Engine::setRowsInterval(int n) {
        iRowsInterval = clamp(n, 1, rRenderArea.h);
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
    SDL_Renderer* Engine::getRendererHandle() {
        return sdlRend;
    }
    KeyState Engine::getKeyState(int sc) const {
        return keyStates.count(sc) == 0 ? KeyState::NONE : keyStates.at(sc);
    }
    Vector2 Engine::getMousePosition() const {
        int x, y;
        SDL_GetMouseState(&x, &y);
        return Vector2(x, y);
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

        // Clear the specified part of screen buffer if requested
        if(bClear) {
            SDL_SetRenderDrawColor(sdlRend, cClearColor.r, cClearColor.g, cClearColor.b, cClearColor.a);
            SDL_RenderFillRect(sdlRend, &rClearArea);
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
                    float lineHeight = rRenderArea.h * (pcmDist / cdi->perpDist);
                    int drawStart    = rRenderArea.y + (rRenderArea.h - lineHeight) / 2 + lineHeight * (1 - cdi->wallDataPtr->hMax);
                    int drawEnd      = rRenderArea.y + (rRenderArea.h + lineHeight) / 2 - lineHeight * cdi->wallDataPtr->hMin;

                    // Obtain information on the wall looks
                    SDL_Texture* texPtr = mainScene->getTextureSource(cdi->wallDataPtr->texId);
                    bool isSolidColor = false;
                    if(texPtr == nullptr)
                        isSolidColor  = true;

                    // Compute normalized horizontal position on the wall plane
                    float planeHorizontal = (cdi->localInter - cdi->wallDataPtr->pivot).magnitude() / cdi->wallDataPtr->length;
                    if(flipped)
                        planeHorizontal = 1 - planeHorizontal;

                    // Find the first exclusion range after the drawing start coordinate
                    bool drawable = true;  // Does coordinates allow for valid line drawing?
                    int exclCount = drawExcls.size();
                    int lineStart = drawStart;
                    int lineEnd   = drawEnd;
                    int neIndex   = -1;  // Next exclusion range index in the `drawExcls` vector
                    for(int e = 0; e < exclCount; e++) {
                        const pair<int, int>& ex = drawExcls.at(e);

                        bool isNext = true;
                        bool found  = true;
                        if(drawStart >= ex.first && drawStart <= ex.second) {
                            // Start coordinate is included in the exclusion range, move it down
                            neIndex   = e + 1;
                            lineStart = ex.second;
                            if(neIndex == exclCount) {
                                // There is no next exclusion, end coordinate is unrestricted thus original
                                neIndex = -1;
                                isNext  = false;
                            }
                        } else if(drawStart <= ex.first) {
                            // Encountered exclusion below the start coordinate
                            neIndex   = e;
                        } else {
                            found = false;
                        }

                        if(found) {
                            if(isNext) {
                                // Drawing end coordinate is clipped to a next exclusion if it is existent
                                int neHeight = drawExcls.at(neIndex).first;
                                lineEnd      = drawEnd < neHeight ? drawEnd : neHeight;
                            }
                            drawable = lineStart < lineEnd;
                            break;
                        }
                    }

                    if(drawable) {

                        // Draw the line with exclusions taken into account
                        pair<int, int> next = neIndex == -1 ? make_pair(0, 0) : drawExcls.at(neIndex);
                        while(true) {
                            // Draw drawable part of the line drawing range if possible
                            int dbLs = clamp(lineStart, rRenderArea.y, rRenderArea.y + rRenderArea.h);
                            int dbLe = clamp(lineEnd,   rRenderArea.y, rRenderArea.y + rRenderArea.h);
                            if(dbLs != dbLe) {

                                SDL_Rect rendRect = { column, lineStart, iColumnsPerRay, lineEnd - lineStart };
                                if(isSolidColor) {
                                    // Draw solid-color column
                                    uint8_t cr, cg, cb, ca;
                                    deColor(cdi->wallDataPtr->tint, cr, cg, cb, ca);
                                    SDL_SetRenderDrawColor(sdlRend, cr, cg, cb, ca);
                                    SDL_RenderFillRect(sdlRend, &rendRect);
                                } else {
                                    // Draw part of a texture
                                    int texWidth, texHeight;
                                    SDL_QueryTexture(texPtr, NULL, NULL, &texWidth, &texHeight);

                                    // Texture pixel height in screen pixels
                                    float tpHeight = (drawEnd - drawStart) / (float)texHeight;

                                    float offset   = (rendRect.y - drawStart);
                                    float length   = rendRect.h;

// THIS BLOCK REMOVES PARTIAL PIXELS = FIXES WRONG PIXELS STRETCH
                                    if(lineStart == drawStart)
                                        rendRect.h = floorf(rendRect.h / tpHeight) * tpHeight;
                                    else if(lineEnd == drawEnd) {
                                        float dist = rendRect.y - drawStart;
                                        dist = ceilf(dist / tpHeight) * tpHeight;
                                        rendRect.y = drawStart + dist;
                                        rendRect.h = floorf(rendRect.h / tpHeight) * tpHeight;
                                    }

                                    offset /= (float)(drawEnd - drawStart);
                                    length /= (float)(drawEnd - drawStart);
                                    
                                    SDL_Rect texRect  = { texWidth * planeHorizontal, texHeight * offset, 1, texHeight * length };

                                    SDL_RenderCopy(sdlRend, texPtr, &texRect, &rendRect);
                                }

                            }

                            if(neIndex == -1)
                                break;
                            
                            // Hop to another drawable part of the line
                            lineStart = next.second;
                            if(++neIndex >= exclCount) {
                                lineEnd = drawEnd;
                                neIndex = -1;
                            } else {
                                next    = drawExcls.at(neIndex);
                                lineEnd = next.first;
                            }

                            if(drawEnd >= lineStart && drawEnd <= lineEnd) {
                                // End coordinate is included in a line part drawing range, clip the drawing end coordinate to it
                                lineEnd = drawEnd;
                                neIndex = -1;
                            } else if(drawEnd <= lineStart) {
                                // Line part drawing range is below the end coordinate, nothing will be visible from now on
                                break;
                            }
                        }                    

                        // Prepare exclusions vector for the new exclusion (the drawn line range), every range in that vector
                        // must stay separated from each other.
                        int varStart = drawStart;
                        int varEnd   = drawEnd;
                        int e = -1;
                        while(++e < exclCount) {
                            pair<int, int>& ex = drawExcls.at(e);

                            if(ex.first <= varEnd && ex.second >= varStart) {
                                varStart = ex.first  < varStart ? ex.first  : varStart;
                                varEnd   = ex.second > varEnd   ? ex.second : varEnd  ;
                                drawExcls.erase(drawExcls.begin() + e);
                                exclCount--;
                                e = -1;
                            }
                        }
                        // Add the drawn line range as new exclusion, perform it in such way that will remain the vector sorted
                        // ascendingly by start coordinate.
                        int t = exclCount;
                        for(int e = 0; e < exclCount; e++)
                            if(varStart <= drawExcls.at(e).first) {
                                t = e;
                                break;
                            }
                        drawExcls.insert(drawExcls.begin() + t, make_pair(varStart, varEnd));

                    }  // drawable?

                    // Decide if ray should keep on walking, free column drawing information because it was already used
                    if(cdi->wallDataPtr->stopsRay)
                        keepWalking = false;
                    delete cdi;
                    if(!keepWalking)
                        break;
                }
            }


            #ifdef DEBUG
            
            // Draw exclusion ranges
            for(const pair<int, int>& excl : drawExcls) {
                SDL_SetRenderDrawColor(sdlRend, 0, 255, 0, 255);
                SDL_RenderDrawPoint(sdlRend, column, excl.first);
                SDL_SetRenderDrawColor(sdlRend, 255, 0, 0, 255);
                SDL_RenderDrawPoint(sdlRend, column, excl.second + 1);
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
        
        SDL_Delay(delay); // Maybe this causes the lag when unfreezing?

        if(bRedraw) {
            SDL_RenderPresent(sdlRend);
            bRedraw = false;
        }
        frameIndex++;
        return bRun;
    }
}
