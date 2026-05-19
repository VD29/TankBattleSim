#include <gtest/gtest.h>
#include "components/Transform.h"
#include "components/Health.h"
#include "components/Movement.h"
#include "components/Weapon.h"
#include "components/Team.h"

// ── Transform ─────────────────────────────────────────────────────────────────

TEST(Transform, DefaultInit) {
    Transform t;
    EXPECT_FLOAT_EQ(t.x,     0.0f);
    EXPECT_FLOAT_EQ(t.y,     0.0f);
    EXPECT_FLOAT_EQ(t.angle, 0.0f);
    EXPECT_FLOAT_EQ(t.prevX, 0.0f);
    EXPECT_FLOAT_EQ(t.prevY, 0.0f);
}

TEST(Transform, AggregateInit) {
    Transform t{10.0f, 20.0f, 1.57f, 9.0f, 19.0f};
    EXPECT_FLOAT_EQ(t.x,     10.0f);
    EXPECT_FLOAT_EQ(t.y,     20.0f);
    EXPECT_FLOAT_EQ(t.angle,  1.57f);
    EXPECT_FLOAT_EQ(t.prevX,  9.0f);
    EXPECT_FLOAT_EQ(t.prevY, 19.0f);
}

// ── Health ────────────────────────────────────────────────────────────────────

TEST(Health, DefaultInit) {
    Health h;
    EXPECT_EQ(h.hp,    100);
    EXPECT_EQ(h.maxHp, 100);
    EXPECT_EQ(h.armor,   0);
    EXPECT_FALSE(h.isDead());
}

TEST(Health, IsDeadWhenHpZero) {
    Health h{0, 100, 0};
    EXPECT_TRUE(h.isDead());
}

TEST(Health, IsDeadWhenHpNegative) {
    Health h{-10, 100, 0};
    EXPECT_TRUE(h.isDead());
}

TEST(Health, IsAliveWhenHpPositive) {
    Health h{1, 100, 0};
    EXPECT_FALSE(h.isDead());
}

TEST(Health, ArmorField) {
    Health h{80, 100, 15};
    EXPECT_EQ(h.armor, 15);
}

// ── Movement ──────────────────────────────────────────────────────────────────

TEST(Movement, DefaultInit) {
    Movement m;
    EXPECT_FLOAT_EQ(m.vx,       0.0f);
    EXPECT_FLOAT_EQ(m.vy,       0.0f);
    EXPECT_FLOAT_EQ(m.speed,    0.0f);
    EXPECT_FLOAT_EQ(m.turnRate, 0.0f);
}

TEST(Movement, AggregateInit) {
    Movement m{3.0f, -1.5f, 10.0f, 0.5f};
    EXPECT_FLOAT_EQ(m.vx,        3.0f);
    EXPECT_FLOAT_EQ(m.vy,       -1.5f);
    EXPECT_FLOAT_EQ(m.speed,    10.0f);
    EXPECT_FLOAT_EQ(m.turnRate,  0.5f);
}

// ── Weapon ────────────────────────────────────────────────────────────────────

TEST(Weapon, DefaultInit) {
    Weapon w;
    EXPECT_EQ(w.damage,           0);
    EXPECT_FLOAT_EQ(w.range,      0.0f);
    EXPECT_FLOAT_EQ(w.cooldown,   0.0f);
    EXPECT_FLOAT_EQ(w.cooldownTimer, 0.0f);
    EXPECT_EQ(w.totalShots,       0);
    EXPECT_EQ(w.totalHits,        0);
}

TEST(Weapon, AggregateInit) {
    Weapon w{25, 150.0f, 2.0f, 0.0f, 10, 7};
    EXPECT_EQ(w.damage,              25);
    EXPECT_FLOAT_EQ(w.range,        150.0f);
    EXPECT_FLOAT_EQ(w.cooldown,       2.0f);
    EXPECT_FLOAT_EQ(w.cooldownTimer,  0.0f);
    EXPECT_EQ(w.totalShots,          10);
    EXPECT_EQ(w.totalHits,            7);
}

TEST(Weapon, CooldownTimerCountsToReady) {
    Weapon w{10, 50.0f, 1.0f, 0.5f, 0, 0};
    EXPECT_GT(w.cooldownTimer, 0.0f); // not ready yet
    w.cooldownTimer -= 0.5f;
    EXPECT_LE(w.cooldownTimer, 0.0f); // ready to fire
}

// ── Team ──────────────────────────────────────────────────────────────────────

TEST(TeamComponent, DefaultIsTeamA) {
    TeamComponent tc;
    EXPECT_EQ(tc.team, Team::TEAM_A);
}

TEST(TeamComponent, CanBeSetToTeamB) {
    TeamComponent tc{Team::TEAM_B};
    EXPECT_EQ(tc.team, Team::TEAM_B);
}

TEST(TeamComponent, TeamsAreDistinct) {
    EXPECT_NE(Team::TEAM_A, Team::TEAM_B);
}
