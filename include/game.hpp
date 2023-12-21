
#ifndef _GAME_HPP
#define _GAME_HPP

#include <iostream>
#include <fstream>
#include <cmath>
#include <vector>
#include <map>
#include "../include/math.hpp"

typedef std::pair<int, int> int_pair;

struct LineEquation {
    float slope;     // Line growth rate
    float intercept; // Displacement along Y axis

    LineEquation();
    LineEquation(float slope, float intercept);
    vec2f intersection(const LineEquation& other) const;
};

struct RayHitInfo {
    float distance; // Distance traveled by a ray
    vec2f point;    // Global point hit by the ray
    bool side;      // Flag indicating if ray hit the tile from E/W direction

    RayHitInfo();
    RayHitInfo(float distance, vec2f point, bool side);
};

struct WallInfo {
    int id;                // Number indicating properties of a wall
    vec2f normal;          // Vector telling in which direction a wall is facing
    LineEquation geometry; // Equation describing a wall geometry from the top perspective

    WallInfo();
    WallInfo(int id, vec2f normal, LineEquation geometry);
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
        std::map<int, std::vector<LineEquation>> geometry;
        
        /* Returns an index corresponding to data array element that is found at position (<x>, <y>). */
        int posToDataIndex(int x, int y) const;

    public:
        enum {
            // General errors
            E_G_CLEAR = 0,
            E_G_TILE_NOT_FOUND,
            E_G_LINE_NOT_FOUND,
            // Plane file errors
            E_PF_CLEAR = 3,
            E_PF_FAILED_TO_LOAD,
            E_PF_MISSING_SEMICOLON,
            E_PF_INVALID_DIMENSIONS,
            E_PF_INVALID_TILE_DATA,
            E_PF_INVALID_LINE_DATA
        };

        Plane();
        Plane(int width, int height);
        Plane(const std::string& file);
        ~Plane();
        // Sets all tiles id to <tileId>
        void setAllTiles(int tileId);
        // Checks if a point (<x>, <y>) is inside bounds of the plane
        bool inBounds(int x, int y) const;
        int getTile(int x, int y) const;
        int getHeight() const;
        int getWidth() const;
        int maxData() const;  // Returns the highest value of a tile used.
        int setTile(int x, int y, int tileId);
        // Sets properties of line with id <lineId> belonging to a tile with id <tileId>
        int setLine(int tileId, int lineId, float slope, float height);
        // Loads plane tiles data from a specified file
        int_pair load(const std::string& file);
        // Get all line equations for the tile with id <tileId>
        std::vector<LineEquation> getLines(int tileId) const;
};

class DDA_Algorithm {
    private:
        int maxTileDistSquared;
        const Plane* plane;

    public:
        // Array filled by <sendRay()> with information about hit tiles which stopped
        // the algorithm.
        RayHitInfo* hits;

        DDA_Algorithm(const Plane& plane, int maxTileDist);
        ~DDA_Algorithm();
        /* Performs the algorithm for finding plane tiles which a ray walking
         * from point <start> in direction <direction> touch, amount of touched
         * tiles is returned, remember that ray ignores tiles with data 0.
         * Information about next hit tiles is stored in <hits> array. */
        int sendRay(vec2f start, vec2f direction);
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
        /* Constructs a camera and positions it in point (<x>, <y>), then sets
         * its field of view to <fov> rad and initial direction to <dir> rad. */
        Camera(float x, float y, float fov, float dir);
        void changeDirection(float radians);
        void changePosition(vec2f delta);
        float getFieldOfView() const;
        vec2f getPlane() const;
        vec2f getPosition() const;
        vec2f getDirection() const;
        void setDirection(float radians);
        void setPosition(vec2f pos);
};

#endif
