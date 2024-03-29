
#ifndef _RPGE_SCENE_HPP
#define _RPGE_SCENE_HPP

#include <fstream>
#include <map>
#include <string>
#include <vector>
#include <SDL2/SDL_image.h>
#include "RPGE_globals.hpp"
#include "RPGE_math.hpp"

namespace rpge {
    using ::std::map;
    using ::std::vector;
    using ::std::string;
    using ::std::pair;
    using ::std::ifstream;
    using ::std::abs;
    using ::std::make_pair;
    using ::std::stof;

    /**
     * Defines a wall properties.
     * You should call `updateMetrics` after changing `func` member, it ensures that variables responsible for
     * proper wall texturing (`pivot` and `length`) are up to date.
     */
    struct WallData {
        LinearFunc func;  // Function describing top-down look of the wall
        Vector2 pivot;    // Point located in the left half of a tile, indicates the wall beginning
        float length;     // Length of a wall
        float hMin, hMax; // Range of wall height to draw
        uint32_t tint;    // Tint color of the wall surface
        uint16_t texId;   // ID number of texture to use (0 indicates no texture)
        bool stopsRay;    // Flag telling if ray should stop after hitting the wall

        WallData();
        WallData(const LinearFunc& func, const uint32_t& tint, float hMin, float hMax, uint16_t texId, bool stopsRay);

        void updateMetrics();
    };
    #ifdef DEBUG
    ostream& operator<<(ostream& stream, const WallData& wd);
    #endif

    /**
     * Provides a bridge of communication between you and Raycaster Plus Scene (RPS), you can load
     * a scene from file or create it manually. You can also modify scene properties at runtime to
     * give it a little bit of life.
     * 
     * RP Scene consists of tile IDs (non-negative numbers) table with `width` columns and `height` rows,
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
            map<int, SDL_Texture*> texSources;    // Texture ID -> Pointer to texture structure
            map<string, int> texIds;              // File name -> Texture ID
            vector<int> tileIds;                  // All types of tile IDs
            SDL_Renderer* sdlRend;

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

            Scene(SDL_Renderer* sdlRend);
            Scene(SDL_Renderer* sdlRend, int width, int height);
            Scene(SDL_Renderer* sdlRend, const string& rpsFile);
            ~Scene();

            /* Returns if tile location ( `x`, `y` ) is included in the scene bounds */
            bool               checkPosition(int x, int y) const;

            /* Appends given wall definition `wd` to collection of walls for tile with ID `tileId`, returns wall
             * index assigned to the created wall that can be later used to obtain it back from vector returned
             * by `getTileWalls` method. */
            int                createTileWall(int tileId, const WallData& wd);
            
            /* Sets ID of a tile localized at ( `x`, `y` ) to `tileId`, returns whether operation was
            * successfull. This function does not override source file.  */
            bool               setTileId(int x, int y, int tileId);
                
            /* Returns latest error code set by the class instance */
            int                getError() const;
                
            /* Returns ID of a tile localized at ( `x`, `y` ) if possible, otherwise returns 0 */
            int                getTileId(int x, int y) const;
                
            /* Returns width of the scene in tiles */
            int                getWidth() const;

            /* Returns height of the scene in tiles */
            int                getHeight() const;

            /* Returns array index of a texture with file name `file` if it is loaded, otherwise
            * returns 0. */
            int                getTextureId(const string& file) const;
            
            /* Returns file name of a texture with array index of `texId` if it is loaded, otherwise
            * returns empty string. */
            string             getTextureName(int texId) const;

            SDL_Texture*       getTextureSource(int texId);

	        /* Returns pointer to a vector holding all types of tile IDs */
            const vector<int>* getTileIds() const;

	        /* Returns pointer to a vector filled with wall definitions for tile with ID `tileId`, or null
             * pointer if there are no walls defined. You really should not change vector structure, but feel
             * free to edit its elements by reference. */
            vector<WallData>*  getTileWalls(int tileId);

	        /* Loads texture from file `file` to an array. Returns array index at which the texture was
	         * loaded but incremented by one, if failed returns 0. */
            int                loadTexture(const string& file);

	        /* Loads scene from RPS (Raycaster Plus Scene) file `rpsFile`, returns line at which interpretation
	         * error occurred or the last line with error not set. */ 
            int                loadFromFile(const string& rpsFile);
    };
    #ifdef DEBUG
    ostream& operator<<(ostream& stream, const Scene& scene);
    #endif
}

#endif
