#pragma once

#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>


class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads);
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    void Enqueue(std::function<void()> task);

private:
    std::vector<std::thread>            workers_;
    std::queue<std::function<void()>>   task_queue_;
    std::mutex                          task_mutex_;
    std::condition_variable             cv_;

    bool                                stop_ = false;
};