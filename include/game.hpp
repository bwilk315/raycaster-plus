
#ifndef _GAME_HPP
#define _GAME_HPP

#include <iostream>
#include <cstring>
#include <cmath>
#include <vector>
#include <fstream>
#include "../include/math.hpp"

struct LineEquation {
    int data;
    float slope;
    float intercept;

    LineEquation();
    LineEquation(int data, float slope, float intercept);
    vec2f intersection(const LineEquation& other);
};

/* Describes a plane filled with data tiles, each having its unique position
 * in two-dimensional cartesian coordinates system.
 * The origin point (0, 0) is located at the bottom-left corner of the plane,
 * X axis grows when moving right, Y axis do so when moving up. */
class Plane {
    private:
        int width = 0;
        int height = 0;
        int* data = nullptr;
        std::vector<LineEquation> lines;
        
        /* Returns an index corresponding to the data array element that is
         * found at the position (<x>, <y>). */
        int posToDataIndex(int x, int y) const;

    public:

        /* Constructs a plane with dimensions (<width>, <height>).
         * If <indexFill> flag is set, all plane tiles are set to their indices. */
        Plane(int width, int height, bool indexFill = false);
        Plane(const char* planeFile);
        ~Plane();
        void fillData(int value);
        int getData(int x, int y) const;
        LineEquation getLine(int data) const; // Get line equation for tile with given data
        int getHeight() const;
        int getWidth() const;
        bool inBounds(int x, int y) const;  // Checks if a point (x, y) is inside bounds of the plane.
        int maxData() const;  // Returns the highest value of a tile used.
        bool setData(int x, int y, int value);
        bool setLine(int data, float slope, float height);
};

struct DDA_RayHitInfo {
    int data;
    float distance;
    vec2f point;
    bool side;

    DDA_RayHitInfo();
    DDA_RayHitInfo(int data, float distance, vec2f point, bool side);
};

class DDA_Algorithm {
    private:
        int maxTileDistSquared;
        const Plane* plane;

    public:
        // Array filled by <sendRay()> with information about hit tiles which stopped
        // the algorithm.
        DDA_RayHitInfo* hits;

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
