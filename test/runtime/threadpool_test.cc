#include <gtest/gtest.h>
#include "mr/runtime/threadpool.h"

#include <atomic>
#include <chrono>
#include <vector>

TEST(ThreadPoolTest, AllTasksCompleteBeforeDestructor) {
    std::atomic<int> counter{0};
    {
        ThreadPool pool(4);
        for (int i = 0; i < 100; ++i)
            pool.Enqueue([&counter]() { counter.fetch_add(1); });
    }
    EXPECT_EQ(counter.load(), 100);
}

TEST(ThreadPoolTest, SingleThread) {
    std::atomic<int> counter{0};
    {
        ThreadPool pool(1);
        for (int i = 0; i < 50; ++i)
            pool.Enqueue([&counter]() { counter.fetch_add(1); });
    }
    EXPECT_EQ(counter.load(), 50);
}

TEST(ThreadPoolTest, TasksRunConcurrently) {
    const int num_tasks = 8;
    std::atomic<int> running{0};
    std::atomic<int> peak{0};
    std::mutex peak_mutex;

    {
        ThreadPool pool(num_tasks);
        for (int i = 0; i < num_tasks; ++i) {
            pool.Enqueue([&]() {
                int current = running.fetch_add(1) + 1;
                {
                    std::lock_guard<std::mutex> lock(peak_mutex);
                    if (current > peak.load()) peak.store(current);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                running.fetch_sub(1);
            });
        }
    }
    EXPECT_GT(peak.load(), 1);
}

TEST(ThreadPoolTest, EmptyPoolDestructsCleanly) {
    EXPECT_NO_FATAL_FAILURE({
        ThreadPool pool(4);
    });
}

TEST(ThreadPoolTest, NoTasksEnqueued) {
    std::atomic<int> counter{0};
    {
        ThreadPool pool(4);
    }
    EXPECT_EQ(counter.load(), 0);
}
