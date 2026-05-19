#pragma once
#include "ecs/Entity.h"

struct TargetComponent {
    EntityID targetID  = 0;
    bool     hasTarget = false;
};
