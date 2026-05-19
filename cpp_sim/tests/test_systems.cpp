#include <cmath>
#include <fstream>
#include <string>
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include "ecs/World.h"
#include "components/Transform.h"
#include "components/Health.h"
#include "components/Movement.h"
#include "components/Weapon.h"
#include "components/Team.h"
#include "components/Target.h"
#include "components/ShootIntent.h"
#include "systems/MovementSystem.h"
#include "systems/AISystem.h"
#include "systems/CombatSystem.h"
#include "utils/EventLogger.h"

// ── Helpers ───────────────────────────────────────────────────────────────────

static EntityID makeTank(World& world,
                          float x, float y,
                          Team t,
                          int hp       = 100,
                          float speed  = 50.0f,
                          int damage   = 20,
                          float range  = 200.0f,
                          float cd     = 1.0f) {
    const auto id = world.createEntity();
    world.addComponent<Transform>(id, {x, y, 0.0f, x, y});
    world.addComponent<Movement>(id, {0.0f, 0.0f, speed, 1.0f});
    world.addComponent<Health>(id,   {hp, hp, 0});
    world.addComponent<Weapon>(id,   {damage, range, cd, 0.0f, 0, 0});
    world.addComponent<TeamComponent>(id, {t});
    return id;
}

// ── MovementSystem ────────────────────────────────────────────────────────────

TEST(MovementSystem, MovesEntityByVelocityTimesDt) {
    World world;
    const auto id = world.createEntity();
    world.addComponent<Transform>(id, {100.0f, 200.0f, 0.0f, 0.0f, 0.0f});
    world.addComponent<Movement>(id,  {10.0f, 5.0f, 50.0f, 1.0f});

    MovementSystem sys;
    sys.update(world, 1.0f);

    const auto& t = world.getComponent<Transform>(id);
    EXPECT_FLOAT_EQ(t.x, 110.0f);
    EXPECT_FLOAT_EQ(t.y, 205.0f);
}

TEST(MovementSystem, StoresPrevPositionBeforeMove) {
    World world;
    const auto id = world.createEntity();
    world.addComponent<Transform>(id, {50.0f, 60.0f, 0.0f, 0.0f, 0.0f});
    world.addComponent<Movement>(id,  {10.0f, 0.0f, 50.0f, 1.0f});

    MovementSystem sys;
    sys.update(world, 1.0f);

    const auto& t = world.getComponent<Transform>(id);
    EXPECT_FLOAT_EQ(t.prevX, 50.0f);
    EXPECT_FLOAT_EQ(t.prevY, 60.0f);
}

TEST(MovementSystem, ClampsPositionAtMaxBounds) {
    World world;
    const auto id = world.createEntity();
    world.addComponent<Transform>(id, {795.0f, 595.0f, 0.0f, 0.0f, 0.0f});
    world.addComponent<Movement>(id,  {100.0f, 100.0f, 200.0f, 1.0f});

    MovementSystem sys;
    sys.update(world, 1.0f);

    const auto& t = world.getComponent<Transform>(id);
    EXPECT_FLOAT_EQ(t.x, 800.0f);
    EXPECT_FLOAT_EQ(t.y, 600.0f);
}

TEST(MovementSystem, ClampsPositionAtMinBounds) {
    World world;
    const auto id = world.createEntity();
    world.addComponent<Transform>(id, {5.0f, 5.0f, 0.0f, 0.0f, 0.0f});
    world.addComponent<Movement>(id,  {-100.0f, -100.0f, 200.0f, 1.0f});

    MovementSystem sys;
    sys.update(world, 1.0f);

    const auto& t = world.getComponent<Transform>(id);
    EXPECT_FLOAT_EQ(t.x, 0.0f);
    EXPECT_FLOAT_EQ(t.y, 0.0f);
}

TEST(MovementSystem, IgnoresEntitiesWithoutMovementComponent) {
    World world;
    const auto id = world.createEntity();
    world.addComponent<Transform>(id, {10.0f, 10.0f, 0.0f, 0.0f, 0.0f});
    // No Movement component added

    MovementSystem sys;
    EXPECT_NO_THROW(sys.update(world, 1.0f));

    const auto& t = world.getComponent<Transform>(id);
    EXPECT_FLOAT_EQ(t.x, 10.0f); // unchanged
}

// ── AISystem ──────────────────────────────────────────────────────────────────

TEST(AISystem, SteersTowardNearestEnemy) {
    World world;
    const auto ally  = makeTank(world, 0.0f,   0.0f, Team::TEAM_A, 100, 50.0f, 20, 50.0f, 1.0f);
    const auto enemy = makeTank(world, 100.0f,  0.0f, Team::TEAM_B);

    AISystem sys;
    sys.update(world, 0.016f);

    const auto& m = world.getComponent<Movement>(ally);
    EXPECT_GT(m.vx, 0.0f);          // moving in positive x toward enemy
    EXPECT_FLOAT_EQ(m.vy, 0.0f);    // no y movement needed
    (void)enemy;
}

TEST(AISystem, SetsTargetComponent) {
    World world;
    const auto ally  = makeTank(world, 0.0f, 0.0f, Team::TEAM_A);
    const auto enemy = makeTank(world, 50.0f, 0.0f, Team::TEAM_B);

    AISystem sys;
    sys.update(world, 0.016f);

    ASSERT_TRUE(world.hasComponent<TargetComponent>(ally));
    const auto& tc = world.getComponent<TargetComponent>(ally);
    EXPECT_TRUE(tc.hasTarget);
    EXPECT_EQ(tc.targetID, enemy);
}

TEST(AISystem, AddsShootIntentWhenInRangeAndCooldownReady) {
    World world;
    // enemy is within range=200, cooldownTimer starts at 0 (ready)
    const auto ally  = makeTank(world, 0.0f,  0.0f, Team::TEAM_A, 100, 50.0f, 20, 200.0f, 1.0f);
    makeTank(world, 50.0f, 0.0f, Team::TEAM_B);

    AISystem sys;
    sys.update(world, 0.016f);

    EXPECT_TRUE(world.hasComponent<ShootIntent>(ally));
}

TEST(AISystem, NoShootIntentWhenOutOfRange) {
    World world;
    // enemy is 500 units away, range = 200
    const auto ally = makeTank(world, 0.0f,   0.0f, Team::TEAM_A, 100, 50.0f, 20, 200.0f, 1.0f);
    makeTank(world, 500.0f, 0.0f, Team::TEAM_B);

    AISystem sys;
    sys.update(world, 0.016f);

    EXPECT_FALSE(world.hasComponent<ShootIntent>(ally));
}

TEST(AISystem, NoShootIntentWhenCooldownNotReady) {
    World world;
    const auto ally  = makeTank(world, 0.0f, 0.0f, Team::TEAM_A, 100, 50.0f, 20, 200.0f, 1.0f);
    // Override weapon: timer not expired
    world.getComponent<Weapon>(ally).cooldownTimer = 5.0f;
    makeTank(world, 50.0f, 0.0f, Team::TEAM_B);

    AISystem sys;
    sys.update(world, 0.016f);

    EXPECT_FALSE(world.hasComponent<ShootIntent>(ally));
}

TEST(AISystem, SkipsDeadEntities) {
    World world;
    const auto ally = makeTank(world, 0.0f, 0.0f, Team::TEAM_A);
    world.getComponent<Health>(ally).hp = 0; // already dead

    makeTank(world, 50.0f, 0.0f, Team::TEAM_B);

    AISystem sys;
    sys.update(world, 0.016f);

    EXPECT_FALSE(world.hasComponent<ShootIntent>(ally));
}

TEST(AISystem, DoesNotTargetSameTeam) {
    World world;
    const auto a1 = makeTank(world, 0.0f,  0.0f, Team::TEAM_A);
    const auto a2 = makeTank(world, 10.0f, 0.0f, Team::TEAM_A); // very close same-team

    AISystem sys;
    sys.update(world, 0.016f);

    // Neither should have a target because there are no enemies
    EXPECT_FALSE(world.hasComponent<TargetComponent>(a1) &&
                 world.getComponent<TargetComponent>(a1).hasTarget);
    EXPECT_FALSE(world.hasComponent<TargetComponent>(a2) &&
                 world.getComponent<TargetComponent>(a2).hasTarget);
}

// ── CombatSystem ──────────────────────────────────────────────────────────────

static std::shared_ptr<EventLogger> makeLogger(const std::string& path = "/tmp/tank_test.jsonl") {
    return std::make_shared<EventLogger>(path);
}

TEST(CombatSystem, AppliesDamageToTarget) {
    World world;
    const auto shooter = world.createEntity();
    const auto target  = world.createEntity();

    world.addComponent<Weapon>(shooter,      {20, 100.0f, 1.0f, 0.0f, 0, 0});
    world.addComponent<ShootIntent>(shooter, {target});
    world.addComponent<Health>(target,       {100, 100, 0});

    CombatSystem sys(makeLogger());
    sys.update(world, 0.016f);

    EXPECT_EQ(world.getComponent<Health>(target).hp, 80);
}

TEST(CombatSystem, ArmorReducesDamage) {
    World world;
    const auto shooter = world.createEntity();
    const auto target  = world.createEntity();

    world.addComponent<Weapon>(shooter,      {20, 100.0f, 1.0f, 0.0f, 0, 0});
    world.addComponent<ShootIntent>(shooter, {target});
    world.addComponent<Health>(target,       {100, 100, 8}); // 8 armor

    CombatSystem sys(makeLogger());
    sys.update(world, 0.016f);

    EXPECT_EQ(world.getComponent<Health>(target).hp, 88); // 20 - 8 = 12 dmg
}

TEST(CombatSystem, DamageCannotGoBelowZero) {
    World world;
    const auto shooter = world.createEntity();
    const auto target  = world.createEntity();

    world.addComponent<Weapon>(shooter,      {5, 100.0f, 1.0f, 0.0f, 0, 0});
    world.addComponent<ShootIntent>(shooter, {target});
    world.addComponent<Health>(target,       {100, 100, 50}); // armor > damage

    CombatSystem sys(makeLogger());
    sys.update(world, 0.016f);

    EXPECT_EQ(world.getComponent<Health>(target).hp, 100); // no damage
}

TEST(CombatSystem, IncrementsShotsAndHits) {
    World world;
    const auto shooter = world.createEntity();
    const auto target  = world.createEntity();

    world.addComponent<Weapon>(shooter,      {10, 100.0f, 1.0f, 0.0f, 0, 0});
    world.addComponent<ShootIntent>(shooter, {target});
    world.addComponent<Health>(target,       {100, 100, 0});

    CombatSystem sys(makeLogger());
    sys.update(world, 0.016f);

    const auto& w = world.getComponent<Weapon>(shooter);
    EXPECT_EQ(w.totalShots, 1);
    EXPECT_EQ(w.totalHits,  1);
}

TEST(CombatSystem, ResetsCooldownTimer) {
    World world;
    const auto shooter = world.createEntity();
    const auto target  = world.createEntity();

    world.addComponent<Weapon>(shooter,      {10, 100.0f, 2.0f, -0.5f, 0, 0});
    world.addComponent<ShootIntent>(shooter, {target});
    world.addComponent<Health>(target,       {100, 100, 0});

    CombatSystem sys(makeLogger());
    sys.update(world, 0.016f);

    EXPECT_FLOAT_EQ(world.getComponent<Weapon>(shooter).cooldownTimer, 2.0f);
}

TEST(CombatSystem, RemovesShootIntentAfterProcessing) {
    World world;
    const auto shooter = world.createEntity();
    const auto target  = world.createEntity();

    world.addComponent<Weapon>(shooter,      {10, 100.0f, 1.0f, 0.0f, 0, 0});
    world.addComponent<ShootIntent>(shooter, {target});
    world.addComponent<Health>(target,       {100, 100, 0});

    CombatSystem sys(makeLogger());
    sys.update(world, 0.016f);

    EXPECT_FALSE(world.hasComponent<ShootIntent>(shooter));
}

TEST(CombatSystem, CountsShotEvenOnMiss) {
    World world;
    const auto shooter = world.createEntity();
    const auto target  = world.createEntity(); // no Health component

    world.addComponent<Weapon>(shooter,      {10, 100.0f, 1.0f, 0.0f, 0, 0});
    world.addComponent<ShootIntent>(shooter, {target});

    CombatSystem sys(makeLogger());
    sys.update(world, 0.016f);

    EXPECT_EQ(world.getComponent<Weapon>(shooter).totalShots, 1);
    EXPECT_EQ(world.getComponent<Weapon>(shooter).totalHits,  0);
}

// ── EventLogger ───────────────────────────────────────────────────────────────

TEST(EventLogger, WritesValidJsonLine) {
    const std::string path = "/tmp/tank_test_logger.jsonl";
    {
        EventLogger logger(path);
        logger.log("tank_moved", 7, {{"x", 10.5}, {"y", 20.0}});
    }
    std::ifstream f(path);
    std::string line;
    ASSERT_TRUE(std::getline(f, line));
    const auto j = nlohmann::json::parse(line);
    EXPECT_EQ(j["type"],   "tank_moved");
    EXPECT_EQ(j["entity"], 7);
    EXPECT_DOUBLE_EQ(j["x"].get<double>(), 10.5);
}

TEST(EventLogger, MultipleLogsAppearAsMultipleLines) {
    const std::string path = "/tmp/tank_test_multiline.jsonl";
    {
        EventLogger logger(path);
        logger.log("damage_dealt",   1, {{"damage", 10}});
        logger.log("tank_destroyed", 2, {{"killed_by", 1}});
    }
    std::ifstream f(path);
    int lines = 0;
    for (std::string l; std::getline(f, l); ) { ++lines; }
    EXPECT_GE(lines, 2);
}

TEST(EventLogger, ThrowsOnBadFilePath) {
    EXPECT_THROW(EventLogger("/nonexistent_dir/log.jsonl"), std::runtime_error);
}
