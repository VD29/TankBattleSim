#include <array>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <thread>

#include "ecs/World.h"
#include "components/Health.h"
#include "components/Movement.h"
#include "components/ShootIntent.h"
#include "components/Target.h"
#include "components/Team.h"
#include "components/Transform.h"
#include "components/Weapon.h"
#include "systems/AISystem.h"
#include "systems/CombatSystem.h"
#include "systems/MovementSystem.h"
#include "threading/JobSystem.h"
#include "utils/EventLogger.h"

static constexpr int   TANKS_PER_TEAM = 64;
static constexpr float DT             = 0.016f;
static constexpr int   MAX_TICKS      = 500'000;

static void spawnTank(World& world, float x, float y, Team team) {
    const auto id = world.createEntity();
    world.addComponent<Transform>(id,     {x, y, 0.0f, x, y});
    world.addComponent<Movement>(id,      {0.0f, 0.0f, 50.0f, 1.0f});
    world.addComponent<Health>(id,        {100, 100, 0});
    world.addComponent<Weapon>(id,        {20, 200.0f, 1.0f, 0.0f, 0, 0});
    world.addComponent<TeamComponent>(id, {team});
}

struct SimResult {
    std::string winner;
    float       wallSec;
    int         ticks;
};

static SimResult runSim(int numThreads, const std::string& logPath) {
    { std::ofstream f(logPath, std::ios::trunc); }   // clear log before each run

    World world;
    auto logger = std::make_shared<EventLogger>(logPath);

    std::mt19937 rng(42);   // fixed seed — same battle every run
    std::uniform_real_distribution<float> yDist (  0.0f, 600.0f);
    std::uniform_real_distribution<float> xDistA(  0.0f, 300.0f);
    std::uniform_real_distribution<float> xDistB(500.0f, 800.0f);

    for (int i = 0; i < TANKS_PER_TEAM; ++i)
        spawnTank(world, xDistA(rng), yDist(rng), Team::TEAM_A);
    for (int i = 0; i < TANKS_PER_TEAM; ++i)
        spawnTank(world, xDistB(rng), yDist(rng), Team::TEAM_B);

    // Pre-warm lazy pools so AISystem never creates them during parallel ticks.
    world.getPool<TargetComponent>();
    world.getPool<ShootIntent>();

    MovementSystem movSys;
    AISystem       aiSys;
    CombatSystem   combatSys(logger);
    JobSystem      js(static_cast<std::size_t>(numThreads));

    const auto wallStart = std::chrono::steady_clock::now();

    for (int tick = 0; tick < MAX_TICKS; ++tick) {
        // MovementSystem and AISystem dispatched in parallel; CombatSystem after.
        auto movFut = js.submit([&] { movSys.update(world, DT); });
        auto aiFut  = js.submit([&] { aiSys.update(world, DT); });
        movFut.get();
        aiFut.get();

        combatSys.update(world, DT);

        int aliveA = 0, aliveB = 0;
        for (const auto id : world.view<Health, TeamComponent>()) {
            if (world.getComponent<Health>(id).isDead()) continue;
            if (world.getComponent<TeamComponent>(id).team == Team::TEAM_A) ++aliveA;
            else ++aliveB;
        }

        if (aliveA == 0 || aliveB == 0) {
            const float elapsed = std::chrono::duration<float>(
                std::chrono::steady_clock::now() - wallStart).count();
            const std::string w = aliveA > 0 ? "Team A"
                                : aliveB > 0 ? "Team B"
                                             : "Draw";
            return {w, elapsed, tick + 1};
        }
    }

    const float elapsed = std::chrono::duration<float>(
        std::chrono::steady_clock::now() - wallStart).count();
    return {"Timeout", elapsed, MAX_TICKS};
}

// ── benchmark ─────────────────────────────────────────────────────────────────

static void runBenchmark() {
    constexpr std::array<int, 3> configs = {1, 2, 4};
    std::array<SimResult, 3>     results{};

    std::cout << "Benchmark — " << TANKS_PER_TEAM
              << " tanks/team, fixed seed, 3 configurations\n\n";

    for (std::size_t i = 0; i < configs.size(); ++i) {
        std::cout << "  " << configs[i] << " thread(s)... " << std::flush;
        results[i] = runSim(configs[i],
            "/tmp/tank_bench_" + std::to_string(configs[i]) + ".jsonl");
        std::cout << std::fixed << std::setprecision(3)
                  << results[i].wallSec << "s\n";
    }

    const float base = results[0].wallSec;

    auto printTable = [&](std::ostream& os) {
        os << "\n+----------+----------+---------+\n";
        os <<   "| Threads  |     Time | Speedup |\n";
        os <<   "+----------+----------+---------+\n";
        for (std::size_t i = 0; i < configs.size(); ++i) {
            const float speedup = base / results[i].wallSec;
            os << "|        " << configs[i]
               << " | " << std::fixed << std::setprecision(3)
               << std::setw(7) << results[i].wallSec << "s"
               << " | " << std::setprecision(2) << std::setw(5) << speedup << "x |\n";
        }
        os << "+----------+----------+---------+\n";
        os << "\nWinner: " << results[0].winner
           << " in " << results[0].ticks << " ticks\n";
    };

    printTable(std::cout);

    const std::string outPath = "../../shared/benchmark_results.txt";
    if (std::ofstream f(outPath); f.is_open()) {
        printTable(f);
        std::cout << "\nSaved to shared/benchmark_results.txt\n";
    } else {
        std::cerr << "Warning: could not write to " << outPath << "\n";
    }
}

// ── main ─────────────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    const bool benchmark = argc > 1 && std::string(argv[1]) == "--benchmark";

    if (benchmark) {
        runBenchmark();
    } else {
        const auto hwThreads = std::thread::hardware_concurrency();
        const int  threads   = hwThreads > 0 ? static_cast<int>(hwThreads) : 4;

        std::cout << "TankBattleSim — " << TANKS_PER_TEAM
                  << " tanks/team, " << threads << " thread(s)\n";

        const auto r = runSim(threads, "../../shared/battle_log.jsonl");

        std::cout << "Winner:   " << r.winner << "\n"
                  << "Duration: " << std::fixed << std::setprecision(3)
                  << r.wallSec << "s wall time\n"
                  << "Ticks:    " << r.ticks << "\n";
    }
    return 0;
}
