#pragma once
#include <fstream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <nlohmann/json.hpp>
#include "ecs/Entity.h"

// Thread-safe writer for newline-delimited JSON events.
// Supported event types: "tank_moved", "damage_dealt", "tank_destroyed".
class EventLogger {
public:
    explicit EventLogger(const std::string& filepath)
        : file_(filepath, std::ios::app) {
        if (!file_.is_open()) {
            throw std::runtime_error("EventLogger: cannot open " + filepath);
        }
    }

    void log(const std::string& eventType, EntityID entity, nlohmann::json data) {
        data["type"]   = eventType;
        data["entity"] = entity;

        std::lock_guard<std::mutex> lock(mutex_);
        file_ << data.dump() << '\n';
        file_.flush();
    }

private:
    std::ofstream    file_;
    std::mutex       mutex_;
};
