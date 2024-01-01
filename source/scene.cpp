
#include "../include/scene.hpp"

namespace rp {

    /*********************************************/
    /********** STRUCTURE: WALL DETAILS **********/
    /*********************************************/
    
    const uint16_t WallDetails::NO_TEXTURE = 0;

    WallDetails::WallDetails() {
        this->func = LinearFunc();
        this->tint = 0;
        this->bp0 = Vector2::ZERO;
        this->bp1 = Vector2::ZERO;
        this->hMin = 0;
        this->hMax = 1;
        this->texId = WallDetails::NO_TEXTURE;
        this->stopsRay = true;
    }
    WallDetails::WallDetails(const LinearFunc& func, const uint32_t& tint, float hMin, float hMax, uint16_t texId, bool stopsRay) {
        this->func = func;
        this->tint = tint;
        this->hMin = hMin; 
        this->hMax = hMax;
        this->texId = texId;
        this->stopsRay = stopsRay;
        updateBoundaryPoints();
    }
    void WallDetails::updateBoundaryPoints() {
        if(func.slope == 0) {
            bp0.x = 0;
            bp0.y = func.height;
            bp1.x = 1;
            bp1.y = func.height;
            return;
        }
        bp0.x = -1 * func.height / func.slope;
        if(bp0.x < 0) {
            bp0.x = 0;
            bp0.y = func.height;
        } else if(bp0.x > 1) {
            bp0.x = 1;
            bp0.y = func.slope + func.height;
        } else {
            bp0.y = 0;
        }
        bp1.x = func.slope == 0 ? 1 : ((1 - func.height) / func.slope);
        if(bp1.x < 0) {
            bp1.x = 0;
            bp1.y = func.height;
        } else if(bp1.x > 1) {
            bp1.x = 1;
            bp1.y = func.slope + func.height; 
        } else {
            bp1.y = 1;
        }
    }
    #ifdef DEBUG
    ostream& operator<<(ostream& stream, const WallDetails& wd) {
        stream << "WallDetails(func=" << wd.func << ", tint=" << wd.tint << ", bp0=" << wd.bp0;
        stream << ", bp1=" << wd.bp1 << ", hMin=" << wd.hMin << ", hMax=" << wd.hMax << ", texId=";
        stream << wd.texId << ", stopsRay=" << wd.stopsRay << ")";
        return stream;
    }
    #endif

    /**********************************/
    /********** CLASS: SCENE **********/
    /**********************************/
    
    const int Scene::INVALID_ID = -1;

    int Scene::posAsDataIndex(int x, int y) const {
        return width * (height - y - 1) + x;
    }
    Scene::Scene() {
        this->width = 0;
        this->height = 0;
        this->tiles = nullptr;
        this->walls = map<int, vector<WallDetails>>();
        this->textures = map<int, Texture>();
        this->cachedFiles = map<string, int>();
    }
    Scene::Scene(int width, int height) {
        this->width = width;
        this->height = height;
        this->tiles = new int[width * height];
        this->walls = map<int, vector<WallDetails>>();
        this->textures = map<int, Texture>();
        this->cachedFiles = map<string, int>();
    }
    Scene::Scene(const string& file) : Scene() {
        loadFromFile(file);
    }
    Scene::~Scene() {
        if(tiles != nullptr)
            delete[] tiles;
    }
    bool Scene::checkPosition(int x, int y) const {
        return (x > -1 && x < width) && (y > -1 && y < height);
    }
    int Scene::getTileData(int x, int y) const {
        if(checkPosition(x, y))
            return tiles[posAsDataIndex(x, y)];
        return 0;
    }
    int Scene::getWidth() const {
        return width;
    }
    int Scene::getHeight() const {
        return height;
    }
    int Scene::maxTileData() const {
        int max = -1;
        for(int i = 0; i < width * height; i++)
            if(tiles[i] > max)
                max = tiles[i];
        return max;
    }
    const Texture* Scene::getTexture(int id) const {
        if(textures.count(id) == 0)
            return nullptr;
        return &textures.at(id);
    }
    vector<WallDetails> Scene::getTileWalls(int tile) const {
        if(walls.count(tile) == 0)
            return vector<WallDetails>();
        return walls.at(tile);
    }
    int Scene::setTileWall(int tile, int wall, WallDetails details) {
        if(walls.count(tile) == 0)
            walls.insert(pair<int, vector<WallDetails>>(tile, vector<WallDetails>()));
        // Add new wall details entry if needed, index of influenced wall is returned
        size_t wallsCount = walls.at(tile).empty() ? 0 : walls.at(tile).size();
        if(wall < 0 || wall > wallsCount - 1) {
            walls.at(tile).push_back(details);
            return wallsCount;
        }
        walls.at(tile).at(wall) = details;
        return wall;
    }
    int Scene::loadTexture(const string& file) {
        if(cachedFiles.count(file) != 0)
            return cachedFiles.at(file);
        // Add a new texture entry, if loading failed then erase it, returns the texture ID
        uint16_t id = cachedFiles.size() + 1;
        textures.insert(pair<uint16_t, Texture>(id, Texture()));
        if(textures.at(id).loadFromFile(file) == Texture::E_CLEAR) {
            cachedFiles.insert(pair<string, int>(file, id));
            return id;
        }
        textures.erase(id);
        return Scene::INVALID_ID;
    }
    int Scene::setTileData(int x, int y, int data) {
        if(checkPosition(x, y)) {
            tiles[posAsDataIndex(x, y)] = data;
            return Scene::E_CLEAR;
        }
        return Scene::E_TILE_NOT_FOUND;
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
                case 's': {
                    if(args.size() != 3)
                        return int_pair(ln, Scene::E_RPS_INVALID_ARGUMENTS_COUNT);
                    else if(!isFloat(args.at(1)) || !isFloat(args.at(2)))
                        return int_pair(ln, Scene::E_RPS_UNKNOWN_NUMBER_FORMAT);
                    width = (int)stof(args.at(1));
                    height = (int)stof(args.at(2));
                    wdh = height - 1;
                    tiles = new int[width * height];
                    break;
                }
                // Define next world data height (counting from top)
                case 'w': {
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
                }
                // Define properties of a tile with specified data
                case 't': {
                    if(args.size() != 21)
                        return int_pair(ln, Scene::E_RPS_INVALID_ARGUMENTS_COUNT);
                    else if(!(
                        isFloat(args.at(1))  && isFloat(args.at(3))  && isFloat(args.at(4))  &&
                        isFloat(args.at(6))  && isFloat(args.at(7))  && isFloat(args.at(8))  &&
                        isFloat(args.at(9))  && isFloat(args.at(10)) && isFloat(args.at(11)) &&
                        isFloat(args.at(13)) && isFloat(args.at(15)) && isFloat(args.at(16)) &&
                        isFloat(args.at(17)) && isFloat(args.at(18))
                    )) return int_pair(ln, Scene::E_RPS_UNKNOWN_NUMBER_FORMAT);

                    string text = args.at(20);
                    int tLen = text.length();
                    if(tLen < 2 || text[0] != '"' || text[tLen - 1] != '"')
                        return int_pair(ln, Scene::E_RPS_UNKNOWN_STRING_FORMAT);

                    string textureFile = text.substr(1, tLen - 2); // Without double apostrophes
                    uint16_t assignedId = loadTexture(textureFile);
                    setTileWall(
                        (int)stof(args.at(1)),
                        -1, // Enforce new wall entry creation (to not override existing ones)
                        WallDetails(
                            LinearFunc(
                                clamp(stof(args.at(3)), -1 * LinearFunc::MAX_SLOPE, LinearFunc::MAX_SLOPE),
                                clamp(stof(args.at(4)), -1 * LinearFunc::MAX_SLOPE + 1, LinearFunc::MAX_SLOPE - 1),
                                stof(args.at(6)),
                                stof(args.at(7)),
                                stof(args.at(8)),
                                stof(args.at(9))
                            ),
                            Texture::getColorAsNumber(
                                (uint8_t)stof(args.at(15)),
                                (uint8_t)stof(args.at(16)),
                                (uint8_t)stof(args.at(17)),
                                (uint8_t)stof(args.at(18))
                            ),
                            stof(args.at(10)),
                            stof(args.at(11)),
                            assignedId,
                            (bool)stof(args.at(13))
                        )
                    );
                    break;
                }
                default: {
                    return int_pair(ln, Scene::E_RPS_OPERATION_NOT_AVAILABLE);
                    break;
                }
            }
        }

        stream.close();
        return int_pair(ln, Scene::E_CLEAR);
    }
    #ifdef DEBUG
    ostream& operator<<(ostream& stream, const Scene& scene) {
        stream << "Plane(width=" << scene.getWidth() << ",height=" << scene.getHeight() << ")";
        return stream;
    }
    #endif
}
