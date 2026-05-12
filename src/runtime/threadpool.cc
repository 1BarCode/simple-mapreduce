#include "mr/runtime/threadpool.h"

ThreadPool::ThreadPool(size_t num_threads) : stop_(false) {
    for (size_t i = 0; i < num_threads; ++i) {
        workers_.emplace_back([this]() {
            while (true) {
                std::unique_lock<std::mutex> lock{this->task_mutex_};

                // poll the queue without wasting cpu cycles
                // return true to continue into crit section
                this->cv_.wait(lock, [this]() {
                    return this->stop_ || !this->task_queue_.empty();
                });

                if (this->stop_ && this->task_queue_.empty()) return;

                std::function<void()> task = std::move(this->task_queue_.front());
                this->task_queue_.pop();
                lock.unlock();

                task();
            }
        });
    }
}

void ThreadPool::Enqueue(std::function<void()> task) {
    std::unique_lock<std::mutex> lock{task_mutex_};
    task_queue_.emplace(std::move(task));
    lock.unlock();

    cv_.notify_one();
}

ThreadPool::~ThreadPool() {
    std::unique_lock<std::mutex> lock{task_mutex_};
    stop_ = true;
    lock.unlock();

    cv_.notify_all();

    for (std::thread& worker : workers_) {
        if (worker.joinable()) worker.join();
    }
}
