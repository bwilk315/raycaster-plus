
#include "../include/plane.hpp"

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
    /********** CLASS: PLANE **********/
    /**********************************/
    
    int Plane::posToDataIndex(int x, int y) const {
        return width * (height - y - 1) + x;
    }
    Plane::Plane() {
        this->width = 1;
        this->height = 1;
        this->tiles = new int;
        this->walls = map<int, vector<Wall>>();
    }
    Plane::Plane(int width, int height) {
        this->width = width;
        this->height = height;
        this->tiles = new int[width * height];
        this->walls = map<int, vector<Wall>>();
    }
    Plane::~Plane() {
        if(tiles != nullptr)
            delete[] tiles;
    }
    void Plane::addTileWall(int tile, Wall wall) {
        if(walls.count(tile) == 0)
            walls.insert(pair<int, vector<Wall>>(tile, vector<Wall>()));
        walls.at(tile).push_back(wall);
    }
    bool Plane::contains(int x, int y) const {
        return (x > -1 && x < width) && (y > -1 && y < height);
    }
    int_pair Plane::getTileData(int x, int y) const {
        if(contains(x, y))
            return int_pair(tiles[this->posToDataIndex(x, y)], Plane::E_CLEAR);
        return int_pair(-1, Plane::E_TILE_NOT_FOUND);
    }
    int_pair Plane::maxTileData() const {
        int max = -1;
        for(int i = 0; i < width * height; i++)
            if(tiles[i] > max) max = tiles[i];
        return int_pair(max, (max == -1) ? (Plane::E_TILE_NOT_FOUND) : (Plane::E_CLEAR));
    }
    int Plane::getHeight() const {
        return height;
    }
    int Plane::getWidth() const {
        return width;
    }
    int Plane::setTileData(int x, int y, int data) {
        if(contains(x, y)) {
            tiles[posToDataIndex(x, y)] = data;
            return Plane::E_CLEAR;
        }
        return Plane::E_TILE_NOT_FOUND;
    }
    int Plane::setTileWall(int tile, int index, Wall wall) {
        if(walls.count(tile) == 0)
            return Plane::E_TILE_NOT_FOUND;
        else if((walls.at(tile).empty()) ||
                (index < 0) ||
                (index > walls.at(tile).size() - 1))
            return Plane::E_WALL_NOT_DEFINED;
        walls.at(tile).at(index) = wall;
        return Plane::E_CLEAR;
    }
    int_pair Plane::loadFromFile(const string& file) {
        ifstream stream(file);
        int ln = 0;
        if(!stream.good())
            return int_pair(ln, Plane::E_PFI_FAILED_TO_READ);

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
                        return int_pair(ln, Plane::E_PFI_INVALID_ARGUMENTS_COUNT);
                    else if(!isFloat(args.at(1)) || !isFloat(args.at(2)))
                        return int_pair(ln, Plane::E_PFI_UNKNOWN_NUMBER_FORMAT);
                    width = (int)stof(args.at(1));
                    height = (int)stof(args.at(2));
                    wdh = height - 1;
                    tiles = new int[width * height];
                    break;
                // Define next world data height (counting from top)
                case 'w':
                    if(wdh == -1)
                        return int_pair(ln, Plane::E_PFI_OPERATION_NOT_AVAILABLE);
                    if(args.size() != width + 1)
                        return int_pair(ln, Plane::E_PFI_INVALID_ARGUMENTS_COUNT);
                    for(int x = 0; x < width; x++) {
                        if(!isFloat(args.at(1 + x)))
                            return int_pair(ln, Plane::E_PFI_UNKNOWN_NUMBER_FORMAT);
                        setTileData(x, wdh, (int)stof(args.at(1 + x)));
                    }
                    wdh--;
                    break;
                // Define properties of a tile with specified data
                case 't':
                    if(args.size() != 12)
                        return int_pair(ln, Plane::E_PFI_INVALID_ARGUMENTS_COUNT);
                    else if(!(
                        isFloat(args.at(1))  && isFloat(args.at(3))  && isFloat(args.at(4))  &&
                        isFloat(args.at(6))  && isFloat(args.at(7))  && isFloat(args.at(9))  &&
                        isFloat(args.at(10)) && isFloat(args.at(11))
                    )) return int_pair(ln, Plane::E_PFI_UNKNOWN_NUMBER_FORMAT);

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
                    return int_pair(ln, Plane::E_PFI_OPERATION_NOT_AVAILABLE);
                    break;
            }
        }

        stream.close();
        return int_pair(ln, Plane::E_CLEAR);
    }
    vector<Wall> Plane::getTileWalls(int tile) const {
        if(walls.count(tile) == 0)
            return vector<Wall>();
        return walls.at(tile);
    }

    ostream& operator<<(ostream& stream, const Plane& plane) {
        stream << "Plane(width=" << plane.getWidth() << ",height=" << plane.getHeight() << ")";
        return stream;
    }
}
