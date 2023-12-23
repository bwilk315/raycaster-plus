
#ifndef _GAME_HPP
#define _GAME_HPP

#include <iostream> // Ekhem
#include <fstream>
#include <vector>
#include <cmath>
#include <map>
#include <SDL2/SDL_pixels.h>
#include "../include/math.hpp"

typedef std::pair<int, int> int_pair;

struct LineEquation {
    float slope;       // Line growth rate
    float intercept;   // Displacement along Y axis
    float domainStart; // <0; 1>
    float domainEnd;   // <0; 1>

    LineEquation();
    LineEquation(float slope, float intercept, float domainStart, float domainEnd);
    vec2f intersection(const LineEquation& other) const;
};

struct RayHitInfo {
    float distance; // Distance traveled by a ray
    vec2i tile;     // Hit tile position
    vec2f point;    // Global point hit by the ray

    RayHitInfo();
    RayHitInfo(float distance, vec2i tile, vec2f point, bool side);
};

/* Information about a single wall inbounded in some tile. Line created using the information
 * is a top-view of that wall. */
struct WallInfo {
    LineEquation line;
    SDL_Color color;   // all channels: <0; 255>

    WallInfo();
    WallInfo(LineEquation line, SDL_Color color);
};

/* Describes a grid filled with tile data, each having its unique position in two-dimensional
 * cartesian coordinates system. The origin point (0, 0) is located at the bottom-left corner
 * of the grid, X axis grows when moving right, Y axis do so when moving up. */
class Plane {
    private:
        int width = 0;
        int height = 0;
        int* tiles = nullptr;
        // Dictionary defining geometry (consisting of 1+ lines) for walls identified by id numbers
        std::map<int, std::vector<WallInfo>> walls;

        /* Returns an index corresponding to data array element that is found at position (<x>, <y>). */
        int posToDataIndex(int x, int y) const;

    public:
        enum {
            // General errors
            E_G_CLEAR = 0,
            E_G_TILE_NOT_FOUND,
            // Plane file errors
            E_PF_CLEAR = 3,
            E_PF_FAILED_TO_LOAD,
            E_PF_OPERATION_NOT_AVAILABLE, // Operation depends on something that is not done yet
            E_PF_UNKNOWN_NUMBER_FORMAT,   // It can be caused by a text not being an actual number
            E_PF_INVALID_ARGUMENTS_COUNT
        };

        Plane();
        Plane(int width, int height);
        Plane(const std::string& file);
        ~Plane();
        // Sets all tiles id to <tileId>
        void setAllTiles(int tileId);
        // Checks if a point (<x>, <y>) is inside bounds of the plane
        bool inBounds(int x, int y) const;
        int getTileData(int x, int y) const;
        int getHeight() const;
        int getWidth() const;
        int maxData() const;  // Returns the highest value of a tile used.
        int setTileData(int x, int y, int tileData);
        // Sets properties of line with id <lineId> belonging to a tile with id <tileId>
        void setTileWall(int tileData, int wallIndex, LineEquation line, SDL_Color color);
        // Loads plane tiles data from a specified file
        int_pair load(const std::string& file);
        // Get all line equations for the tile with id <tileId>
        std::vector<WallInfo> getTileWalls(int tileData) const;
};

/* Class providing DDA algorithm in a stepping-like way. First initialize it using <init> function,
 * then use <next> function to perform one DDA step. */
class DDA_Algorithm {
    private:
        int maxTileDistSquared; // For later comparisons without square root
        const Plane* plane;

        bool initialized;
        int stepX, stepY;             // Direction of a ray stepping
        int planePosX, planePosY;     // Position of a tile the ray is currently in
        float deltaDistX, deltaDistY; // Distances needed to move by one unit in both axes
        float sideDistX, sideDistY;   // Currently-traveled distance by moving one unit in both axes
        vec2f start;     // Ray starting point
        vec2f direction; // Ray stepping direction (normalized)

    public:
        int rayFlag;
        // Ray flags telling various things about a ray
        enum {
            RF_CLEAR    = 0,
            RF_HIT      = 1 << 1, // Informs that hit occurred
            RF_SIDE     = 1 << 2, // Indicates that ray hit the tile from east/west direction
            RF_TOO_FAR  = 1 << 3, // Set if a tile hit by ray exceeded the maximum tile distance
            RF_OUTSIDE  = 1 << 4  // Tells that ray hit a tile which is out of the plane bounds
        };

        DDA_Algorithm(const Plane* plane, int maxTileDist);
        /* Prepares things needed to perform the algorithm from point <start> in direction <direction>. */
        void init(const vec2f& start, const vec2f& direction);
        /* Performs one step resulting in hitting some tile, information about the hit is returned. */
        RayHitInfo next();
};

class Camera {
    private:
        float planeMagnitude;
        float fov;
        vec2f plane;
        vec2f position;
        vec2f direction;

    public:
        Camera();
        /* Constructs a camera and positions it at point (<x>, <y>), then sets its field of view to
         * <fov> rad and initial direction to <dir> rad. */
        Camera(float x, float y, float fov, float dir);
        void changeDirection(float radians);
        void changePosition(vec2f delta);
        void setDirection(float radians);
        void setPosition(vec2f pos);
        float getFieldOfView() const;
        vec2f getPlane() const;
        vec2f getPosition() const;
        vec2f getDirection() const;
};

#endif
