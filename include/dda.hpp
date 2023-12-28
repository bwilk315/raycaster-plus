
#ifndef _DDA_HPP
#define _DDA_HPP

#include "scene.hpp"
#include "math.hpp"

namespace rp {
    using ::std::ostream;

    struct RayHitInfo {
        float distance;
        Vector2 tile;
        Vector2 point;

        RayHitInfo();
        RayHitInfo(float distance, Vector2 tile, Vector2 point);
    };
    ostream& operator<<(ostream& stream, const RayHitInfo& hit);

    class DDA {
        private:
            bool initialized;
            int maxTileDist;
            int stepX, stepY;             // Direction of a ray stepping
            int planePosX, planePosY;     // Position of a tile the ray is currently in
            float deltaDistX, deltaDistY; // Distances needed to move by one unit in both axes
            float sideDistX, sideDistY;   // Currently-traveled distance by moving one unit in both axes
            Vector2 start;     // Ray starting point
            Vector2 direction; // Ray stepping direction (normalized)

            Scene* scene = nullptr;
        public:
            const float MAX_DD = 1e30f; // Maximum delta distance for both axes
            int rayFlag;
            // Ray flags telling various things about a ray
            enum {
                RF_CLEAR    = 0,
                RF_HIT      = 1 << 1, // Informs that hit occurred
                RF_SIDE     = 1 << 2, // Indicates that ray hit the tile from east/west direction
                RF_TOO_FAR  = 1 << 3, // Set if a tile hit by ray exceeded the maximum tile distance
                RF_OUTSIDE  = 1 << 4, // Tells that ray hit a tile which is out of the plane bounds
                RF_ERROR    = 1 << 5
            };

            DDA();
            DDA(Scene* scene, int maxTileDist);

            void setTargetScene(Scene* scene);
            void setMaxTileDistance(float distance);
            float getMaxTileDistance() const;
            Scene* const getTargetScene();
            /* Prepares things needed to perform the algorithm from point <start> in direction <direction>.
             * Returns two initial hits. */
            void init(const Vector2& start, const Vector2& direction);
            /* Performs one step resulting in hitting some tile, information about the hit is returned. */
            RayHitInfo next();
    };
}

#endif
