
#include "../include/dda.hpp"

#include <iostream>

namespace rp {

    /************************************************/
    /********** STRUCTURE: RAY HIT INFORMATION ******/
    /************************************************/

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
    ostream& operator<<(ostream& stream, const RayHitInfo& hit) {
        stream << "RayHitInfo(distance=" << hit.distance << ", tile=" << hit.tile << ", point=" << hit.point << ")";
        return stream;
    }

    /******************************************************/
    /********** CLASS: DIGITAL DIFFERENTIAL ANALYSIS ******/
    /******************************************************/

    DDA::DDA() {
        this->initialized = false;
    }
    DDA::DDA(Scene* scene, int maxTileDist) {
        this->initialized = false;
        setTargetScene(scene);
        setMaxTileDistance(maxTileDist);
    }
    void DDA::setTargetScene(Scene* scene) {
        this->scene = scene;
    }
    void DDA::setMaxTileDistance(float distance) {
        maxTileDist = distance;
    }
    float DDA::getMaxTileDistance() const {
        return maxTileDist;
    }
    Scene* DDA::getTargetScene() {
        return scene;
    }
    void DDA::init(const Vector2& start, const Vector2& direction) {
        this->start = start;
        this->direction = direction;
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
        // State for the first hit
        this->rayFlag = sideDistX < sideDistY ? DDA::RF_SIDE : DDA::RF_CLEAR;
    }
    RayHitInfo DDA::next() {
        if(!initialized) {
            rayFlag = DDA::RF_ERROR;
            return RayHitInfo();
        }
        
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
        if(deltaPosX * deltaPosX + deltaPosY * deltaPosY > maxTileDist * maxTileDist) {
            rayFlag = DDA::RF_TOO_FAR;
            return RayHitInfo();
        }
        // Check if hit tile is not outside the plane
        int_pair p = scene->getTileData(planePosX, planePosY);
        if(p.second != Scene::E_CLEAR || !scene->contains(planePosX, planePosY)) {
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
                start + direction * distance
            );
        }
        // This should not trigger but it exists for safety reasons
        rayFlag = DDA::RF_CLEAR;
        return RayHitInfo();
    }
}

