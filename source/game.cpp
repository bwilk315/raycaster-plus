
#include "../include/game.hpp"

/**********************************************/
/********** STRUCTURE: LINE EQUATION **********/
/**********************************************/

LineEquation::LineEquation() {
    this->id = -1;
    this->slope = 0;
    this->intercept = 0;
}
LineEquation::LineEquation(int id, float slope, float intercept) {
    this->id = id;
    this->slope = slope;
    this->intercept = intercept;
}
vec2f LineEquation::intersection(const LineEquation& other) const {
    float x = (this->intercept - other.intercept) / (other.slope - this->slope);
    return vec2f(x, this->slope * x + this->intercept);
}

/*********************************************/
/********** STRUCTURE: RAY HIT INFO **********/
/*********************************************/

RayHitInfo::RayHitInfo() {
    this->distance = -1;
    this->tile = vec2i(0, 0);
    this->point = vec2f::ZERO;
    this->side = false;
}
RayHitInfo::RayHitInfo(float distance, vec2i tile, vec2f point, bool side) {
    this->distance = distance;
    this->tile = tile;
    this->point = point;
    this->side = side;
}

/******************************************/
/********** STRUCTURE: WALL INFO **********/
/******************************************/

WallStripeInfo::WallStripeInfo() {
    this->tileId = -1;
    this->lineId = -1;
    this->distance = -1;
    this->normal = vec2f::ZERO;
}
WallStripeInfo::WallStripeInfo(int tileId, int lineId, float distance, vec2f normal) {
    this->tileId = tileId;
    this->lineId = lineId;
    this->distance = distance;
    this->normal = normal;
}
bool WallStripeInfo::operator<(const WallStripeInfo& other) const {
    return this->distance < other.distance;
}
bool WallStripeInfo::operator>(const WallStripeInfo& other) const {
    return this->distance > other.distance;
}

/**********************************/
/********** CLASS: PLANE **********/
/**********************************/

int Plane::posToDataIndex(int x, int y) const {
    return this->width * (this->height - y - 1) + x;
}
Plane::Plane() {
    this->width = -1;
    this->height = -1;
}
Plane::Plane(int width, int height) {
    this->width = width;
    this->height = height;
    this->tiles = new int[width * height];
    this->geometry = std::map<int, std::vector<LineEquation>>();
    this->lineColors = std::map<int, SDL_Color>();
}
Plane::Plane(const std::string& file) {
    this->geometry = std::map<int, std::vector<LineEquation>>();
    this->lineColors = std::map<int, SDL_Color>();
    this->load(file);
}
Plane::~Plane() {
    if(this->tiles != nullptr)
        delete[] this->tiles;
}
void Plane::setAllTiles(int tileId) {
    for(int i = 0; i < this->width * this->height; i++) {
        this->tiles[i] = tileId;
    }
}
bool Plane::inBounds(int x, int y) const {
    return (x > -1 && x < this->width) && (y > -1 && y < this->height);
}
int Plane::getTile(int x, int y) const {
    if(inBounds(x, y))
        return this->tiles[this->posToDataIndex(x, y)];
    return -1;
}
int Plane::getHeight() const {
    return this->height;
}
int Plane::getWidth() const {
    return this->width;
}
int Plane::maxData() const {
    int max = 0;
    for(int i = 0; i < this->width * this->height; i++) {
        if(this->tiles[i] > max) max = this->tiles[i];
    }
    return max;
}
int Plane::setTile(int x, int y, int tileId) {
    if(this->inBounds(x, y)) {
        this->tiles[this->posToDataIndex(x, y)] = tileId;
        return Plane::E_G_CLEAR;
    }
    return Plane::E_G_TILE_NOT_FOUND;
}
int Plane::setLine(int tileId, int lineId, float slope, float intercept) {
    for(auto& ve : this->geometry)
        if(ve.first == tileId) {
            if(lineId < 0 || lineId > ve.second.size() - 1)
                return Plane::E_G_LINE_NOT_FOUND;
            ve.second.at(lineId).slope = slope;
            ve.second.at(lineId).intercept = intercept;
            return Plane::E_G_CLEAR;
        }
    return Plane::E_G_TILE_NOT_FOUND;
}
int_pair Plane::load(const std::string& file) {
    std::ifstream stream(file);
    int line = 1;
    if(!stream.good()) {
        return int_pair(line, Plane::E_PF_FAILED_TO_LOAD);
    }

    const std::string SEMICOLON = ";";
    std::string bf0, bf1, bf2, bf3; // Buffers for taking input
    // Read world dimensions
    stream >> bf0; // Width
    stream >> bf1; // Height
    stream >> bf2; // Semicolon

    if(bf2 != SEMICOLON)
        return int_pair(line, Plane::E_PF_MISSING_SEMICOLON);
    if(!isNumber(bf0) || !isNumber(bf1))
        return int_pair(line, Plane::E_PF_INVALID_DIMENSIONS);
    this->width = (int)std::stof(bf0);
    this->height = (int)std::stof(bf1);
    this->tiles = new int[this->width * this->height];

    // Read world tiles data
    for(int y = this->height - 1; y != -1; y--) {
        line++;
        for(int x = 0; x != this->width; x++) {
            stream >> bf0;
            if(!isNumber(bf0))
                return int_pair(line, Plane::E_PF_INVALID_TILE_DATA);
            this->setTile(x, y, (int)std::stof(bf0));
        }
        stream >> bf0;
        if(bf0 != SEMICOLON)
            return int_pair(line, Plane::E_PF_MISSING_SEMICOLON);
    }

    // Read line equations (aka wall geometries)
    while(true) {
        line++;
        stream >> bf0; // Tile (aka wall) id
        if(bf0 == SEMICOLON) // Two semicolons in a row indicate termination
            break;
        stream >> bf1; // Line identification number
        stream >> bf2; // Line slope
        stream >> bf3; // Line intercept
        
        if(!isNumber(bf0) || !isNumber(bf1) || !isNumber(bf2) || !isNumber(bf3)) 
            return int_pair(line, Plane::E_PF_INVALID_LINE_DATA);
        int tileId = (int)std::stof(bf0);
        // Create vector entry if it does not exist
        if(this->geometry.count(tileId) == 0)
            this->geometry.insert(std::pair<int, std::vector<LineEquation>>(
                tileId, std::vector<LineEquation>()
            ));
        // Add a new line equation for a specified tile id
        this->geometry.at(tileId).push_back(
            LineEquation((int)std::stof(bf1), std::stof(bf2), std::stof(bf3))
        );
        // Ensure semicolon at the end
        stream >> bf0;
        if(bf0 != SEMICOLON)
            return int_pair(line, Plane::E_PF_MISSING_SEMICOLON);
    }

    // Read line colors (very similar to the previous one above)
    while(true) {
        line++;
        stream >> bf0; // Line id
        stream >> bf1; // Red channel
        stream >> bf2; // Green channel
        stream >> bf3; // Blue channel

        if(bf0 == SEMICOLON)
            break;
        else if(!isNumber(bf0) || !isNumber(bf1) || !isNumber(bf2) || !isNumber(bf3))
            return int_pair(line, Plane::E_PF_INVALID_COLOR_DATA);
        int lineId = (int)std::stof(bf0);
        this->lineColors.insert(std::pair<int, SDL_Color>(
            lineId, { (Uint8)std::stof(bf1), (Uint8)std::stof(bf2), (Uint8)std::stof(bf3), 255 }
        ));
        stream >> bf0;
        if(bf0 != SEMICOLON)
            return int_pair(line, Plane::E_PF_MISSING_SEMICOLON);
    }

    stream.close();
    return int_pair(line, Plane::E_PF_CLEAR);
}
SDL_Color Plane::getLineColor(int lineId) const {
    for(const auto& lc : this->lineColors)
        if(lc.first == lineId)
            return lc.second;
    return { 0, 0, 0, 0 };
}
std::vector<LineEquation> Plane::getLines(int id) const {
    for(const auto& ve : this->geometry)
        if(ve.first == id)
            return ve.second;
    return std::vector<LineEquation>();
}

/******************************************/
/********** CLASS: DDA ALGORITHM **********/
/******************************************/

DDA_Algorithm::DDA_Algorithm(const Plane& plane, int maxTileDist) {
    this->plane = &plane;
    this->hits = new RayHitInfo[maxTileDist];
    this->maxTileDistSquared = maxTileDist * maxTileDist;
}
DDA_Algorithm::~DDA_Algorithm() {
    if(this->hits != nullptr)
        delete[] this->hits;
}
int DDA_Algorithm::sendRay(vec2f start, vec2f direction) {
    vec2i mapPos = start.toInt();
    // Assuming the ray direction vector d is normalized (of length 1), pythagoras way of computing
    // these distances, e.g. to by unit in x: sqrt(1 + (dY/dX)^2), can be exchanged with much simpler
    // one: abs(1/dX) (it can be derived manually).
    float deltaDistX = direction.x == 0 ? 1e30 : std::abs(1 / direction.x);
    float deltaDistY = direction.y == 0 ? 1e30 : std::abs(1 / direction.y);
    float sideDistX, sideDistY;
    int stepX, stepY;
    // Calculate step and initial sideDist
    if(direction.x < 0) {
        stepX = -1;
        sideDistX = (start.x - mapPos.x) * deltaDistX;
    } else {
        stepX = 1;
        sideDistX = (1 + mapPos.x - start.x) * deltaDistX;
    }
    if(direction.y < 0) {
        stepY = -1;
        sideDistY = (start.y - mapPos.y) * deltaDistY;
    } else {
        stepY = 1;
        sideDistY = (1 + mapPos.y - start.y) * deltaDistY;
    }
    // Perform Digital Differential Analysis algorithm
    bool side = false; // Has he side of a tile (X axis) got hit?
    int hitCount = 0;
    while(true) {
        if(sideDistX < sideDistY) {
            sideDistX += deltaDistX;
            mapPos.x += stepX;
            side = true;
        } else {
            sideDistY += deltaDistY;
            mapPos.y += stepY;
            side = false;
        }
        // Check if the tile is not exceeded the maximum distance
        int deltaPosX = mapPos.x - start.x;
        int deltaPosY = mapPos.y - start.y;
        if(deltaPosX * deltaPosX + deltaPosY * deltaPosY > maxTileDistSquared)
            break;
        // Process the hit tile data, register it if possible
        if(!this->plane->inBounds(mapPos.x, mapPos.y))
            break;
        int tileData = this->plane->getTile(mapPos.x, mapPos.y);
        if(tileData != 0) {
            // Find the hit point by moving DDA-computed distance along sent ray direction
            float pointDist = side ? sideDistX - deltaDistX : sideDistY - deltaDistY;
            this->hits[hitCount++] = { pointDist, mapPos, start + direction * pointDist, side };
        }
    }
    return hitCount;
}

/***********************************/
/********** CLASS: CAMERA **********/
/***********************************/

Camera::Camera() {
    this->planeMagnitude = 1;
    this->fov = M_PI_2;
    this->plane = vec2f::RIGHT;
    this->direction = vec2f::UP;
    this->position = vec2f::ZERO;
}
Camera::Camera(float x, float y, float fov, float dir) {
    this->planeMagnitude = tanf(fov / 2); // Assuming direction has length of 1
    this->fov = fov;
    this->plane = vec2f::RIGHT * this->planeMagnitude;
    this->direction = vec2f::UP;
    this->position = vec2f(x, y);
    this->setDirection(dir);
}
void Camera::changeDirection(float radians) {
    this->direction = this->direction.rotate(radians);
    this->plane = this->plane.rotate(radians);
}
void Camera::changePosition(vec2f delta) {
    this->position = this->position + delta;
}
void Camera::setDirection(float radians) {
    this->direction = vec2f::UP.rotate(radians);
    this->plane = vec2f::RIGHT.rotate(radians) * this->planeMagnitude;
}
void Camera::setPosition(vec2f pos) {
    this->position = pos;
}
float Camera::getFieldOfView() const {
    return this->fov;
}
vec2f Camera::getPlane() const {
    return this->plane;
}
vec2f Camera::getPosition() const {
    return this->position;
}
vec2f Camera::getDirection() const {
    return this->direction;
}
