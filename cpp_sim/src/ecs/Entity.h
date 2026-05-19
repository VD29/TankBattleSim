#pragma once
#include <cstdint>
#include <queue>
#include <unordered_set>

using EntityID = uint32_t;

class EntityManager {
public:
    EntityID create() {
        EntityID id;
        if (!freeIDs_.empty()) {
            id = freeIDs_.front();
            freeIDs_.pop();
        } else {
            id = nextID_++;
        }
        alive_.insert(id);
        return id;
    }

    void destroy(EntityID id) {
        if (!isAlive(id)) return;
        alive_.erase(id);
        freeIDs_.push(id);
    }

    bool isAlive(EntityID id) const {
        return alive_.count(id) > 0;
    }

private:
    uint32_t nextID_ = 0;
    std::queue<EntityID> freeIDs_;
    std::unordered_set<EntityID> alive_;
};
