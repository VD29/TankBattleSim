#pragma once
#include "ecs/Entity.h"

// Added by AISystem when a tank is ready to fire; consumed and removed by CombatSystem.
struct ShootIntent {
    EntityID targetID = 0;
};
