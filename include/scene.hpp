
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
    #endif
    using ::std::map;
    using ::std::vector;
    using ::std::string;
    using ::std::pair;
    using ::std::ifstream;
    using ::std::stof;
    using ::std::abs;

    typedef pair<int, int> int_pair;

    struct WallDetails {
        static const uint16_t NO_TEXTURE;

        LinearFunc func;  // Function describing top-down look of the wall
        Vector2 bp0, bp1; // Intersection points of tile boundary and the function
        float hMin, hMax; // Range of wall height to draw
        uint32_t tint;    // Tint color of the wall surface
        uint16_t texId;   // ID number of texture to use (0 indicates no texture)
        bool stopsRay;    // Flag telling if ray should stop after hitting the wall

        WallDetails();
        WallDetails(const LinearFunc& func, const uint32_t& tint, float hMin, float hMax, uint16_t texId, bool stopsRay);
        
        void updateBoundaryPoints();
    };
    #ifdef DEBUG
    ostream& operator<<(ostream& stream, const WallDetails& wd);
    #endif

    class Scene {
        private:
            int width;
            int height;
            int* tiles;
            map<int, vector<WallDetails>> walls;
            map<int, Texture> textures;
            map<string, int> cachedFiles; // Map of texture file names and their existing IDs

            int posAsDataIndex(int x, int y) const;
        public:
            static const int INVALID_ID;
            enum {
                E_CLEAR,
                E_TILE_NOT_FOUND,
                // Raycaster Plus Scene file interpreter errors 
                E_RPS_FAILED_TO_READ,
                E_RPS_OPERATION_NOT_AVAILABLE, // Operation depends on something that is not done yet
                E_RPS_UNKNOWN_NUMBER_FORMAT,   // It can be caused by a text not being an actual number
                E_RPS_INVALID_ARGUMENTS_COUNT,
                E_RPS_UNKNOWN_STRING_FORMAT    // Caused by not following string notation where needed
            };

            Scene();
            Scene(int width, int height);
            Scene(const string& file);
            ~Scene();
            bool checkPosition(int x, int y) const;
            int getTileData(int x, int y) const;
            int getWidth() const;
            int getHeight() const;
            int maxTileData() const;
            const Texture* getTexture(int id) const;
            vector<WallDetails> getTileWalls(int tile) const;
            int setTileWall(int tile, int wall, WallDetails details); // Returns index of the influenced wall
            int loadTexture(const string& file); // Returns ID of the loaded texture
            int setTileData(int x, int y, int data);
            int_pair loadFromFile(const string& file);
    };
    #ifdef DEBUG
    ostream& operator<<(ostream& stream, const Scene& scene);
    #endif
}

#endif
