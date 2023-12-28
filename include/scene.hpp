
#ifndef _PLANE_HPP
#define _PLANE_HPP

#include <fstream>
#include <map>
#include <string>
#include <vector>
#include <SDL2/SDL_pixels.h>
#include "math.hpp"
#include "texture.hpp"

namespace rp {
    using ::std::map;
    using ::std::vector;
    using ::std::string;
    using ::std::pair;
    using ::std::ifstream;
    using ::std::ostream;
    using ::std::stof;

    typedef pair<int, int> int_pair;

    struct Wall {
        LineEquation line;
        SDL_Color color;
        string textureFile;
        Wall();
        Wall(LineEquation line, SDL_Color color);
        Wall(LineEquation line, SDL_Color color, const string& textureFile);
    };

    class Scene {
        private:
            int width = 0;
            int height = 0;
            int* tiles = nullptr;
            map<int, vector<Wall>> walls;
            map<string, Texture> textures;

            int posToDataIndex(int x, int y) const;
        public:
            enum {
                // General errors
                E_CLEAR,
                E_TILE_NOT_FOUND,
                E_WALL_NOT_DEFINED,
                // Raycaster Plus Scene file interpreter errors 
                E_RPS_FAILED_TO_READ,
                E_RPS_OPERATION_NOT_AVAILABLE, // Operation depends on something that is not done yet
                E_RPS_UNKNOWN_NUMBER_FORMAT,   // It can be caused by a text not being an actual number
                E_RPS_INVALID_ARGUMENTS_COUNT,
                E_RPS_UNKNOWN_STRING_FORMAT   // Caused by not following string notation where needed
            };

            Scene();
            Scene(int width, int height);
            Scene(const string& file);
            ~Scene();
            void addTileWall(int tile, Wall wall);
            void loadTexture(const string& file);
            bool contains(int x, int y) const;
            int_pair getTileData(int x, int y) const;
            int_pair maxTileData() const;
            const Texture* getTexture(const string& file) const;
            int getWidth() const;
            int getHeight() const;
            int setTileData(int x, int y, int data);
            int setTileWall(int tile, int index, Wall wall);
            int_pair loadFromFile(const string& file);
            vector<Wall> getTileWalls(int tile) const;
    };

    ostream& operator<<(ostream& stream, const Scene& scene);
}

#endif
