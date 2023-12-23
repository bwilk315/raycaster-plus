
#include "../include/game.hpp"

/**********************************************/
/********** STRUCTURE: LINE EQUATION **********/
/**********************************************/

LineEquation::LineEquation() {
    this->id = -1;
    this->slope = 0;
    this->intercept = 0;
}
LineEquation::LineEquation(int id, float slope, float intercept, float domainStart, float domainEnd) {
    this->id = id;
    this->slope = slope;
    this->intercept = intercept;
    this->domainStart = domainStart;
    this->domainEnd = domainEnd;
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
}
RayHitInfo::RayHitInfo(float distance, vec2i tile, vec2f point, bool side) {
    this->distance = distance;
    this->tile = tile;
    this->point = point;
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
int Plane::setLine(int tileId, int lineId, float slope, float intercept, float domainStart, float domainEnd) {
    for(auto& ve : this->geometry)
        if(ve.first == tileId)
            for(LineEquation& le : ve.second)
                if(le.id == lineId) {
                    le.slope = slope;
                    le.intercept = intercept;
                    le.domainStart = domainStart == -1 ? le.domainStart : clamp(domainStart, 0, 1);
                    le.domainEnd = domainEnd == -1 ? le.domainEnd : clamp(domainEnd, 0, 1);
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
        
        // Load tileId, lineId, slope and intercept
        if(!isNumber(bf0) || !isNumber(bf1) || !isNumber(bf2) || !isNumber(bf3)) 
            return int_pair(line, Plane::E_PF_INVALID_LINE_DATA);
        int tileId = (int)std::stof(bf0);
        int lineId = (int)std::stof(bf1);
        float slope = std::stof(bf2);
        float intercept = std::stof(bf3);

        // Load domain start and end values
        stream >> bf0;
        stream >> bf1;
        if(!isNumber(bf0) || !isNumber(bf1))
            return int_pair(line, Plane::E_PF_INVALID_LINE_DATA);
        float domainStart = std::stof(bf0);
        float domainEnd = std::stof(bf1);

        // Create vector entry if it does not exist
        if(this->geometry.count(tileId) == 0)
            this->geometry.insert(std::pair<int, std::vector<LineEquation>>(
                tileId, std::vector<LineEquation>()
            ));
        // Add a new line equation for a specified tile id
        this->geometry.at(tileId).push_back(
            LineEquation(lineId, slope, intercept, domainStart, domainEnd)
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

DDA_Algorithm::DDA_Algorithm(const Plane* plane, int maxTileDist) {
    this->plane = plane;
    this->maxTileDistSquared = maxTileDist * maxTileDist;
}
void DDA_Algorithm::init(const vec2f& start, const vec2f& direction) {
    this->start = start;
    this->direction = direction;
    this->rayFlag = DDA_Algorithm::RF_CLEAR;
    this->initialized = true;

    planePosX = (int)start.x;
    planePosY = (int)start.y;
    deltaDistX = direction.x == 0 ? 1e30 : std::abs(1 / direction.x);
    deltaDistY = direction.y == 0 ? 1e30 : std::abs(1 / direction.y);
    // Set initial distances and stepping direction
    if(direction.x < 0) {
        stepX = -1;
        sideDistX = (start.x - this->planePosX) * deltaDistX;
    } else {
        stepX = 1;
        sideDistX = (1 + this->planePosX - start.x) * deltaDistX;
    }
    if(direction.y < 0) {
        stepY = -1;
        sideDistY = (start.y - this->planePosY) * deltaDistY;
    } else {
        stepY = 1;
        sideDistY = (1 + this->planePosY - start.y) * deltaDistY;
    }
}
RayHitInfo DDA_Algorithm::next() {
    // Step along appropriate axis
    if(sideDistX < sideDistY) {
        sideDistX += deltaDistX;
        planePosX += stepX;
        rayFlag = DDA_Algorithm::RF_SIDE;
    } else {
        sideDistY += deltaDistY;
        planePosY += stepY;
        rayFlag = DDA_Algorithm::RF_CLEAR;
    }
    // Check if hit tile is not exceeding the maximum tile distance
    int deltaPosX = planePosX - this->start.x;
    int deltaPosY = planePosY - this->start.y;
    if(deltaPosX * deltaPosX + deltaPosY * deltaPosY > this->maxTileDistSquared) {
        rayFlag = DDA_Algorithm::RF_TOO_FAR;
        return RayHitInfo();
    }
    // Check if hit tile is not outside the plane
    if(!this->plane->inBounds(planePosX, planePosY)) {
        rayFlag = DDA_Algorithm::RF_OUTSIDE;
        return RayHitInfo();
    }
    // If tile data is not zero, then ray hit this tile
    int tileData = this->plane->getTile(planePosX, planePosY);
    if(tileData != 0) {
        float distance = (rayFlag == DDA_Algorithm::RF_SIDE) ? (sideDistX - deltaDistX) : (sideDistY - deltaDistY);
        rayFlag |= DDA_Algorithm::RF_HIT;
        return RayHitInfo(
            distance,
            vec2i(planePosX, planePosY),
            start + direction * distance,
            rayFlag & DDA_Algorithm::RF_SIDE
        );
    }
    // This should not trigger but it exists for safety reasons
    rayFlag = DDA_Algorithm::RF_CLEAR;
    return RayHitInfo();
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
