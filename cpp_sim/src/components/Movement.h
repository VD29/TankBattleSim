#pragma once

struct Movement {
    float vx       = 0.0f;  // current velocity x
    float vy       = 0.0f;  // current velocity y
    float speed    = 0.0f;  // max speed (units/second)
    float turnRate = 0.0f;  // max rotation (radians/second)
};
