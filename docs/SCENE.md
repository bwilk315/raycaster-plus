
# RPGE Scene

Scene is a collection of tiles which form a 2D grid, each tile is assigned ID number which refers to a 1x1 square (in world units) containing specific definitions of walls that are later rendered. Tiles with ID of 0 are considered *air tiles* - they do and hold nothing.

## Coordinates system

Coordinates system can be either *global* (relative to the world origin) or *local* (relative to some tile origin), see illustration below:\
![coordinates system illustration](./images/coords.svg)

As you can see on the left, tiles are organized to lay on integer-coordinates thus creating a 2D grid, while on the right side you can see local surface of the tile where walls are defined.

## Tile walls
Single wall is just a vertically-aligned plane, which properties can be updated at runtime. You can have as many walls per tile as you want, but you should be aware of the fact that too many walls impacts the rendering performance.
