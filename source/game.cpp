
#include "../include/game.hpp"

LineEquation::LineEquation() {
    this->data = -1;
    this->slope = 0;
    this->intercept = 0;
}
LineEquation::LineEquation(int data, float slope, float intercept) {
    this->data = data;
    this->slope = slope;
    this->intercept = intercept;
}
vec2f LineEquation::intersection(const LineEquation& other) const {
    // given k: y=a0*x+b0 and k: y=a1*x+b1, argument of the lines' crossing is given by
    // formula: x=(b1-b0)/(a0-a1), knowing x it is possible to know the intersection height.
    float x = (this->intercept - other.intercept) / (other.slope - this->slope);
    return vec2f(x, this->slope * x + this->intercept);
}

int Plane::posToDataIndex(int x, int y) const {
    // If data was formed in 2D-array-like shape, bottom-left element would be
    // the origin having position (0, 0).
    return this->width * (this->height - y - 1) + x;
}
Plane::Plane(int width, int height) {
    this->width = width;
    this->height = height;
    this->data = new int[width * height];
    this->lines = std::vector<LineEquation>();
}
Plane::Plane(const std::string& planeFile) {
    std::ifstream file(planeFile);
    if(!file.good()) {
        this->error = Plane::CANNOT_OPEN_WORLD_FILE;
        return;
    }
    
    // THIS NEEDS GENERALIZATION, MAYBE AS INTERPRETER CLASS
    std::string bf0, bf1, bf2;
    // First two numbers are plane dimensions, use them to construct a plane
    file >> bf0;
    file >> bf1;
    if(!isNumber(bf0) || !isNumber(bf1)) {
        this->error = Plane::IFT_READ_DIMENSIONS;
        return;
    }
    this->width = std::stoi(bf0);
    this->height = std::stoi(bf1);
    this->data = new int[this->width * this->height];
    // Collect the actual map data
    for(int y = this->height - 1; y != -1; y--) {
        for(int x = 0; x != this->width; x++) {
            file >> bf0;
            if(!isNumber(bf0) || file.eof()) {
                this->error = Plane::IFT_READ_WORLD;
                return;
            }
            int tileData = std::stoi(bf0);
            if(!this->setData(x, y, tileData)) {
                this->error = Plane::IFT_READ_WORLD;
                return;
            }
        }
    }
    // Collect information about line equations for tiles with specific data
    this->lines = std::vector<LineEquation>();
    while(!file.eof()) {
        int errSum = 0;
        file >> bf0;
        file >> bf1;
        file >> bf2;
        if(errSum || !isNumber(bf0) || !isNumber(bf1) || !isNumber(bf2)) {
            this->error = Plane::IFT_READ_LINE_EQUATIONS;
            return;
        }
        this->lines.push_back({
            std::stoi(bf0), // tile data
            std::stof(bf1), // slope
            std::stof(bf2)  // intercept
        });
    }

    file.close();
}
Plane::~Plane() {
    if(this->data != nullptr) delete[] this->data;
}
void Plane::fillData(int value) {
    for(int i = 0; i < this->width * this->height; i++) {
        this->data[i] = value;
    }
}
int Plane::getData(int x, int y) const {
    if(inBounds(x, y))
        return this->data[this->posToDataIndex(x, y)];
    return -1;
}
int Plane::getError() const {
    return this->error;
}
LineEquation Plane::getLine(int data) const {
    LineEquation out;
    for(const LineEquation& le : this->lines)
        if(le.data == data) {
            out = le;
            break;
        }
    return out;
}
int Plane::getHeight() const {
    return this->height;
}
int Plane::getWidth() const {
    return this->width;
}
bool Plane::inBounds(int x, int y) const {
    return (x > -1 && x < this->width) && (y > -1 && y < this->height);
}
int Plane::maxData() const {
    int max = 0;
    for(int i = 0; i < this->width * this->height; i++) {
        if(this->data[i] > max) max = this->data[i];
    }
    return max;
}
bool Plane::setData(int x, int y, int value) {
    if(this->inBounds(x, y)) {
        this->data[this->posToDataIndex(x, y)] = value;
        return true;
    }
    return false;
}
bool Plane::setLine(int data, float slope, float intercept) {
    for(LineEquation& le : this->lines)
        if(le.data == data) {
            le.slope = slope;
            le.intercept = intercept;
            return true;
        }
    return false;
}


DDA_RayHitInfo::DDA_RayHitInfo() {
    this->data = -1;
    this->distance = -1;
    this->point = vec2f::ZERO;
    this->side = false;
}
DDA_RayHitInfo::DDA_RayHitInfo(int data, float distance, vec2f point, bool side) {
    this->data = data;
    this->distance = distance;
    this->point = point;
    this->side = side;
}

DDA_Algorithm::DDA_Algorithm(const Plane& plane, int maxTileDist) {
    this->plane = &plane;
    this->hits = new DDA_RayHitInfo[maxTileDist];
    this->maxTileDistSquared = maxTileDist * maxTileDist; // Only square is useful
    memset(this->hits, 0, maxTileDist);
}
DDA_Algorithm::~DDA_Algorithm() {
    if(this->hits != NULL) {
        delete[] this->hits;
    }
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
        if(deltaPosX * deltaPosX + deltaPosY * deltaPosY > maxTileDistSquared) {
            break;
        }
        // Process the hit tile data, register it if possible
        if(!this->plane->inBounds(mapPos.x, mapPos.y)) {
            break;
        } else {
            int tileData = this->plane->getData(mapPos.x, mapPos.y);
            if(tileData > 0) {
                // Find the hit point by moving DDA-computed distance along sent ray direction
                float pointDist = side ? sideDistX - deltaDistX : sideDistY - deltaDistY;
                this->hits[hitCount++] = { tileData, pointDist, start + direction * pointDist, side };
            }
        }
    }
    return hitCount;
}


Camera::Camera() {
    // Defaults
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
void Camera::setDirection(float radians) {
    this->direction = vec2f::UP.rotate(radians);
    this->plane = vec2f::RIGHT.rotate(radians) * this->planeMagnitude;
}
void Camera::setPosition(vec2f pos) {
    this->position = pos;
}
