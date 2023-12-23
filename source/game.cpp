
#include "../include/game.hpp"

/**********************************************/
/********** STRUCTURE: LINE EQUATION **********/
/**********************************************/

LineEquation::LineEquation() {
    this->slope = 0;
    this->intercept = 0;
}
LineEquation::LineEquation(float slope, float intercept, float domainStart, float domainEnd) {
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

/******************************************/
/********** STRUCTURE: WALL INFO **********/
/******************************************/

WallInfo::WallInfo() {
    this->line = LineEquation();
    this->color = { 0xFF, 0xFF, 0xFF };
}
WallInfo::WallInfo(LineEquation line, SDL_Color color) {
    this->line = line;
    this->color = color;
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
    this->walls = std::map<int, std::vector<WallInfo>>();
}
Plane::Plane(const std::string& file) {
    this->walls = std::map<int, std::vector<WallInfo>>();
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
int Plane::getTileData(int x, int y) const {
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
int Plane::setTileData(int x, int y, int tileData) {
    if(this->inBounds(x, y)) {
        this->tiles[this->posToDataIndex(x, y)] = tileData;
        return Plane::E_G_CLEAR;
    }
    return Plane::E_G_TILE_NOT_FOUND;
}
void Plane::setTileWall(int tileData, int wallIndex, LineEquation line, SDL_Color color) {
    // Ensure wall for specified tile exists
    if(this->walls.count(tileData) == 0)
        this->walls.insert(std::pair<int, std::vector<WallInfo>>(
            tileData,
            std::vector<WallInfo>()
        ));
    // Additionaly ensure that specified wall exists, if vector is empty is has no size
    if(this->walls.at(tileData).empty() || wallIndex < 0 || wallIndex > this->walls.at(tileData).size() - 1) {
        wallIndex = this->walls.at(tileData).size();
        this->walls.at(tileData).push_back(WallInfo());
    }
    // Finally set the well-ensured target
    WallInfo* ptr = &this->walls.at(tileData).at(wallIndex);
    ptr->line = line;
    ptr->color = color;
}

int_pair Plane::load(const std::string& file) {
    std::ifstream stream(file);
    int ln = 0;
    if(!stream.good()) {
        return int_pair(ln, Plane::E_PF_FAILED_TO_LOAD);
    }

    std::string fileLine;
    int wdh = -1; // World data height (starting from top)
    while(std::getline(stream, fileLine)) {
        ln++;

        // Extract space-separated arguments
        std::vector<std::string> args;
        std::string current = "";
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
                    return int_pair(ln, Plane::E_PF_INVALID_ARGUMENTS_COUNT);
                else if(!isFloat(args.at(1)) || !isFloat(args.at(2)))
                    return int_pair(ln, Plane::E_PF_UNKNOWN_NUMBER_FORMAT);
                this->width = (int)std::stof(args.at(1));
                this->height = (int)std::stof(args.at(2));
                wdh = this->height - 1;
                this->tiles = new int[this->width * this->height];
                break;
            // Define next world data height (counting from top)
            case 'w':
                if(wdh == -1)
                    return int_pair(ln, Plane::E_PF_OPERATION_NOT_AVAILABLE);
                if(args.size() != this->width + 1)
                    return int_pair(ln, Plane::E_PF_INVALID_ARGUMENTS_COUNT);
                for(int x = 0; x < this->width; x++) {
                    if(!isFloat(args.at(1 + x)))
                        return int_pair(ln, Plane::E_PF_UNKNOWN_NUMBER_FORMAT);
                    this->setTileData(x, wdh, (int)std::stof(args.at(1 + x)));
                }
                wdh--;
                break;
            // Define properties of a tile with specified data
            case 't':
                if(args.size() != 12)
                    return int_pair(ln, Plane::E_PF_INVALID_ARGUMENTS_COUNT);
                else if(!(
                    isFloat(args.at(1))  && isFloat(args.at(3))  && isFloat(args.at(4))  &&
                    isFloat(args.at(6))  && isFloat(args.at(7))  && isFloat(args.at(9))  &&
                    isFloat(args.at(10)) && isFloat(args.at(11))
                )) return int_pair(ln, Plane::E_PF_UNKNOWN_NUMBER_FORMAT);

                this->setTileWall(
                    (int)std::stof(args.at(1)),
                    -1, // Enforce new wall entry creation
                    LineEquation(
                        std::stof(args.at(3)),
                        std::stof(args.at(4)),
                        std::stof(args.at(6)),
                        std::stof(args.at(7))
                    ),
                    {
                        (Uint8)std::stof(args.at(9)),
                        (Uint8)std::stof(args.at(10)),
                        (Uint8)std::stof(args.at(11))
                    }
                );
                break;
            default:
                return int_pair(ln, Plane::E_PF_OPERATION_NOT_AVAILABLE);
                break;
        }
    }

    //std::cout << this->width << ", " << this->height << std::endl;
    // for(int a = this->width - 1; a != -1; a--) {
    //     for(int b = 0; b < this->height; b++)
    //         std::cout << this->getTileData(b, a) << " ";
    //     std::cout << "\n";
    // }

    stream.close();
    return int_pair(ln, Plane::E_PF_CLEAR);
}
// int_pair Plane::load(const std::string& file) {
//     std::ifstream stream(file);
//     int line = 1;
//     if(!stream.good()) {
//         return int_pair(line, Plane::E_PF_FAILED_TO_LOAD);
//     }

//     const std::string SEMICOLON = ";";
//     std::string bf0, bf1, bf2, bf3; // Buffers for taking input
//     // Read world dimensions
//     stream >> bf0; // Width
//     stream >> bf1; // Height
//     stream >> bf2; // Semicolon

//     if(bf2 != SEMICOLON)
//         return int_pair(line, Plane::E_PF_MISSING_SEMICOLON);
//     if(!isFloat(bf0) || !isFloat(bf1))
//         return int_pair(line, Plane::E_PF_INVALID_DIMENSIONS);
//     this->width = (int)std::stof(bf0);
//     this->height = (int)std::stof(bf1);
//     this->tiles = new int[this->width * this->height];

//     // Read world tiles data
//     for(int y = this->height - 1; y != -1; y--) {
//         line++;
//         for(int x = 0; x != this->width; x++) {
//             stream >> bf0;
//             if(!isFloat(bf0))
//                 return int_pair(line, Plane::E_PF_INVALID_TILE_DATA);
//             this->setTile(x, y, (int)std::stof(bf0));
//         }
//         stream >> bf0;
//         if(bf0 != SEMICOLON)
//             return int_pair(line, Plane::E_PF_MISSING_SEMICOLON);
//     }

//     // Read line equations (aka wall geometries)
//     while(true) {
//         line++;
//         stream >> bf0; // Tile (aka wall) id
//         if(bf0 == SEMICOLON) // Two semicolons in a row indicate termination
//             break;
//         stream >> bf1; // Line identification number
//         stream >> bf2; // Line slope
//         stream >> bf3; // Line intercept
        
//         // Load tileId, lineId, slope and intercept
//         if(!isFloat(bf0) || !isFloat(bf1) || !isFloat(bf2) || !isFloat(bf3)) 
//             return int_pair(line, Plane::E_PF_INVALID_LINE_DATA);
//         int tileId = (int)std::stof(bf0);
//         int lineId = (int)std::stof(bf1);
//         float slope = std::stof(bf2);
//         float intercept = std::stof(bf3);

//         // Load domain start and end values
//         stream >> bf0;
//         stream >> bf1;
//         if(!isFloat(bf0) || !isFloat(bf1))
//             return int_pair(line, Plane::E_PF_INVALID_LINE_DATA);
//         float domainStart = std::stof(bf0);
//         float domainEnd = std::stof(bf1);

//         // Create vector entry if it does not exist
//         if(this->geometry.count(tileId) == 0)
//             this->geometry.insert(std::pair<int, std::vector<LineEquation>>(
//                 tileId, std::vector<LineEquation>()
//             ));
//         // Add a new line equation for a specified tile id
//         this->geometry.at(tileId).push_back(
//             LineEquation(lineId, slope, intercept, domainStart, domainEnd)
//         );
//         // Ensure semicolon at the end
//         stream >> bf0;
//         if(bf0 != SEMICOLON)
//             return int_pair(line, Plane::E_PF_MISSING_SEMICOLON);
//     }

//     // Read line colors (very similar to the previous one above)
//     while(true) {
//         line++;
//         stream >> bf0; // Line id
//         stream >> bf1; // Red channel
//         stream >> bf2; // Green channel
//         stream >> bf3; // Blue channel

//         if(bf0 == SEMICOLON)
//             break;
//         else if(!isFloat(bf0) || !isFloat(bf1) || !isFloat(bf2) || !isFloat(bf3))
//             return int_pair(line, Plane::E_PF_INVALID_COLOR_DATA);
//         int lineId = (int)std::stof(bf0);
//         this->lineColors.insert(std::pair<int, SDL_Color>(
//             lineId, { (Uint8)std::stof(bf1), (Uint8)std::stof(bf2), (Uint8)std::stof(bf3), 255 }
//         ));
//         stream >> bf0;
//         if(bf0 != SEMICOLON)
//             return int_pair(line, Plane::E_PF_MISSING_SEMICOLON);
//     }

//     stream.close();
//     return int_pair(line, Plane::E_PF_CLEAR);
// }
std::vector<WallInfo> Plane::getTileWalls(int tileData) const {
    if(this->walls.count(tileData) == 0)
        return std::vector<WallInfo>();
    return this->walls.at(tileData);
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
    int tileData = this->plane->getTileData(planePosX, planePosY);
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
