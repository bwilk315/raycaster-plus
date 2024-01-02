
#include "../include/scene.hpp"

namespace rp {

    /******************************************/
    /********** STRUCTURE: WALL DATA **********/
    /******************************************/

    WallData::WallData() {
        this->func = LinearFunc();
        this->tint = 0;
        this->bp0 = Vector2::ZERO;
        this->bp1 = Vector2::ZERO;
        this->hMin = 0;
        this->hMax = 1;
        this->texId = 0;
        this->stopsRay = true;
    }
    WallData::WallData(const LinearFunc& func, const uint32_t& tint, float hMin, float hMax, uint16_t texId, bool stopsRay) {
        this->func = func;
        this->tint = tint;
        this->hMin = hMin; 
        this->hMax = hMax;
        this->texId = texId;
        this->stopsRay = stopsRay;
        updateBoundaryPoints();
    }
    void WallData::updateBoundaryPoints() {
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
    ostream& operator<<(ostream& stream, const WallData& wd) {
        stream << "WallData(func=" << wd.func << ", tint=" << wd.tint << ", bp0=" << wd.bp0;
        stream << ", bp1=" << wd.bp1 << ", hMin=" << wd.hMin << ", hMax=" << wd.hMax << ", texId=";
        stream << wd.texId << ", stopsRay=" << wd.stopsRay << ")";
        return stream;
    }
    #endif

    /**********************************/
    /********** CLASS: SCENE **********/
    /**********************************/

    int Scene::posAsDataIndex(int x, int y) const {
        return width * (height - y - 1) + x;
    }
    Scene::Scene() {
        this->error = E_CLEAR;
        this->width = 0;
        this->height = 0;
        this->tiles = nullptr;
        this->tileWalls = map<int, vector<WallData>>();
        this->texSources = map<int, Texture>();
        this->texIds = map<string, int>();
    }
    Scene::Scene(int width, int height) {
        this->error = E_CLEAR;
        this->width = width;
        this->height = height;
        this->tiles = new int[width * height];
        this->tileWalls = map<int, vector<WallData>>();
        this->texSources = map<int, Texture>();
        this->texIds = map<string, int>();
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
    bool Scene::setTileId(int x, int y, int tileId) {
        if(checkPosition(x, y)) {
            tiles[posAsDataIndex(x, y)] = tileId;
            return true;
        }
        return false;
    }
    int Scene::getError() const {
        return error;
    }
    int Scene::getTileId(int x, int y) const {
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
    const Texture* Scene::getTexture(int texId) const {
        if(texSources.count(texId) == 0)
            return nullptr;
        return &texSources.at(texId);
    }
    const Texture* Scene::getTexture(const string& rpsFile) const {
        if(texIds.count(rpsFile) == 0)
            return nullptr;
        return &texSources.at(texIds.at(rpsFile));
    }
    vector<WallData> Scene::getTileWalls(int tileId) const {
        if(tileWalls.count(tileId) == 0)
            return vector<WallData>();
        return tileWalls.at(tileId);
    }
    int Scene::setTileWall(int tileId, int wallIndex, WallData newData) {
        if(tileWalls.count(tileId) == 0)
            tileWalls.insert(pair<int, vector<WallData>>(tileId, vector<WallData>()));
        // Add new wall details entry if needed, index of influenced wall is returned
        size_t wallsCount = tileWalls.at(tileId).empty() ? 0 : tileWalls.at(tileId).size();
        if(wallIndex < 0 || wallIndex > wallsCount - 1) {
            tileWalls.at(tileId).push_back(newData);
            return wallsCount;
        }
        tileWalls.at(tileId).at(wallIndex) = newData;
        return wallIndex;
    }
    int Scene::loadTexture(const string& pngFile) {
        if(texIds.count(pngFile))
            return texIds.at(pngFile);
        // Add a new texture entry, if loading failed then erase it, returns the texture ID
        int id = texIds.size() + 1;
        texSources.insert(pair<int, Texture>(id, Texture()));
        texSources.at(id).loadFromFile(pngFile);
        if(texSources.at(id).getError()) {
            texSources.erase(id);
            return 0;
        }
        texIds.insert(pair<string, int>(pngFile, id));
        return id;
    }
    int Scene::loadFromFile(const string& file) {
        error = E_CLEAR;
        ifstream stream(file);
        int ln = 0;
        if(!stream.good()) {
            error = E_RPS_FAILED_TO_READ;
            return ln;
        }

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
                    if(args.size() != 3) {
                        error = E_RPS_INVALID_ARGUMENTS_COUNT;
                        return ln;
                    } else if(!isFloat(args.at(1)) || !isFloat(args.at(2))) {
                        error = E_RPS_UNKNOWN_NUMBER_FORMAT;
                        return ln;
                    }
                    width = (int)stof(args.at(1));
                    height = (int)stof(args.at(2));
                    wdh = height - 1;
                    tiles = new int[width * height];
                    break;
                }
                // Define next world data height (counting from top)
                case 'w': {
                    if(wdh == -1) {
                        error = E_RPS_OPERATION_NOT_AVAILABLE;
                        return ln;
                    }
                    if(args.size() != width + 1) {
                        error = E_RPS_INVALID_ARGUMENTS_COUNT;
                        return ln;
                    }
                    for(int x = 0; x < width; x++) {
                        if(!isFloat(args.at(1 + x))) {
                            error = E_RPS_UNKNOWN_NUMBER_FORMAT;
                            return ln;
                        }
                        setTileId(x, wdh, (int)stof(args.at(1 + x)));
                    }
                    wdh--;
                    break;
                }
                // Define properties of a tile with specified data
                case 't': {
                    if(args.size() != 21) {
                        error = E_RPS_INVALID_ARGUMENTS_COUNT;
                        return ln;
                    } else if(!(
                        isFloat(args.at(1))  && isFloat(args.at(3))  && isFloat(args.at(4))  &&
                        isFloat(args.at(6))  && isFloat(args.at(7))  && isFloat(args.at(8))  &&
                        isFloat(args.at(9))  && isFloat(args.at(10)) && isFloat(args.at(11)) &&
                        isFloat(args.at(13)) && isFloat(args.at(15)) && isFloat(args.at(16)) &&
                        isFloat(args.at(17)) && isFloat(args.at(18))
                    )) {
                        error = E_RPS_UNKNOWN_NUMBER_FORMAT;
                        return ln;
                    }

                    string text = args.at(20);
                    int tLen = text.length();
                    if(tLen < 2 || text[0] != '"' || text[tLen - 1] != '"') {
                        error = E_RPS_UNKNOWN_STRING_FORMAT;
                        return ln;
                    }

                    string textureFile = text.substr(1, tLen - 2); // Without double apostrophes
                    uint16_t assignedId = loadTexture(textureFile);
                    setTileWall(
                        (int)stof(args.at(1)),
                        -1, // Enforce new wall entry creation (to not override existing ones)
                        WallData(
                            LinearFunc(
                                stof(args.at(3)),
                                stof(args.at(4)),
                                stof(args.at(6)),
                                stof(args.at(7)),
                                stof(args.at(8)),
                                stof(args.at(9))
                            ),
                            encodeRGBA(
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
                    error = E_RPS_OPERATION_NOT_AVAILABLE;
                    return ln;
                    break;
                }
            }
        }

        stream.close();
        return ln;
    }
    #ifdef DEBUG
    ostream& operator<<(ostream& stream, const Scene& scene) {
        stream << "Plane(width=" << scene.getWidth() << ",height=" << scene.getHeight() << ")";
        return stream;
    }
    #endif
}
