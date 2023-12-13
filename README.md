
# Raycaster Plus
## General description
As the name suggests, it is raycasting-based game engine enchanced using non-standard additional features. With help of SDL2 library and amazing article on topic of raycasting written by Lode Vandevenne I programmed the engine core, which only allows boxes to be rendered.
After documenting the core mechanics (to not get lost obviously), I proceeded to extend its capabilities. Stable extensions of RP game engine core are listed below:
- Tiles penetration (ray is able to pass through tiles)
- Line-based wall geometry (see *World file* section below)

## Instructions
At this moment there is no API to interact with the engine, in spite of that it is still customizable.
Personalize your experience by defining your own settings in *Raycaster Configuration* section of the main file.
Each tile is assigned data in form of a single integer, the switch statement at line 173 determines which data define what color, feel free to edit this statement.
Create your own world by editing the *world.plane* file, you could use other name but it would require to change *Set up* section a bit.

## World file
Worlds can be programmed, loaded from file or sometimes both.
Structure of such file is as follows:
- Data is separated by spaces (use them only when needed)
- First line tells the world size: ``w h``
- Next ``h`` lines contain ``w`` integers separated by spaces defining world tiles data
- Next lines consist of three separated integers: ``t a b``, where first specifies some tile data, and two other ones a line equation ``ax + b`` with respect to tile local bottom-left corner, describing top-down wall look of every tile with data ``t`` (tile is a square with side length of 1)

Example world, I named it *Big Fan* because it kinda looks like a fan, remember that comments are not yet supported, therefore you must remove them if you want to load this:
```python
9 11  # World size is 11 rows, each has 9 columns
1 1 1 1 1 1 1 1 1
1 0 0 0 0 0 0 0 1
1 0 0 0 0 0 0 0 1
1 0 2 0 0 0 4 0 1
1 0 0 7 2 3 0 0 1
1 0 0 4 1 4 0 0 1
1 0 0 6 2 5 0 0 1
1 0 4 0 0 0 2 0 1
1 0 0 0 0 0 0 0 1
1 0 0 0 0 0 0 0 1
1 1 1 1 1 1 1 1 1 
2 1 0        # All twos' wall geometry is now a line y = 1*x+0 = x
3 1000 -1    # Way of achieving a vertical line to the left
4 -1 1       # Literally the twos' wall but rotated 90 degrees
5 0 0.999    # Trick for horizontal wall to the top
6 1000 -999  # Another trick, for vertical wall to the right
7 0 0        # Just a horizontal wall to the bottom
```
If you want to specify where player camera should be spawned initially, edit the *Set up* section.
Notice that I intentionally offseted some lines by a small amount to not exceed or even reach tile boundaries (otherwise wall would not be shown) which are like below:
- x is between 0 and 1 exclusive
- y is the same

For now there is no automation for the process of writing a world, you need to do it by hand but do not worry! you will familarize yourself with linear equations by playing with them.
