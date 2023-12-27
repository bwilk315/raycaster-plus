
#include "../include/scene.hpp"

namespace rp {

    /********************************************/
    /********** STRUCTURE: PLANAR WALL **********/
    /********************************************/
    
    Wall::Wall() {
        this->line = LineEquation();
        this->color = { 255, 255, 255 };
    }
    Wall::Wall(LineEquation line, SDL_Color color) {
        this->line = line;
        this->color = color;
    }

    /**********************************/
    /********** CLASS: SCENE **********/
    /**********************************/
    
    int Scene::posToDataIndex(int x, int y) const {
        return width * (height - y - 1) + x;
    }
    Scene::Scene() {
        this->width = 1;
        this->height = 1;
        this->tiles = new int;
        this->walls = map<int, vector<Wall>>();
    }
    Scene::Scene(int width, int height) {
        this->width = width;
        this->height = height;
        this->tiles = new int[width * height];
        this->walls = map<int, vector<Wall>>();
    }
    Scene::Scene(const string file) {
        loadFromFile(file);
    }
    Scene::~Scene() {
        if(tiles != nullptr)
            delete[] tiles;
    }
    void Scene::addTileWall(int tile, Wall wall) {
        if(walls.count(tile) == 0)
            walls.insert(pair<int, vector<Wall>>(tile, vector<Wall>()));
        walls.at(tile).push_back(wall);
    }
    bool Scene::contains(int x, int y) const {
        return (x > -1 && x < width) && (y > -1 && y < height);
    }
    int_pair Scene::getTileData(int x, int y) const {
        if(contains(x, y))
            return int_pair(tiles[posToDataIndex(x, y)], Scene::E_CLEAR);
        return int_pair(-1, Scene::E_TILE_NOT_FOUND);
    }
    int_pair Scene::maxTileData() const {
        int max = -1;
        for(int i = 0; i < width * height; i++)
            if(tiles[i] > max)
                max = tiles[i];
        return int_pair(max, (max == -1) ? (Scene::E_TILE_NOT_FOUND) : (Scene::E_CLEAR));
    }
    int Scene::getWidth() const {
        return width;
    }
    int Scene::getHeight() const {
        return height;
    }
    int Scene::setTileData(int x, int y, int data) {
        if(contains(x, y)) {
            tiles[posToDataIndex(x, y)] = data;
            return Scene::E_CLEAR;
        }
        return Scene::E_TILE_NOT_FOUND;
    }
    int Scene::setTileWall(int tile, int index, Wall wall) {
        if(walls.count(tile) == 0)
            return Scene::E_TILE_NOT_FOUND;
        else if((walls.at(tile).empty()) ||
                (index < 0) ||
                (index > walls.at(tile).size() - 1))
            return Scene::E_WALL_NOT_DEFINED;
        walls.at(tile).at(index) = wall;
        return Scene::E_CLEAR;
    }
    int_pair Scene::loadFromFile(const string& file) {
        ifstream stream(file);
        int ln = 0;
        if(!stream.good())
            return int_pair(ln, Scene::E_RPS_FAILED_TO_READ);

        string fileLine;
        int wdh = -1; // World data height (starting from top)
        while(std::getline(stream, fileLine)) {
            ln++;
            // Extract space-separated arguments
            vector<string> args;
            string current = "";
            for(const char& ch : fileLine) {
                if(ch == ' ') {
                    if(!current.empty()) // Omit fancy spaces
                        args.push_back(current);
                    current = "";
                }
                else current += ch;
            }
            if(current != "") // Include the last one
                args.push_back(current);
            // Quit if blank line is encountered
            if(args.size() == 0)
                continue;
            // Interpret the arguments as a single-letter command
            char cmd = args.at(0)[0];
            switch(cmd) {
                // Single line comment
                case '#':
                    continue;
                // Define world size
                case 's':
                    if(args.size() != 3)
                        return int_pair(ln, Scene::E_RPS_INVALID_ARGUMENTS_COUNT);
                    else if(!isFloat(args.at(1)) || !isFloat(args.at(2)))
                        return int_pair(ln, Scene::E_RPS_UNKNOWN_NUMBER_FORMAT);
                    width = (int)stof(args.at(1));
                    height = (int)stof(args.at(2));
                    wdh = height - 1;
                    tiles = new int[width * height];
                    break;
                // Define next world data height (counting from top)
                case 'w':
                    if(wdh == -1)
                        return int_pair(ln, Scene::E_RPS_OPERATION_NOT_AVAILABLE);
                    if(args.size() != width + 1)
                        return int_pair(ln, Scene::E_RPS_INVALID_ARGUMENTS_COUNT);
                    for(int x = 0; x < width; x++) {
                        if(!isFloat(args.at(1 + x)))
                            return int_pair(ln, Scene::E_RPS_UNKNOWN_NUMBER_FORMAT);
                        setTileData(x, wdh, (int)stof(args.at(1 + x)));
                    }
                    wdh--;
                    break;
                // Define properties of a tile with specified data
                case 't':
                    if(args.size() != 12)
                        return int_pair(ln, Scene::E_RPS_INVALID_ARGUMENTS_COUNT);
                    else if(!(
                        isFloat(args.at(1))  && isFloat(args.at(3))  && isFloat(args.at(4))  &&
                        isFloat(args.at(6))  && isFloat(args.at(7))  && isFloat(args.at(9))  &&
                        isFloat(args.at(10)) && isFloat(args.at(11))
                    )) return int_pair(ln, Scene::E_RPS_UNKNOWN_NUMBER_FORMAT);

                    addTileWall(
                        (int)stof(args.at(1)),
                        {
                            LineEquation(
                                stof(args.at(3)),
                                stof(args.at(4)),
                                stof(args.at(6)),
                                stof(args.at(7))
                            ),
                            {
                                (Uint8)stof(args.at(9)),
                                (Uint8)stof(args.at(10)),
                                (Uint8)stof(args.at(11))
                            }
                        }
                    );
                    break;
                default:
                    return int_pair(ln, Scene::E_RPS_OPERATION_NOT_AVAILABLE);
                    break;
            }
        }

        stream.close();
        return int_pair(ln, Scene::E_CLEAR);
    }
    vector<Wall> Scene::getTileWalls(int tile) const {
        if(walls.count(tile) == 0)
            return vector<Wall>();
        return walls.at(tile);
    }

    ostream& operator<<(ostream& stream, const Scene& scene) {
        stream << "Plane(width=" << scene.getWidth() << ",height=" << scene.getHeight() << ")";
        return stream;
    }
}
