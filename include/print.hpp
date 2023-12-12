
#ifndef _PRINT_HPP
#define _PRINT_HPP

#include <iostream>

void printRepeated(const char* str, int count);

#include "../include/math.hpp"

void printVector(const vec2i& vec);
void printVector(const vec2f& vec);

#include "../include/game.hpp"

/* If <ticks> flag is set, tile index ticks are included.
 * When printing, tiles with data <nullId> are printed as a blank space. */
void printPlane(const Plane& plane, bool ticks = false, int nullId = -1);


#endif
