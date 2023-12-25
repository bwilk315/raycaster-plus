
#include "../include/dda.hpp"

namespace rp {

    /************************************************/
    /********** STRUCTURE: RAY HIT INFORMATION ******/
    /************************************************/

    RayHitInfo::RayHitInfo() {
        this->distance = -1;
        this->tile = Vector2::ZERO;
        this->point = Vector2::ZERO;
    }
    RayHitInfo::RayHitInfo(float distance, Vector2 tile, Vector2 point, bool side) {
        this->distance = distance;
        this->tile = tile;
        this->point = point;
    }

    /******************************************************/
    /********** CLASS: DIGITAL DIFFERENTIAL ANALYSIS ******/
    /******************************************************/

    DDA::DDA(const Plane* plane, int maxTileDist) {
        this->plane = plane;
        this->maxTileDistSquared = maxTileDist * maxTileDist;
    }
    void DDA::init(const Vector2& start, const Vector2& direction) {
        this->start = start;
        this->direction = direction;
        this->rayFlag = DDA::RF_CLEAR;
        this->initialized = true;

        planePosX = (int)start.x;
        planePosY = (int)start.y;
        deltaDistX = direction.x == 0 ? DDA::MAX_DD : std::abs(1 / direction.x);
        deltaDistY = direction.y == 0 ? DDA::MAX_DD : std::abs(1 / direction.y);
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
        // Step along appropriate axis
        if(sideDistX < sideDistY) {
            sideDistX += deltaDistX;
            planePosX += stepX;
            rayFlag = DDA::RF_SIDE;
        } else {
            sideDistY += deltaDistY;
            planePosY += stepY;
            rayFlag = DDA::RF_CLEAR;
        }
        // Check if hit tile is not exceeding the maximum tile distance
        int deltaPosX = planePosX - start.x;
        int deltaPosY = planePosY - start.y;
        if(deltaPosX * deltaPosX + deltaPosY * deltaPosY > maxTileDistSquared) {
            rayFlag = DDA::RF_TOO_FAR;
            return RayHitInfo();
        }
        // Check if hit tile is not outside the plane
        int_pair p = plane->getTileData(planePosX, planePosY);
        if(p.second != Plane::E_CLEAR || !plane->contains(planePosX, planePosY)) {
            rayFlag = DDA::RF_OUTSIDE;
            return RayHitInfo();
        }
        // If tile data is not zero, then ray hit this tile
        int tileData = p.first;
        if(tileData != 0) {
            float distance = (rayFlag == DDA::RF_SIDE) ? (sideDistX - deltaDistX) : (sideDistY - deltaDistY);
            rayFlag |= DDA::RF_HIT;
            return RayHitInfo(
                distance,
                Vector2(planePosX, planePosY),
                start + direction * distance,
                rayFlag & DDA::RF_SIDE
            );
        }
        // This should not trigger but it exists for safety reasons
        rayFlag = DDA::RF_CLEAR;
        return RayHitInfo();
    }
}

