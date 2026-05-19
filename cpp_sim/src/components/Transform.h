#pragma once

struct Transform {
    float x     = 0.0f;
    float y     = 0.0f;
    float angle = 0.0f;  // radians
    float prevX = 0.0f;  // snapshot from previous tick for visualizer interpolation
    float prevY = 0.0f;
};
