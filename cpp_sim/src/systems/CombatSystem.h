#pragma once
#include <algorithm>
#include <memory>
#include "systems/ISystem.h"
#include "ecs/World.h"
#include "components/Health.h"
#include "components/Weapon.h"
#include "components/ShootIntent.h"
#include "utils/EventLogger.h"

class CombatSystem final : public ISystem {
public:
    explicit CombatSystem(std::shared_ptr<EventLogger> logger)
        : logger_(std::move(logger)) {}

    void update(World& world, float /*dt*/) override {
        // view() returns a value-copy of IDs, so removing ShootIntent mid-loop is safe.
        for (const EntityID shooterID : world.view<ShootIntent, Weapon>()) {
            const EntityID targetID = world.getComponent<ShootIntent>(shooterID).targetID;
            auto&          weapon   = world.getComponent<Weapon>(shooterID);

            weapon.totalShots++;
            weapon.cooldownTimer = weapon.cooldown;  // reset regardless of hit

            if (world.hasComponent<Health>(targetID)) {
                auto& health = world.getComponent<Health>(targetID);
                if (!health.isDead()) {
                    const int dmg = std::max(0, weapon.damage - health.armor);
                    health.hp -= dmg;
                    weapon.totalHits++;

                    logger_->log("damage_dealt", shooterID, {
                        {"target", targetID},
                        {"damage", dmg}
                    });

                    if (health.isDead()) {
                        logger_->log("tank_destroyed", targetID, {
                            {"killed_by", shooterID}
                        });
                    }
                }
            }

            world.removeComponent<ShootIntent>(shooterID);
        }
    }

private:
    std::shared_ptr<EventLogger> logger_;
};
