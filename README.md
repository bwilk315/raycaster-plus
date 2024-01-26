
# Raycaster Plus Game Engine

Simple pseudo-3D game engine built using SDL2 - its core functionality is like other *post-tutorial* raycasters', but heavily extended so making games is possible and not complicated at all.

Frames are software-rendered, which means that only CPU is used for calculations. From my experience CPU utilization peek is when it needs to draw every pixel of the window screen, but with so much control over events in RPGE (like when to render and clear screen) you can optimize your game in a way that will match well with gameplay. GPU-accelerated
rendering is planned to be implemented somewhere in the future.

To learn more check the *docs* directory in the root of project tree.

NOTICE: Documentation is not ready yet and may be changed
