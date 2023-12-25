
#ifndef _PLANE_HPP
#define _PLANE_HPP

#include <fstream>
#include <map>
#include <string>
#include <vector>
#include <SDL2/SDL_pixels.h>
#include "math.hpp"

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
        Wall();
        Wall(LineEquation line, SDL_Color color);
    };

    class Plane {
        private:
            int width = 0;
            int height = 0;
            int* tiles = nullptr;
            map<int, vector<Wall>> walls;

            int posToDataIndex(int x, int y) const;

        public:
            enum {
                // General errors
                E_CLEAR = 0,
                E_TILE_NOT_FOUND,
                E_WALL_NOT_DEFINED,
                // Plane file interpreter errors 
                E_PFI_FAILED_TO_READ = 3,
                E_PFI_OPERATION_NOT_AVAILABLE, // Operation depends on something that is not done yet
                E_PFI_UNKNOWN_NUMBER_FORMAT,   // It can be caused by a text not being an actual number
                E_PFI_INVALID_ARGUMENTS_COUNT
            };

            Plane();
            Plane(int width, int height);
            ~Plane();
            void addTileWall(int tile, Wall wall);
            bool contains(int x, int y) const;
            int_pair getTileData(int x, int y) const;
            int_pair maxTileData() const;
            int getHeight() const;
            int getWidth() const;
            int setTileData(int x, int y, int data);
            int setTileWall(int tile, int index, Wall wall);
            int_pair loadFromFile(const string& file);
            vector<Wall> getTileWalls(int tile) const;
    };

    ostream& operator<<(ostream& stream, const Plane& plane);
}

#endif
