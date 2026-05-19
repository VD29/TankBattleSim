#pragma once
#include <algorithm>
#include "systems/ISystem.h"
#include "ecs/World.h"
#include "components/Transform.h"
#include "components/Movement.h"

class MovementSystem final : public ISystem {
public:
    void update(World& world, float dt) override {
        for (const EntityID id : world.view<Transform, Movement>()) {
            auto& t = world.getComponent<Transform>(id);
            auto& m = world.getComponent<Movement>(id);

            // Snapshot position so the visualizer can interpolate between frames
            t.prevX = t.x;
            t.prevY = t.y;

            t.x += m.vx * dt;
            t.y += m.vy * dt;

            t.x = std::clamp(t.x, 0.0f, kWidth);
            t.y = std::clamp(t.y, 0.0f, kHeight);
        }
    }

private:
    static constexpr float kWidth  = 800.0f;
    static constexpr float kHeight = 600.0f;
};
