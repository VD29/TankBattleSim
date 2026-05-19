#pragma once
#include <cmath>
#include <limits>
#include "systems/ISystem.h"
#include "ecs/World.h"
#include "components/Transform.h"
#include "components/Health.h"
#include "components/Movement.h"
#include "components/Weapon.h"
#include "components/Team.h"
#include "components/Target.h"
#include "components/ShootIntent.h"

class AISystem final : public ISystem {
public:
    void update(World& world, float dt) override {
        // Snapshot all potential targets once to avoid re-querying per shooter.
        const auto candidates = world.view<Transform, TeamComponent, Health>();

        for (const EntityID id : world.view<Transform, Weapon, TeamComponent>()) {
            if (world.hasComponent<Health>(id) &&
                world.getComponent<Health>(id).isDead()) {
                continue;
            }

            auto& tf     = world.getComponent<Transform>(id);
            auto& weapon = world.getComponent<Weapon>(id);
            auto& team   = world.getComponent<TeamComponent>(id);

            // ── Find nearest alive enemy ───────────────────────────────────
            EntityID nearestID     = 0;
            float    nearestDistSq = std::numeric_limits<float>::max();
            bool     found         = false;

            for (const EntityID cid : candidates) {
                if (cid == id) continue;
                if (world.getComponent<TeamComponent>(cid).team == team.team) continue;
                if (world.getComponent<Health>(cid).isDead()) continue;

                const auto& ct = world.getComponent<Transform>(cid);
                const float dx = ct.x - tf.x;
                const float dy = ct.y - tf.y;
                const float dSq = dx * dx + dy * dy;
                if (dSq < nearestDistSq) {
                    nearestDistSq = dSq;
                    nearestID     = cid;
                    found         = true;
                }
            }

            if (!found) continue;

            world.addComponent<TargetComponent>(id, {nearestID, true});

            // ── Steer toward enemy ─────────────────────────────────────────
            const auto& et   = world.getComponent<Transform>(nearestID);
            const float dx   = et.x - tf.x;
            const float dy   = et.y - tf.y;
            const float dist = std::sqrt(nearestDistSq);

            if (world.hasComponent<Movement>(id) && dist > 0.0f) {
                auto& mov   = world.getComponent<Movement>(id);
                mov.vx = (dx / dist) * mov.speed;
                mov.vy = (dy / dist) * mov.speed;
            }

            // ── Tick cooldown & mark intent to shoot ───────────────────────
            weapon.cooldownTimer -= dt;

            if (dist <= weapon.range && weapon.cooldownTimer <= 0.0f) {
                world.addComponent<ShootIntent>(id, {nearestID});
            }
        }
    }
};
