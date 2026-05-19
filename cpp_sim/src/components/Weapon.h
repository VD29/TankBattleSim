#pragma once

struct Weapon {
    int   damage        = 0;
    float range         = 0.0f;
    float cooldown      = 0.0f;  // seconds between shots
    float cooldownTimer = 0.0f;  // counts down to 0; shot is ready when <= 0
    int   totalShots    = 0;
    int   totalHits     = 0;
};
