#include <algorithm>
#include <gtest/gtest.h>
#include "ecs/Entity.h"
#include "ecs/ComponentPool.h"
#include "ecs/World.h"
#include "components/Transform.h"
#include "components/Movement.h"
#include "components/Health.h"

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


TEST(ComponentPool, InsertAndGet) {
    ComponentPool<Transform> pool;
    pool.insert(1, {3.0f, 4.0f});
    EXPECT_FLOAT_EQ(pool.get(1).x, 3.0f);
    EXPECT_FLOAT_EQ(pool.get(1).y, 4.0f);
}

TEST(ComponentPool, HasAfterInsert) {
    ComponentPool<Transform> pool;
    pool.insert(7, {0.0f, 0.0f});
    EXPECT_TRUE(pool.has(7));
    EXPECT_FALSE(pool.has(8));
}

TEST(ComponentPool, InsertOverwritesExistingComponent) {
    ComponentPool<Transform> pool;
    pool.insert(1, {1.0f, 0.0f});
    pool.insert(1, {9.0f, 0.0f});
    EXPECT_FLOAT_EQ(pool.get(1).x, 9.0f);
    EXPECT_EQ(pool.size(), 1u);
}

TEST(ComponentPool, RemoveDecrementsSize) {
    ComponentPool<Transform> pool;
    pool.insert(1, {1.0f, 0.0f});
    pool.insert(2, {2.0f, 0.0f});
    pool.remove(1);
    EXPECT_EQ(pool.size(), 1u);
    EXPECT_FALSE(pool.has(1));
    EXPECT_TRUE(pool.has(2));
}

TEST(ComponentPool, RemoveKeepsOtherComponentsCorrect) {
    ComponentPool<Transform> pool;
    pool.insert(1, {1.0f, 0.0f});
    pool.insert(2, {2.0f, 0.0f});
    pool.insert(3, {3.0f, 0.0f});
    pool.remove(2);
    EXPECT_FLOAT_EQ(pool.get(1).x, 1.0f);
    EXPECT_FLOAT_EQ(pool.get(3).x, 3.0f);
    EXPECT_EQ(pool.size(), 2u);
}

TEST(ComponentPool, RemoveLastElement) {
    ComponentPool<Transform> pool;
    pool.insert(1, {1.0f, 0.0f});
    pool.remove(1);
    EXPECT_EQ(pool.size(), 0u);
    EXPECT_FALSE(pool.has(1));
}

TEST(ComponentPool, RemoveNonExistentIsNoop) {
    ComponentPool<Transform> pool;
    pool.insert(1, {0.0f, 0.0f});
    EXPECT_NO_THROW(pool.remove(999));
    EXPECT_EQ(pool.size(), 1u);
}

TEST(ComponentPool, GetThrowsWhenMissing) {
    ComponentPool<Transform> pool;
    EXPECT_THROW(pool.get(42), std::runtime_error);
}

TEST(ComponentPool, GetAllWithIDsCoversAllEntities) {
    ComponentPool<Transform> pool;
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
    ComponentPool<Transform> pool;
    std::size_t count = 0;
    for ([[maybe_unused]] auto [id, comp] : pool.getAllWithIDs()) { ++count; }
    EXPECT_EQ(count, 0u);
}

TEST(ComponentPool, MutateViaGetReflectsInPool) {
    ComponentPool<Transform> pool;
    pool.insert(5, {0.0f, 0.0f});
    pool.get(5).x = 42.0f;
    EXPECT_FLOAT_EQ(pool.get(5).x, 42.0f);
}

// ── World ─────────────────────────────────────────────────────────────────────

TEST(World, CreateEntityReturnsAliveID) {
    World world;
    const auto id = world.createEntity();
    // entity lifecycle is owned by World; just verify add/has round-trip works
    world.addComponent<Transform>(id, {0.0f, 0.0f});
    EXPECT_TRUE(world.hasComponent<Transform>(id));
}

TEST(World, AddAndGetComponent) {
    World world;
    const auto id = world.createEntity();
    world.addComponent<Transform>(id, {3.0f, 7.0f});
    EXPECT_FLOAT_EQ(world.getComponent<Transform>(id).x, 3.0f);
    EXPECT_FLOAT_EQ(world.getComponent<Transform>(id).y, 7.0f);
}

TEST(World, HasComponentReturnsFalseBeforeAdd) {
    World world;
    const auto id = world.createEntity();
    EXPECT_FALSE(world.hasComponent<Movement>(id));
}

TEST(World, RemoveComponentClearsIt) {
    World world;
    const auto id = world.createEntity();
    world.addComponent<Health>(id, {50});
    world.removeComponent<Health>(id);
    EXPECT_FALSE(world.hasComponent<Health>(id));
}

TEST(World, MutateViaGetComponent) {
    World world;
    const auto id = world.createEntity();
    world.addComponent<Health>(id, {100});
    world.getComponent<Health>(id).hp -= 30;
    EXPECT_EQ(world.getComponent<Health>(id).hp, 70);
}

TEST(World, DestroyEntityRemovesAllComponents) {
    World world;
    const auto id = world.createEntity();
    world.addComponent<Transform>(id, {1.0f, 2.0f});
    world.addComponent<Movement>(id, {3.0f, 4.0f});
    world.addComponent<Health>(id, {100});
    world.destroyEntity(id);
    EXPECT_FALSE(world.hasComponent<Transform>(id));
    EXPECT_FALSE(world.hasComponent<Movement>(id));
    EXPECT_FALSE(world.hasComponent<Health>(id));
}

TEST(World, DestroyEntityLeavesOtherEntitiesIntact) {
    World world;
    const auto a = world.createEntity();
    const auto b = world.createEntity();
    world.addComponent<Transform>(a, {1.0f, 0.0f});
    world.addComponent<Transform>(b, {2.0f, 0.0f});
    world.destroyEntity(a);
    EXPECT_TRUE(world.hasComponent<Transform>(b));
    EXPECT_FLOAT_EQ(world.getComponent<Transform>(b).x, 2.0f);
}

TEST(World, GetPoolReturnsSameInstance) {
    World world;
    EXPECT_EQ(&world.getPool<Transform>(), &world.getPool<Transform>());
}

TEST(World, ViewSingleComponentType) {
    World world;
    const auto a = world.createEntity();
    const auto b = world.createEntity();
    world.createEntity(); // c — no component
    world.addComponent<Health>(a, {100});
    world.addComponent<Health>(b, {50});

    const auto ids = world.view<Health>();
    EXPECT_EQ(ids.size(), 2u);
    EXPECT_NE(std::find(ids.begin(), ids.end(), a), ids.end());
    EXPECT_NE(std::find(ids.begin(), ids.end(), b), ids.end());
}

TEST(World, ViewMultipleComponentTypes) {
    World world;
    const auto a = world.createEntity(); // Transform + Movement
    const auto b = world.createEntity(); // Transform only
    const auto c = world.createEntity(); // Transform + Movement + Health

    world.addComponent<Transform>(a, {1.0f, 0.0f});
    world.addComponent<Movement>(a, {0.0f, 1.0f});

    world.addComponent<Transform>(b, {2.0f, 0.0f});

    world.addComponent<Transform>(c, {3.0f, 0.0f});
    world.addComponent<Movement>(c, {0.0f, 2.0f});
    world.addComponent<Health>(c, {80});

    const auto ids = world.view<Transform, Movement>();
    EXPECT_EQ(ids.size(), 2u);
    EXPECT_NE(std::find(ids.begin(), ids.end(), a), ids.end());
    EXPECT_NE(std::find(ids.begin(), ids.end(), c), ids.end());
    EXPECT_EQ(std::find(ids.begin(), ids.end(), b), ids.end());
}

TEST(World, ViewThreeComponentTypes) {
    World world;
    const auto a = world.createEntity();
    const auto b = world.createEntity();
    world.addComponent<Transform>(a, {0.0f, 0.0f});
    world.addComponent<Movement>(a, {0.0f, 0.0f});
    world.addComponent<Health>(a, {100});
    world.addComponent<Transform>(b, {0.0f, 0.0f});
    world.addComponent<Movement>(b, {0.0f, 0.0f});
    // b has no Health

    const auto ids = world.view<Transform, Movement, Health>();
    EXPECT_EQ(ids.size(), 1u);
    EXPECT_EQ(ids[0], a);
}

TEST(World, ViewReturnsEmptyWhenNoMatch) {
    World world;
    const auto id = world.createEntity();
    world.addComponent<Transform>(id, {0.0f, 0.0f});
    const auto ids = world.view<Transform, Movement>();
    EXPECT_TRUE(ids.empty());
}

TEST(World, ViewEmptyWorld) {
    World world;
    EXPECT_TRUE(world.view<Transform>().empty());
}
