
# RPGE Scene file

Scene is stored in a file (usually in format _*.rps_) with content that meets certain rules, obviously you can actually do some things the other way however the flow listed below is the cleanest one (in my opinion):

1. Initialize map\
    a. Tell its dimensions in tiles: ``s [w] [h]``\
    b. Fill the map with tile IDs:
    ```
    w <x0_y[h-1]> <x1_y[h-1]> ... <x[w-1]_y[h-1]>
      ...        ...        ... ...
    w <x0_y1>     <x1_y1>     ... <x[w-1]_y1>
    w <x0_y0>     <x1_y0>     ... <x[w-1]_y0>
    ```

2. Define tile walls (read SCENE.md for detailed description)\
    Every step is done on the same line, so just append next snippets. Remember that one ID can define lots of walls.\
    a. Find target tile ID ``t <id>``\
    b. Specify wall geometry with linear equation ``l <slope> <height>``\
    c. Provide visible part ranges (display domain) along three space axes: ``d <start_x> <end_x> <start_y> <end_y> <start_h> <end_h>``\
    d. Tell whether a ray can pass through the columns on which the wall is drawn: ``r <should_be_stopped>`` (0 = false, 1 = true)\
    e. Set the default color used when no texture is assigned: ``c <r> <g> <b> <a>``\
    f. Specify path to texture graphic file relative to program executable: ``x "<path_to_texture>"``, leave only double quotes to use solid color instead.

3. Document your work\
    To create a single line comment, put ``# <your_comment>`` on a separate line.

Consider this simple example scene, it consists of red and blue arcs with many green towers around it:
```

# Create 21 by 21 tiles world

s 21 21
w 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
w 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
w 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
w 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
w 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
w 00 00 00 00 00 00 00 00 00 00 11 00 00 00 00 00 00 00 00 00 00
w 00 00 00 00 00 00 11 00 00 00 00 00 00 00 11 00 00 00 00 00 00
w 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
w 00 00 00 00 00 00 00 00 00 01 02 01 03 00 00 00 00 00 00 00 00
w 00 00 00 00 00 00 00 00 09 00 00 00 04 00 00 00 00 00 00 00 00
w 00 00 00 00 00 11 00 00 10 00 11 00 05 00 00 11 00 00 00 00 00
w 00 00 00 00 00 00 00 00 09 00 00 00 04 00 00 00 00 00 00 00 00
w 00 00 00 00 00 00 00 00 06 07 08 07 00 00 00 00 00 00 00 00 00
w 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
w 00 00 00 00 00 00 11 00 00 00 00 00 00 00 11 00 00 00 00 00 00
w 00 00 00 00 00 00 00 00 00 00 11 00 00 00 00 00 00 00 00 00 00
w 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
w 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
w 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
w 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
w 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00

# Define the tiles

# Red arc
t 01 l  0    1     d 0 1 0 1 -1 2 r 0 c 192 000 000 255 x ""
t 02 l  0    1     d 0 1 0 1 -1 0 r 0 c 192 000 000 255 x ""
t 02 l  0    1     d 0 1 0 1  1 2 r 0 c 192 000 000 255 x ""
t 03 l -1    1     d 0 1 0 1 -1 2 r 0 c 192 000 000 255 x ""
t 04 l 10000 -9999 d 0 1 0 1 -1 2 r 0 c 192 000 000 255 x ""
t 05 l 10000 -9999 d 0 1 0 1 -1 0 r 0 c 192 000 000 255 x ""
t 05 l 10000 -9999 d 0 1 0 1  1 2 r 0 c 192 000 000 255 x ""

# Blue arc
t 06 l -1    1     d 0 1 0 1 -1 2 r 0 c 000 000 192 255 x ""
t 07 l 0     0     d 0 1 0 1 -1 2 r 0 c 000 000 192 255 x ""
t 08 l 0     0     d 0 1 0 1 -1 0 r 0 c 000 000 192 255 x ""
t 08 l 0     0     d 0 1 0 1  1 2 r 0 c 000 000 192 255 x ""
t 09 l 10000 0     d 0 1 0 1 -1 2 r 0 c 000 000 192 255 x ""
t 10 l 10000 0     d 0 1 0 1 -1 0 r 0 c 000 000 192 255 x ""
t 10 l 10000 0     d 0 1 0 1  1 2 r 0 c 000 000 192 255 x ""

# Green tower
t 11 l 0     0     d 0 1 0 1  -1 4 r 0 c 000 192 000 255 x ""
t 11 l 0     1     d 0 1 0 1  -1 4 r 0 c 000 192 000 255 x ""
t 11 l 10000 0     d 0 1 0 1  -1 4 r 0 c 000 192 000 255 x ""
t 11 l 10000 -9999 d 0 1 0 1  -1 4 r 0 c 000 192 000 255 x ""

```

You can plug some textures to it and see how it looks!\
PS. Do not accidentaly move out of the scene bounds - it results in vision loss.
