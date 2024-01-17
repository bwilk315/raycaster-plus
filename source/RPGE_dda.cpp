
#include <RPGE_dda.hpp>

namespace rpge {

    /*********************************************/
    /********** STRUCTURE: RAY HIT INFO **********/
    /*********************************************/

    RayHitInfo::RayHitInfo() {
        this->distance = -1;
        this->tile = Vector2::ZERO;
        this->point = Vector2::ZERO;
    }
    RayHitInfo::RayHitInfo(float distance, Vector2 tile, Vector2 point) {
        this->distance = distance;
        this->tile = tile;
        this->point = point;
    }
    #ifdef DEBUG
    ostream& operator<<(ostream& stream, const RayHitInfo& hit) {
        stream << "RayHitInfo(distance=" << hit.distance << ", tile=" << hit.tile << ", point=" << hit.point << ")";
        return stream;
    }
    #endif

    /**********************************************************/
    /********** CLASS: DIGITAL DIFFERENTIAL ANALYSIS **********/
    /**********************************************************/

    DDA::DDA() {
        this->initialized = false;
        this->maxTileDist = 0;
        this->scene = nullptr;
    }
    DDA::DDA(const Scene* scene) : DDA() {
        setTargetScene(scene);
    }
    DDA::DDA(const Scene* scene, int maxTileDist) : DDA(scene) {
        setMaxTileDistance(maxTileDist);
    }
    void DDA::setTargetScene(const Scene* scene) {
        this->scene = scene;
    }
    void DDA::setMaxTileDistance(float distance) {
        maxTileDist = distance;
    }
    float DDA::getMaxTileDistance() const {
        return maxTileDist;
    }
    const Scene* DDA::getTargetScene() const {
        return scene;
    }
    void DDA::init(const Vector2& start, const Vector2& direction) {
        if(scene == nullptr) {
            rayFlag = RF_FAIL;
            return;
        }
        this->initialized = true;
        this->originDone = false;
        this->start = start;
        this->direction = direction;
        this->rayFlag = RF_CLEAR;

        planePosX = (int)start.x;
        planePosY = (int)start.y;
        deltaDistX = direction.x == 0 ? MAX_DD : std::abs(1 / direction.x);
        deltaDistY = direction.y == 0 ? MAX_DD : std::abs(1 / direction.y);
        // Set initial distances and stepping direction
        if(direction.x < 0) {
            stepX = -1;
            sideDistX = (start.x - planePosX) * deltaDistX;
        } else {
            stepX = 1;
            sideDistX = (1 + planePosX - start.x) * deltaDistX;
        }
        if(direction.y < 0) {
            stepY = -1;
            sideDistY = (start.y - planePosY) * deltaDistY;
        } else {
            stepY = 1;
            sideDistY = (1 + planePosY - start.y) * deltaDistY;
        }
    }
    RayHitInfo DDA::next() {
        if(!initialized) {
            rayFlag = RF_FAIL;
            return RayHitInfo();
        } else if(!originDone) {
            // Include the starting tile in the ray walk
            if(scene->getTileId(start.x, start.y) != 0)
                rayFlag = RF_HIT;
            originDone = true;
            return RayHitInfo(0, Vector2((int)start.x, (int)start.y), start);
        }
        
        // Step along appropriate axis
        if(sideDistX < sideDistY) {
            sideDistX += deltaDistX;
            planePosX += stepX;
            rayFlag = RF_SIDE;
        } else {
            sideDistY += deltaDistY;
            planePosY += stepY;
            rayFlag = RF_CLEAR;
        }
        // Check if hit tile is not exceeding the maximum tile distance
        int deltaPosX = planePosX - start.x;
        int deltaPosY = planePosY - start.y;
        if(deltaPosX * deltaPosX + deltaPosY * deltaPosY > maxTileDist * maxTileDist) {
            rayFlag = RF_TOO_FAR;
            return RayHitInfo();
        }
        // Check if hit tile is not outside the plane
        if(!scene->checkPosition(planePosX, planePosY)) {
            rayFlag = RF_OUTSIDE;
            return RayHitInfo();
        }

        // If tile data is not zero, then ray hit this tile
        int tileData = scene->getTileId(planePosX, planePosY);
        if(tileData != 0) {
            float distance = (rayFlag == RF_SIDE) ? (sideDistX - deltaDistX) : (sideDistY - deltaDistY);
            rayFlag |= RF_HIT;
            return RayHitInfo(
                distance,
                Vector2(planePosX, planePosY),
                start + direction * distance
            );
        }
        // This should not trigger but it exists for safety reasons
        rayFlag = RF_CLEAR;
        return RayHitInfo();
    }
    #ifdef DEBUG
    ostream& operator<<(ostream& stream, const DDA& dda) {
        stream << "DDA(rayFlag=" << dda.rayFlag << ", maxTileDist=" << dda.getMaxTileDistance() << ", scene=";
        stream << dda.getTargetScene() << ")";
        return stream;
    }
    #endif
}
