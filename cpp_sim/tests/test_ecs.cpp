#include <gtest/gtest.h>
#include "ecs/Entity.h"
#include "ecs/ComponentPool.h"

// ── EntityManager ─────────────────────────────────────────────────────────────

TEST(EntityManager, CreateReturnsUniqueIDs) {
    EntityManager em;
    EXPECT_NE(em.create(), em.create());
}

TEST(EntityManager, IsAliveAfterCreate) {
    EntityManager em;
    const auto id = em.create();
    EXPECT_TRUE(em.isAlive(id));
}

TEST(EntityManager, NotAliveAfterDestroy) {
    EntityManager em;
    const auto id = em.create();
    em.destroy(id);
    EXPECT_FALSE(em.isAlive(id));
}

TEST(EntityManager, ReusesDestroyedID) {
    EntityManager em;
    const auto first = em.create();
    em.destroy(first);
    EXPECT_EQ(em.create(), first);
}

TEST(EntityManager, DestroyUnknownIDIsNoop) {
    EntityManager em;
    EXPECT_NO_THROW(em.destroy(99999));
}

TEST(EntityManager, MultipleCreateDestroyCycles) {
    EntityManager em;
    const auto a = em.create();
    const auto b = em.create();
    em.destroy(a);
    em.destroy(b);
    const auto c = em.create(); // reuses a (FIFO)
    const auto d = em.create(); // reuses b
    EXPECT_EQ(c, a);
    EXPECT_EQ(d, b);
    EXPECT_TRUE(em.isAlive(c));
    EXPECT_TRUE(em.isAlive(d));
}

// ── ComponentPool ─────────────────────────────────────────────────────────────

struct Position { float x = 0.0f; float y = 0.0f; };

TEST(ComponentPool, InsertAndGet) {
    ComponentPool<Position> pool;
    pool.insert(1, {3.0f, 4.0f});
    EXPECT_FLOAT_EQ(pool.get(1).x, 3.0f);
    EXPECT_FLOAT_EQ(pool.get(1).y, 4.0f);
}

TEST(ComponentPool, HasAfterInsert) {
    ComponentPool<Position> pool;
    pool.insert(7, {0.0f, 0.0f});
    EXPECT_TRUE(pool.has(7));
    EXPECT_FALSE(pool.has(8));
}

TEST(ComponentPool, InsertOverwritesExistingComponent) {
    ComponentPool<Position> pool;
    pool.insert(1, {1.0f, 0.0f});
    pool.insert(1, {9.0f, 0.0f});
    EXPECT_FLOAT_EQ(pool.get(1).x, 9.0f);
    EXPECT_EQ(pool.size(), 1u);
}

TEST(ComponentPool, RemoveDecrementsSize) {
    ComponentPool<Position> pool;
    pool.insert(1, {1.0f, 0.0f});
    pool.insert(2, {2.0f, 0.0f});
    pool.remove(1);
    EXPECT_EQ(pool.size(), 1u);
    EXPECT_FALSE(pool.has(1));
    EXPECT_TRUE(pool.has(2));
}

TEST(ComponentPool, RemoveKeepsOtherComponentsCorrect) {
    ComponentPool<Position> pool;
    pool.insert(1, {1.0f, 0.0f});
    pool.insert(2, {2.0f, 0.0f});
    pool.insert(3, {3.0f, 0.0f});
    pool.remove(2);
    EXPECT_FLOAT_EQ(pool.get(1).x, 1.0f);
    EXPECT_FLOAT_EQ(pool.get(3).x, 3.0f);
    EXPECT_EQ(pool.size(), 2u);
}

TEST(ComponentPool, RemoveLastElement) {
    ComponentPool<Position> pool;
    pool.insert(1, {1.0f, 0.0f});
    pool.remove(1);
    EXPECT_EQ(pool.size(), 0u);
    EXPECT_FALSE(pool.has(1));
}

TEST(ComponentPool, RemoveNonExistentIsNoop) {
    ComponentPool<Position> pool;
    pool.insert(1, {0.0f, 0.0f});
    EXPECT_NO_THROW(pool.remove(999));
    EXPECT_EQ(pool.size(), 1u);
}

TEST(ComponentPool, GetThrowsWhenMissing) {
    ComponentPool<Position> pool;
    EXPECT_THROW(pool.get(42), std::runtime_error);
}

TEST(ComponentPool, GetAllWithIDsCoversAllEntities) {
    ComponentPool<Position> pool;
    pool.insert(10, {1.0f, 0.0f});
    pool.insert(20, {2.0f, 0.0f});
    pool.insert(30, {3.0f, 0.0f});

    std::size_t count = 0;
    float xSum = 0.0f;
    for (auto [id, comp] : pool.getAllWithIDs()) {
        xSum += comp.x;
        ++count;
    }
    EXPECT_EQ(count, 3u);
    EXPECT_FLOAT_EQ(xSum, 6.0f);
}

TEST(ComponentPool, GetAllWithIDsEmptyPool) {
    ComponentPool<Position> pool;
    std::size_t count = 0;
    for ([[maybe_unused]] auto [id, comp] : pool.getAllWithIDs()) { ++count; }
    EXPECT_EQ(count, 0u);
}

TEST(ComponentPool, MutateViaGetReflectsInPool) {
    ComponentPool<Position> pool;
    pool.insert(5, {0.0f, 0.0f});
    pool.get(5).x = 42.0f;
    EXPECT_FLOAT_EQ(pool.get(5).x, 42.0f);
}
