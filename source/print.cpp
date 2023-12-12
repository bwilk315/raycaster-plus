
#include "../include/print.hpp"

void printRepeated(const char* str, int count) {
    while(count--) std::cout << str;
}
void printVector(const vec2i& vec) {
    std::cout << "[ " << vec.x << " " << vec.y << " ]\n";
}
void printVector(const vec2f& vec) {
    std::cout << "[ " << vec.x << " " << vec.y << " ]\n";
}
void printPlane(const Plane& plane, bool ticks, int nullId) {
    int maxDataSpaces = length(plane.maxData());
    int maxTickSpaces = std::max(length(plane.getWidth()), length(plane.getHeight()));
    // To print array-like form of the plane data using its implementation of
    // coordinates (read the Plane class description), Y needs to be inverted.
    for(int y = plane.getHeight() - 1; y != -1; y--) {
        // Draw vertical ticks.
        if(ticks) {
            std::cout << y;
            printRepeated(" ", maxTickSpaces - length(y) + 1);
        }
        for(int x = 0; x != plane.getWidth(); x++) {
            int data = plane.getData(x, y);
            // Complete the tile numeric value with zeroes to make it look
            // more tiley in comparison to others.
            int currSpaces = (maxDataSpaces + maxTickSpaces - 1) - length(data);
            if(data == nullId) {
                printRepeated(" ", currSpaces);
                std::cout << "  ";
            } else {
                printRepeated("0", currSpaces);
                std::cout << data << ' ';
            }
        }
        std::cout << "\n";
    }
    // Draw horizontal ticks.
    if(ticks) {
        printRepeated("*", maxTickSpaces);
        std::cout << ' ';
        for(int x = 0; x != plane.getWidth(); x++) {
            printRepeated(" ", maxTickSpaces - length(x));
            std::cout << x << ' ';
        }
        std::cout << std::endl;
    }
}
