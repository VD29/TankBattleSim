#pragma once
#include <functional>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>
#include "ecs/ComponentPool.h"
#include "ecs/Entity.h"

class World {
public:
    // ── Entity lifecycle ──────────────────────────────────────────────────────

    EntityID createEntity() {
        return entityManager_.create();
    }

    void destroyEntity(EntityID id) {
        // removers_ covers every pool type seen so far; remove is a no-op when
        // the entity does not have that component, so this is always safe.
        for (auto& [key, remover] : removers_) {
            remover(id);
        }
        entityManager_.destroy(id);
    }

    // ── Component access ──────────────────────────────────────────────────────

    template <typename T>
    void addComponent(EntityID id, T component) {
        getPool<T>().insert(id, std::move(component));
    }

    template <typename T>
    T& getComponent(EntityID id) {
        return getPool<T>().get(id);
    }

    template <typename T>
    bool hasComponent(EntityID id) {
        const auto key = std::type_index(typeid(T));
        const auto it  = pools_.find(key);
        if (it == pools_.end()) return false;
        return std::static_pointer_cast<ComponentPool<T>>(it->second)->has(id);
    }

    template <typename T>
    void removeComponent(EntityID id) {
        const auto key = std::type_index(typeid(T));
        const auto it  = pools_.find(key);
        if (it != pools_.end()) {
            std::static_pointer_cast<ComponentPool<T>>(it->second)->remove(id);
        }
    }

    // ── Pool access ───────────────────────────────────────────────────────────

    // Creates the pool on first access; subsequent calls return the same pool.
    template <typename T>
    ComponentPool<T>& getPool() {
        const auto key = std::type_index(typeid(T));
        const auto it  = pools_.find(key);
        if (it != pools_.end()) {
            return *std::static_pointer_cast<ComponentPool<T>>(it->second);
        }
        auto pool      = std::make_shared<ComponentPool<T>>();
        // Capture a typed shared_ptr so destroyEntity can call remove without
        // knowing T at the call site (type erasure via std::function).
        removers_[key] = [pool](EntityID id) { pool->remove(id); };
        pools_[key]    = pool;  // implicit upcast: shared_ptr<ComponentPool<T>> -> shared_ptr<void>
        return *pool;
    }

    // ── View ──────────────────────────────────────────────────────────────────

    // Returns every EntityID that possesses all component types in the pack.
    // Iterates the First pool (usually the rarest component) and filters by
    // the remaining types, so keep the most selective type first for speed.
    template <typename First, typename... Rest>
    std::vector<EntityID> view() {
        std::vector<EntityID> result;
        for ([[maybe_unused]] auto [id, comp] : getPool<First>().getAllWithIDs()) {
            if ((hasComponent<Rest>(id) && ...)) {
                result.push_back(id);
            }
        }
        return result;
    }

private:
    EntityManager entityManager_;

    // Type-erased pool storage: one entry per component type ever registered.
    std::unordered_map<std::type_index, std::shared_ptr<void>> pools_;

    // Parallel map of typed erasers so destroyEntity can remove from every
    // pool without knowing the component types at the call site.
    std::unordered_map<std::type_index, std::function<void(EntityID)>> removers_;
};
