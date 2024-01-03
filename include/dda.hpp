
#ifndef _RP_DDA_HPP
#define _RP_DDA_HPP

#ifdef DEBUG
#include <iostream>
#endif
#include "scene.hpp"
#include "math.hpp"

namespace rp {
    #ifdef DEBUG
    using ::std::ostream;
    using ::std::cout;
    using ::std::endl;
    #endif

    struct RayHitInfo {
        float distance; // Distance of hit point to the starting position
        Vector2 tile;   // Position of a hit tile
        Vector2 point;  // Position of the ray-tile intersection point

        RayHitInfo();
        RayHitInfo(float distance, Vector2 tile, Vector2 point);
    };
    #ifdef DEBUG
    ostream& operator<<(ostream& stream, const RayHitInfo& hit);
    #endif

    /**
     * Offers stepping-based Digital Differential Analysis algorithm implementation, ray-stepping
     * occurs on a scene (instance of `Scene` class) through its tile IDs (ID of 0 is ignored).
     * First set the target scene using `setTargetScene` method, you can also specify maximum
     * distance of a ray from the starting point using method `setMaxTileDistance`.
     * 
     * To start, tell the ray starting position and direction using `init` method, then simply call
     * `next` method to obtain the next hit information.
     * 
     * It is worth to mention about the `rayFlag` member - it tells you the current ray state, and gets
     * updated everytime you request a new ray information.
     */
    class DDA {
        private:
            bool initialized;
            int maxTileDist;
            int stepX, stepY;             // Direction of a ray stepping
            int planePosX, planePosY;     // Position of a tile the ray is currently in
            float deltaDistX, deltaDistY; // Distances needed to move by one unit in both axes
            float sideDistX, sideDistY;   // Currently-traveled distance by moving one unit in both axes
            Vector2 start;     // Cached ray starting point
            Vector2 direction; // Cached ray stepping direction (normalized)

            const Scene* scene = nullptr;
        public:
            const float MAX_DD = 1e30f; // Maximum delta distance for both axes
            int rayFlag;
            // Ray flags telling various things about a ray
            enum {
                RF_CLEAR    = 0,
                RF_HIT      = 1 << 1, // Informs that hit occurred (hit a tile with non-zero ID)
                RF_SIDE     = 1 << 2, // Indicates that ray hit the tile from east/west direction
                RF_TOO_FAR  = 1 << 3, // Set if a tile hit by ray exceeded the maximum tile distance
                RF_OUTSIDE  = 1 << 4, // Tells that ray hit a tile which is out of the plane bounds
                RF_FAIL     = 1 << 5  // Says that ray is unable to walk due to some error
            };

            DDA();
            DDA(const Scene* scene);
            DDA(const Scene* scene, int maxTileDist);

            void setTargetScene(const Scene* scene);
            void setMaxTileDistance(float distance);
            float getMaxTileDistance() const;
            const Scene* getTargetScene() const;
            void init(const Vector2& start, const Vector2& direction);
            RayHitInfo next();
    };
    #ifdef DEBUG
    ostream& operator<<(ostream& stream, const DDA& dda);
    #endif
}

#endif
