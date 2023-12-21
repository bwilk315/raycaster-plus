
#ifndef _GAME_HPP
#define _GAME_HPP

#include <iostream> // Ekhem
#include <fstream>
#include <cmath>
#include <vector>
#include <map>
#include <SDL2/SDL_pixels.h>
#include "../include/math.hpp"

typedef std::pair<int, int> int_pair;

struct LineEquation {
    int id;          // Number indicating properties of a line
    float slope;     // Line growth rate
    float intercept; // Displacement along Y axis

    LineEquation();
    LineEquation(int id, float slope, float intercept);
    vec2f intersection(const LineEquation& other) const;
};

struct RayHitInfo {
    float distance; // Distance traveled by a ray
    vec2i tile;     // Hit tile position
    vec2f point;    // Global point hit by the ray
    bool side;      // Flag indicating if ray hit the tile from E/W direction

    RayHitInfo();
    RayHitInfo(float distance, vec2i tile, vec2f point, bool side);
};

/* Describes properties of vertical line being a part of some planar wall */
struct WallStripeInfo {
    int tileId;     // Number indicating tile properties
    int lineId;     // This specifies line properties instead
    float distance; // Distance from a wall
    vec2f normal;   // Vector telling in which direction a wall is facing

    WallStripeInfo();
    WallStripeInfo(int tileId, int lineId, float distance, vec2f normal);
    bool operator<(const WallStripeInfo& other) const;
    bool operator>(const WallStripeInfo& other) const;
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
        std::map<int, SDL_Color> lineColors;

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
            E_PF_INVALID_LINE_DATA,
            E_PF_INVALID_COLOR_DATA
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
        SDL_Color getLineColor(int lineId) const;
        // Get all line equations for the tile with id <tileId>
        std::vector<LineEquation> getLines(int tileId) const;
};

class DDA_Algorithm {
    private:
        int maxTileDistSquared; // Only square is useful
        const Plane* plane;

    public:
        // Array filled by <sendRay()> with information about hit tiles which stop the algorithm.
        RayHitInfo* hits = nullptr;

        DDA_Algorithm(const Plane& plane, int maxTileDist);
        ~DDA_Algorithm();
        /* Performs the algorithm for finding plane tiles which a ray walking from point <start>
         * in direction <direction> touch, amount of touched tiles is returned, remember that ray
         * ignores tiles with data 0. Information about next hit tiles is stored in <hits> array. */
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
