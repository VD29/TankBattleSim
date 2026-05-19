#pragma once
#include <array>
#include <stdexcept>
#include <unordered_map>
#include "ecs/Entity.h"
#include "utils/constants.h"

// ComponentPool keeps all components of type T in a single contiguous array.
// When a system iterates every component each frame, the CPU prefetcher loads
// the next cache line before it is needed. Pointer-chased structures (std::list,
// std::map, heap-allocated nodes) scatter data across pages, causing a cache
// miss — and a ~100-cycle stall — on every element. With 10k tanks at 60 fps
// that difference is the gap between a real-time sim and a slide show.

template <typename T>
class ComponentPool {
public:
    void insert(EntityID id, T component) {
        if (has(id)) {
            components_[entityToIndex_.at(id)] = std::move(component);
            return;
        }
        if (size_ >= MAX_ENTITIES) {
            throw std::runtime_error("ComponentPool is full");
        }
        const std::size_t idx = size_++;
        components_[idx]    = std::move(component);
        entityToIndex_[id]  = idx;
        indexToEntity_[idx] = id;
    }

    void remove(EntityID id) {
        if (!has(id)) return;

        const std::size_t idx     = entityToIndex_[id];
        const std::size_t lastIdx = size_ - 1;

        if (idx != lastIdx) {
            // Swap with last element to keep the array packed
            const EntityID lastEntity      = indexToEntity_[lastIdx];
            components_[idx]               = std::move(components_[lastIdx]);
            entityToIndex_[lastEntity]     = idx;
            indexToEntity_[idx]            = lastEntity;
        }

        entityToIndex_.erase(id);
        indexToEntity_.erase(lastIdx);
        --size_;
    }

    T& get(EntityID id) {
        if (!has(id)) throw std::runtime_error("Entity has no such component");
        return components_[entityToIndex_.at(id)];
    }

    bool has(EntityID id) const {
        return entityToIndex_.count(id) > 0;
    }

    std::size_t size() const { return size_; }

    // Lightweight non-owning view — no heap allocation, safe to range-for.
    class View {
    public:
        struct Ref { EntityID id; T& component; };

        struct Iterator {
            ComponentPool* pool;
            std::size_t    idx;

            Ref operator*() const {
                return {pool->indexToEntity_[idx], pool->components_[idx]};
            }
            Iterator& operator++() { ++idx; return *this; }
            bool operator!=(const Iterator& o) const { return idx != o.idx; }
        };

        explicit View(ComponentPool& p) : pool_(&p) {}
        Iterator begin() const { return {pool_, 0}; }
        Iterator end()   const { return {pool_, pool_->size_}; }

    private:
        ComponentPool* pool_;
    };

    View getAllWithIDs() { return View{*this}; }

private:
    std::array<T, MAX_ENTITIES> components_{};
    std::unordered_map<EntityID, std::size_t> entityToIndex_;
    std::unordered_map<std::size_t, EntityID> indexToEntity_;
    std::size_t size_ = 0;
};
