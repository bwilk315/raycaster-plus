
#include <RPGE_engine.hpp>

namespace rpge
{    

    /***********************************/
    /********** CLASS: ENGINE **********/
    /***********************************/

    const float Engine::SAFE_LINE_HEIGHT = 0.0001f;

    Engine::Engine(int screenWidth, int screenHeight)
    {
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

        if(SDL_InitSubSystem(SDL_INIT_VIDEO) == 0)
        {
            this->mainCamera = nullptr;
            this->walker     = new DDA();
            this->sdlWindow  = SDL_CreateWindow("Raycaster Plus Engine", 0, 0, screenWidth, screenHeight, SDL_WINDOW_SHOWN);
            if(sdlWindow != nullptr)
            {
                this->sdlRend = SDL_CreateRenderer(sdlWindow, -1, SDL_RENDERER_ACCELERATED);
                if(sdlRend != nullptr)
                {
                    // Everything is OK
                    SDL_SetRenderDrawBlendMode(sdlRend, SDL_BLENDMODE_BLEND);
                    SDL_SetRenderDrawColor(sdlRend, 0, 0, 0, 255);
                    SDL_RenderClear(sdlRend);

                    SDL_SetWindowResizable(this->sdlWindow, SDL_FALSE);
                    return;
                }
            }
        }
        iError |= E_SDL;
    }
    Engine::~Engine()
    {
        if(walker != nullptr)
            delete walker;
        if(sdlWindow != nullptr)
            SDL_DestroyWindow(sdlWindow);
        if(iError != E_SDL) {
            SDL_QuitSubSystem(SDL_INIT_VIDEO);
            SDL_Quit();
        }
    }
    void Engine::clear()
    {
        bClear = true;
    }
    void Engine::stop()
    {
        bRun = false;
    }
    void Engine::setClearColor(uint8_t r, uint8_t g, uint8_t b)
    {
        cClearColor.r = r;
        cClearColor.g = g;
        cClearColor.b = b;
    }
    void Engine::setCursorLock(bool locked)
    {
        bIsCursorLocked = locked;
    }
    void Engine::setCursorVisibility(bool visible)
    {
        if(SDL_ShowCursor(visible) < 0)
            iError |= E_SDL;
    }
    void Engine::setColumnsPerRay(int n)
    {
        iColumnsPerRay = clamp(n, 1, rRenderArea.w);
    }
    void Engine::setFrameRate(int fps)
    {
        iFramesPerSecond = fps < 1 ? 1 : fps;
    }
    void Engine::setLightBehavior(bool enabled, float angle)
    {
        bLightEnabled = enabled;
        vLightDir = (Vector2::RIGHT).rotate(angle);
    }
    void Engine::setMainCamera(const Camera* camera)
    {
        mainCamera = camera;
    }
    void Engine::setClearArea(const SDL_Rect& rect)
    {
        rClearArea.w = clamp(rect.w, 0, rRenderArea.w);
        rClearArea.h = clamp(rect.h, 0, rRenderArea.h);
        rClearArea.x = clamp(rect.x, rRenderArea.x, rRenderArea.x + rRenderArea.w - rClearArea.w);
        rClearArea.y = clamp(rect.y, rRenderArea.y, rRenderArea.y + rRenderArea.h - rClearArea.h);
    }
    void Engine::setRenderArea(const SDL_Rect& rect)
    {
        rRenderArea.w = clamp(rect.w, 0, iScreenWidth);
        rRenderArea.h = clamp(rect.h, 0, iScreenHeight);
        rRenderArea.x = clamp(rect.x, 0, iScreenWidth - rRenderArea.w);
        rRenderArea.y = clamp(rect.y, 0, iScreenHeight - rRenderArea.h);
        setClearArea(rRenderArea);
    }
    void Engine::setRowsInterval(int n)
    {
        iRowsInterval = clamp(n, 1, rRenderArea.h);
    }
    void Engine::render()
    {
        bRedraw = true;
    }
    int Engine::getError() const
    {
        return iError;
    }
    int Engine::getFrameCount() const
    {
        return frameIndex;
    }
    int Engine::getScreenWidth() const
    {
        return iScreenWidth;
    }
    int Engine::getScreenHeight() const
    {
        return iScreenHeight;
    }
    float Engine::getElapsedTime() const
    {
        return elapsedTime.count();
    }
    SDL_Rect Engine::getRenderArea() const
    {
        return rRenderArea;
    }
    SDL_Renderer* Engine::getRendererHandle()
    {
        return sdlRend;
    }
    KeyState Engine::getKeyState(int sc) const
    {
        return keyStates.count(sc) == 0 ? KeyState::NONE : keyStates.at(sc);
    }
    Vector2 Engine::getMousePosition() const
    {
        int x, y;
        SDL_GetMouseState(&x, &y);
        return Vector2(x, y);
    }
    DDA* Engine::getWalker()
    {
        return walker;
    }
    SDL_Window* Engine::getWindowHandle()
    {
        return sdlWindow;
    }
    bool Engine::tick()
    {
        if(iError)
        {
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
        for(auto& p : ksCopy)
        {
            if(p.second == KeyState::DOWN)
                keyStates.at(p.first) = KeyState::PRESS;
            else if(p.second == KeyState::UP)
                keyStates.erase(p.first);
        }

        // Interpret SDL events
        SDL_Event event;
        while(SDL_PollEvent(&event))
        {
            int sc;
            switch(event.type)
            {
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
        if(bClear)
        {
            SDL_SetRenderDrawColor(sdlRend, cClearColor.r, cClearColor.g, cClearColor.b, cClearColor.a);
            SDL_RenderFillRect(sdlRend, &rClearArea);
            bClear = false;
        }
        // Skip drawing process if redrawing is not requested
        int column = bRedraw ? rRenderArea.x : (rRenderArea.x + rRenderArea.w);

        // Draw the current frame, which consists of pixel columns
        for( ; column < (rRenderArea.x + rRenderArea.w); column += iColumnsPerRay)
        {

            // Drawing exclusions for the current pixel column encoded in key-value pair (start-end heights in screen coordinates)
            vector<pair<int, int>> drawExcls;
            bool keepWalking = true;

            // Position of the ray on the camera plane, from -1 (leftmost) to 1 (rightmost)
            float cameraX  = 2 * (column - rRenderArea.x) / (float)rRenderArea.w - 1;
            Vector2 rayDir = (camDir + planeVec * cameraX).normalized();

            walker->init(camPos, rayDir);
            while(keepWalking)
            {


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
                if(walker->rayFlag & DDA::RF_SIDE)
                {
                    localEnter.x = !hit.distance ? localX : (rayDir.x < 0);
                    localEnter.y = localY;
                }
                else
                {
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

                int wallCount = wallData->size();
                // Array of distances to local intersection points of walls with respective indices (e.g. drawInfos[1]
                // is all about wallData.at(1)).
                pair<float, Vector2> drawInfos[wallCount];

                for(int i = 0; i != wallCount; i++)
                {
                    const WallData* wdPtr = &wallData->at(i);
                    float perpDist = 0xffff;
                    Vector2 localInter;

                    // The formula below was derived by parts, and compressed into one long computation
                    // NOTE: this formula works even when enter point is actually inside a tile.
                    float a         = wdPtr->func.slope;
                    float h         = wdPtr->func.height;
                    float interDist = ( localEnter.y - a * localEnter.x - ( h == 0 ? SAFE_LINE_HEIGHT : h ) ) / ( rayDir.x * a - rayDir.y );

                    // Distance is negative when a wall is not reached by the ray, this and the fact that the longest
                    // distance in tile boundary is 1/sqrt(2), can be used to perform early classification.
                    if(interDist >= 0 && interDist <= SQRT2)
                    {
                        localInter = interDist * rayDir + localEnter;

                        // Check if point is included in arguments and values range defined
                        if((localInter.x >= wdPtr->func.xMin && localInter.x <= wdPtr->func.xMax) &&
                           (localInter.y >= wdPtr->func.yMin && localInter.y <= wdPtr->func.yMax))
                        {
                            perpDist = rayDir.dot(camDir) * ( hit.distance + interDist );
                        }
                    }

                    drawInfos[i] = make_pair(perpDist, localInter);
                }


                /******************************************************************************/
                /********** COLUMN DRAWING USING COLLECTED WALLS DRAWING INFORMATION **********/
                /******************************************************************************/

                for(int i = 0; i != wallCount; i++)
                {

                    int nearest = 0;
                    float perpDist = 0xffff;

                    // Find index of the nearest wall (and additionally save its distance)
                    for(int j = 0; j != wallCount; j++)
                    {
                        float cpd = drawInfos[j].first;
                        if(cpd != 0xffff && cpd < perpDist)
                        {
                            nearest = j;
                            perpDist = drawInfos[j].first;
                        }
                    }

                    if(perpDist == 0xffff)
                        continue;

                    // Exclude the (for now) the nearest wall and collect its second property
                    drawInfos[nearest].first = 0xffff;
                    Vector2 localInter = drawInfos[nearest].second;

                    const WallData* wdPtr = &wallData->at(nearest);

                    // Calculate a normal vector of the wall, it always points outwards
                    bool flipped = false;
                    float a      = wdPtr->func.slope;
                    float h      = wdPtr->func.height;
                    float coef   = 1 / sqrt( a * a + 1 );
                    Vector2 normal(a * coef, -1 * coef);
                    if(camPos.y >= a * (camPos.x - hit.tile.x) + hit.tile.y + h)
                    {
                        normal *= -1;
                        flipped = true;
                    }

                    // Find out range describing how column should be drawn for the current wall
                    float lineHeight = rRenderArea.h * (pcmDist / perpDist);
                    int drawStart    = rRenderArea.y + (rRenderArea.h - lineHeight) / 2 + lineHeight * (1 - wdPtr->hMax);
                    int drawEnd      = rRenderArea.y + (rRenderArea.h + lineHeight) / 2 - lineHeight * wdPtr->hMin;

                    // Obtain information on the wall looks
                    SDL_Texture* texPtr = mainScene->getTextureSource(wdPtr->texId);
                    bool isSolidColor = false;
                    if(texPtr == nullptr) isSolidColor  = true;

                    // Compute normalized horizontal position on the wall plane
                    float planeHorizontal = (localInter - wdPtr->pivot).magnitude() / wdPtr->length;
                    if(flipped)
                        planeHorizontal = 1 - planeHorizontal;

                    // Draw the line with exclusions taken into account
                    
                    bool beg = false;
                    int exclCount = drawExcls.size();
                    int lineStart = drawStart;
                    int lineEnd   = drawEnd;
                    int e = 0;
                    int texWidth, texHeight;

                    SDL_QueryTexture(texPtr, NULL, NULL, &texWidth, &texHeight);
                    // Texture pixel height in screen pixels
                    float tpHeight = (drawEnd - drawStart) / (float)texHeight;

                    pair<int, int> ex;
                    while(true)
                    {
                        if(e != exclCount)
                        {
                            ex = drawExcls.at(e);
                            if(ex.second > drawStart)
                            {
                                if(beg || drawStart > ex.first)
                                {
                                    lineStart = ex.second;
                                    if(++e != exclCount)
                                        ex = drawExcls.at(e);
                                }
                                lineEnd = (e == exclCount || drawEnd <= ex.first) ? drawEnd : ex.first;
                                beg = true;
                                if(lineStart > lineEnd)
                                    break;
                            }
                            else
                            {
                                e++;
                                continue;
                            }
                        }

                        // Draw drawable part of the line drawing range if possible
                        SDL_Rect rendRect = { column, lineStart, iColumnsPerRay, lineEnd - lineStart };
                        if(isSolidColor)
                        {
                            // Draw solid-color column
                            uint8_t cr, cg, cb, ca;
                            deColor(wdPtr->tint, cr, cg, cb, ca);
                            SDL_SetRenderDrawColor(sdlRend, cr, cg, cb, ca);
                            SDL_RenderFillRect(sdlRend, &rendRect);
                        }
                        else
                        {
                            // Draw part of a texture
                            float offset   = (rendRect.y - drawStart);
                            float length   = rendRect.h;

                            // THIS BLOCK REMOVES PARTIAL PIXELS = FIXES WRONG PIXELS STRETCH
                            if(lineStart == drawStart)
                                rendRect.h = floorf(rendRect.h / tpHeight) * tpHeight;
                            else if(lineEnd == drawEnd)
                            {
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
                        // Shade the drawn column by drawing black color with appropriate opacity over it
                        SDL_SetRenderDrawColor(sdlRend, 0, 0, 0, (normal.dot(vLightDir) + 1.0f) / 2.0f * 128);
                        SDL_RenderDrawRect(sdlRend, &rendRect);

                        if(e == exclCount || lineEnd == drawEnd || ex.second >= drawEnd)
                            break;
                    } 


                    // Prepare exclusions vector for the new exclusion (the drawn line range), every range in that vector
                    // must stay separated from each other.
                    int varStart = drawStart;
                    int varEnd   = drawEnd;
                    e = -1;
                    while(++e < exclCount)
                    {
                        pair<int, int>& ex = drawExcls.at(e);

                        if(ex.first <= varEnd && ex.second >= varStart)
                        {
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
                        if(varStart <= drawExcls.at(e).first)
                        {
                            t = e;
                            break;
                        }
                    drawExcls.insert(drawExcls.begin() + t, make_pair(varStart, varEnd));

                    // Decide if ray should keep on walking, free column drawing information because it was already used
                    if(wdPtr->stopsRay)
                    {
                        keepWalking = false;
                        break;
                    }
                }
            }

            #ifdef DEBUG

            // Draw exclusion ranges
            for(const pair<int, int>& excl : drawExcls)
            {
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

        if(bRedraw)
        {
            SDL_RenderPresent(sdlRend);
            bRedraw = false;
        }
        frameIndex++;
        return bRun;
    }
}
