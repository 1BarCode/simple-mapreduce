#include "mr/runtime/threadpool.h"

ThreadPool::ThreadPool(size_t numThreads) : stop_(false) {
    for(size_t i = 0; i < numThreads; ++i) {
        workers_.emplace_back([this](){
            while (true) {
                std::unique_lock<std::mutex> lock{this->task_mutex_};
                // return bool: true means proceed
                this->cv_.wait(lock, [this](){
                    return this->stop_ || !this->taskQueue_.empty();
                });

                if (this->stop_ && this->taskQueue_.empty()) return;

                std::function<void()> task = std::move(this->taskQueue_.front());
                this->taskQueue_.pop();
                lock.unlock();

                task();
            };
        });
    };
};

void ThreadPool::Enqueue(std::function<void()> task) {
    std::unique_lock<std::mutex> lock{task_mutex_};
    taskQueue_.emplace(std::move(task));
    lock.unlock();

    cv_.notify_one();
};


ThreadPool::~ThreadPool() {
    std::unique_lock<std::mutex> lock{task_mutex_};
    stop_ = true;
    lock.unlock();

    cv_.notify_all(); // wake all threads to check for stop flag

    for (std::thread &worker : workers_) {
        if (worker.joinable()) worker.join();
    }
};