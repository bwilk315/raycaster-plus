
#ifndef _RP_SCENE_HPP
#define _RP_SCENE_HPP

#ifdef DEBUG
#include <iostream>
#endif
#include <fstream>
#include <map>
#include <string>
#include <vector>
#include "math.hpp"
#include "texture.hpp"

namespace rp {
    #ifdef DEBUG
    using ::std::ostream;
    using ::std::cout;
    using ::std::endl;
    #endif
    using ::std::map;
    using ::std::vector;
    using ::std::string;
    using ::std::pair;
    using ::std::ifstream;
    using ::std::abs;
    using ::std::stof;

    /**
     * Defines a wall properties.
     * You should call `updateBoundaryPoints` after changing `func` member, it ensures that
     * boundary points `bp0` and `bp1` are up to date.
     */
    struct WallData {
        LinearFunc func;  // Function describing top-down look of the wall
        Vector2 bp0, bp1; // Intersection points of tile boundary and the function
        float hMin, hMax; // Range of wall height to draw
        uint32_t tint;    // Tint color of the wall surface
        uint16_t texId;   // ID number of texture to use (0 indicates no texture)
        bool stopsRay;    // Flag telling if ray should stop after hitting the wall

        WallData();
        WallData(const LinearFunc& func, const uint32_t& tint, float hMin, float hMax, uint16_t texId, bool stopsRay);
        
        void updateBoundaryPoints();
    };
    #ifdef DEBUG
    ostream& operator<<(ostream& stream, const WallData& wd);
    #endif

    /**
     * Provides a bridge of communication between you and Raycaster Plus Scene (RPS), you can load
     * a scene from file or create it manually. You can also modify scene properties at runtime to
     * give it a little bit of life.
     * 
     * RP Scene consists of tile IDs (positive numbers) table with `width` columns and `height` rows,
     * each tile ID represents own set of walls, so you can think of them as looks blueprint for every
     * occurrence of that ID. Walls information is stored in structure called `WallData`.
     */
    class Scene {
        private:
            mutable int error;
            int width;
            int height;
            int* tiles;
            map<int, vector<WallData>> tileWalls; // Tile ID -> Array of walls information
            map<int, Texture> texSources;         // Texture ID -> Texture source object
            map<string, int> texIds;              // File name -> Texture ID

            // Returns index in the tiles array that corresponds to the specified position
            int posAsDataIndex(int x, int y) const;
        public:
            enum {
                E_CLEAR,
                // Raycaster Plus Scene (RPS) file interpreter errors 
                E_RPS_FAILED_TO_READ,
                E_RPS_OPERATION_NOT_AVAILABLE, // Operation depends on something that is not done yet
                E_RPS_UNKNOWN_NUMBER_FORMAT,   // It can be caused by a text not being an actual number
                E_RPS_INVALID_ARGUMENTS_COUNT,
                E_RPS_UNKNOWN_STRING_FORMAT    // Caused by not following string notation where needed
            };

            Scene();
            Scene(int width, int height);
            Scene(const string& rpsFile);
            ~Scene();
            bool checkPosition(int x, int y) const;
            bool setTileId(int x, int y, int tileId);
            int getError() const;
            // Returns ID of a tile at the specified position, or 0 on fail
            int getTileId(int x, int y) const;
            int getWidth() const;
            int getHeight() const;
            // Returns 0 if failed
            int getTextureId(const string& rpsFile) const;
            // Returns empty string if failed
            string getTextureName(int texId) const;
            const Texture* getTextureSource(int texId) const;
            const Texture* getTextureSource(const string& rpsFile) const;
            // Returns vector of walls data defined for the specified tile, or empty vector on fail
            vector<WallData> getTileWalls(int tileId) const;
            // Returns index of the influenced wall, if necessary new wall is created
            int setTileWall(int tileId, int wallIndex, WallData newData);
            // Returns ID of the loaded texture, or 0 on fail
            int loadTexture(const string& pngFile);
            // Returns number of a line which error occurred at, notice that the last line is also
            // returned when interpretation is finished, therefore you should additionally check error state. 
            int loadFromFile(const string& file);
    };
    #ifdef DEBUG
    ostream& operator<<(ostream& stream, const Scene& scene);
    #endif
}

#endif
