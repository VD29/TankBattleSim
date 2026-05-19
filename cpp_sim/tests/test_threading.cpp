#include <atomic>
#include <stdexcept>
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include "threading/ThreadPool.h"
#include "threading/JobSystem.h"

// ── ThreadPool ────────────────────────────────────────────────────────────────

TEST(ThreadPool, ConstructsWithSpecifiedSize) {
    ThreadPool pool(3);
    EXPECT_EQ(pool.size(), 3u);
}

TEST(ThreadPool, ZeroThreadsBecomesOne) {
    ThreadPool pool(0);
    EXPECT_EQ(pool.size(), 1u);
}

TEST(ThreadPool, EnqueueReturnsCorrectInt) {
    ThreadPool pool(2);
    auto fut = pool.enqueue([] { return 42; });
    EXPECT_EQ(fut.get(), 42);
}

TEST(ThreadPool, EnqueueReturnsCorrectString) {
    ThreadPool pool(2);
    auto fut = pool.enqueue([] { return std::string("hello"); });
    EXPECT_EQ(fut.get(), "hello");
}

TEST(ThreadPool, EnqueueVoidTaskRuns) {
    ThreadPool pool(2);
    std::atomic<bool> ran{false};
    pool.enqueue([&ran] { ran.store(true); }).get();
    EXPECT_TRUE(ran.load());
}

TEST(ThreadPool, EnqueueWithArguments) {
    ThreadPool pool(2);
    auto fut = pool.enqueue([](int a, int b) { return a + b; }, 10, 32);
    EXPECT_EQ(fut.get(), 42);
}

TEST(ThreadPool, AllTasksComplete) {
    ThreadPool pool(4);
    std::atomic<int> counter{0};
    std::vector<std::future<void>> futs;
    for (int i = 0; i < 100; ++i) {
        futs.push_back(pool.enqueue([&counter] { counter.fetch_add(1); }));
    }
    for (auto& f : futs) f.get();
    EXPECT_EQ(counter.load(), 100);
}

TEST(ThreadPool, PropagatesException) {
    ThreadPool pool(2);
    auto fut = pool.enqueue([]() -> int { throw std::runtime_error("boom"); return 0; });
    EXPECT_THROW(fut.get(), std::runtime_error);
}

TEST(ThreadPool, IndependentFuturesHaveCorrectValues) {
    ThreadPool pool(4);
    std::vector<std::future<int>> futs;
    for (int i = 0; i < 20; ++i) {
        futs.push_back(pool.enqueue([i] { return i * i; }));
    }
    for (int i = 0; i < 20; ++i) {
        EXPECT_EQ(futs[i].get(), i * i);
    }
}

TEST(ThreadPool, SingleThreadedPoolCompletesAllTasks) {
    ThreadPool pool(1);
    std::atomic<int> counter{0};
    std::vector<std::future<void>> futs;
    for (int i = 0; i < 50; ++i) {
        futs.push_back(pool.enqueue([&counter] { counter.fetch_add(1); }));
    }
    for (auto& f : futs) f.get();
    EXPECT_EQ(counter.load(), 50);
}

TEST(ThreadPool, TasksRunAcrossMultipleThreads) {
    const std::size_t N = 4;
    ThreadPool pool(N);
    std::atomic<int> counter{0};
    std::atomic<bool> gate{false};
    std::vector<std::future<void>> futs;
    for (std::size_t i = 0; i < N; ++i) {
        futs.push_back(pool.enqueue([&counter, &gate] {
            counter.fetch_add(1);
            while (counter.load() < 4 && !gate.load()) {}
            gate.store(true);
        }));
    }
    for (auto& f : futs) f.get();
    EXPECT_EQ(counter.load(), static_cast<int>(N));
}

// ── JobSystem ─────────────────────────────────────────────────────────────────

TEST(JobSystem, ThreadCountMatchesRequested) {
    JobSystem js(3);
    EXPECT_EQ(js.threadCount(), 3u);
}

TEST(JobSystem, SubmitReturnsCorrectValue) {
    JobSystem js(2);
    auto fut = js.submit([] { return 99; });
    EXPECT_EQ(fut.get(), 99);
}

TEST(JobSystem, SubmitVoidTaskRuns) {
    JobSystem js(2);
    std::atomic<bool> ran{false};
    js.submit([&ran] { ran.store(true); }).get();
    EXPECT_TRUE(ran.load());
}

TEST(JobSystem, MultipleSubmitsAllComplete) {
    JobSystem js(4);
    std::vector<std::future<int>> futs;
    for (int i = 0; i < 20; ++i) {
        futs.push_back(js.submit([i] { return i; }));
    }
    for (int i = 0; i < 20; ++i) {
        EXPECT_EQ(futs[i].get(), i);
    }
}

TEST(JobSystem, ParallelForRunsAllIndices) {
    JobSystem js(4);
    std::vector<std::atomic<bool>> visited(50);
    js.parallelFor(50, [&visited](std::size_t i) {
        visited[i].store(true);
    });
    for (std::size_t i = 0; i < 50; ++i) {
        EXPECT_TRUE(visited[i].load()) << "index " << i << " not visited";
    }
}

TEST(JobSystem, ParallelForZeroCountIsNoop) {
    JobSystem js(2);
    EXPECT_NO_THROW(js.parallelFor(0, [](std::size_t) {}));
}

TEST(JobSystem, ParallelForFillsVector) {
    const std::size_t N = 64;
    JobSystem js(4);
    std::vector<int> out(N, 0);
    js.parallelFor(N, [&out](std::size_t i) {
        out[i] = static_cast<int>(i * 2);
    });
    for (std::size_t i = 0; i < N; ++i) {
        EXPECT_EQ(out[i], static_cast<int>(i * 2));
    }
}

TEST(JobSystem, ParallelForWithSingleThread) {
    JobSystem js(1);
    std::atomic<int> counter{0};
    js.parallelFor(10, [&counter](std::size_t) { counter.fetch_add(1); });
    EXPECT_EQ(counter.load(), 10);
}

TEST(JobSystem, ParallelForPropagatesException) {
    JobSystem js(2);
    EXPECT_THROW(
        js.parallelFor(4, [](std::size_t i) {
            if (i == 2) throw std::runtime_error("bad index");
        }),
        std::runtime_error
    );
}
