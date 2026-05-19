#pragma once
#include <future>
#include <vector>
#include "threading/ThreadPool.h"

// Higher-level job scheduler built on top of ThreadPool.
// submit()      — fire-and-forget with a returned future.
// parallelFor() — fan out f(index) across all indices, then block until done.
class JobSystem {
public:
    explicit JobSystem(std::size_t numThreads = std::thread::hardware_concurrency())
        : pool_(numThreads) {}

    template<typename F>
    auto submit(F&& f) -> std::future<std::invoke_result_t<F>>;

    template<typename F>
    void parallelFor(std::size_t count, F f);

    std::size_t threadCount() const { return pool_.size(); }

private:
    ThreadPool pool_;
};

template<typename F>
auto JobSystem::submit(F&& f) -> std::future<std::invoke_result_t<F>> {
    return pool_.enqueue(std::forward<F>(f));
}

template<typename F>
void JobSystem::parallelFor(std::size_t count, F f) {
    if (count == 0) return;
    std::vector<std::future<void>> futures;
    futures.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        futures.push_back(pool_.enqueue([f, i] { f(i); }));
    }
    for (auto& fut : futures) fut.get();
}
