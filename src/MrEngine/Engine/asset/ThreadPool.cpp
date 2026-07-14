#include "asset/ThreadPool.h"

#include <algorithm>

namespace AsyncLoad {

ThreadPool::ThreadPool(std::size_t threadCount) : stopping_(false) {
    if (threadCount == 0) {
        threadCount = std::thread::hardware_concurrency();
        if (threadCount == 0) {
            threadCount = 1;
        }
    }

    for (std::size_t i = 0; i < threadCount; ++i) {
        workers_.push_back(std::thread(&ThreadPool::WorkerLoop, this));
    }
}

ThreadPool::~ThreadPool() {
    Stop();
}

bool ThreadPool::Enqueue(const std::function<void()>& task) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stopping_) {
            return false;
        }
        tasks_.push(task);
    }
    cv_.notify_one();
    return true;
}

void ThreadPool::Stop() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stopping_) {
            return;
        }
        stopping_ = true;
    }

    cv_.notify_all();

    for (std::size_t i = 0; i < workers_.size(); ++i) {
        if (workers_[i].joinable()) {
            workers_[i].join();
        }
    }
    workers_.clear();
}

void ThreadPool::WorkerLoop() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this]() { return stopping_ || !tasks_.empty(); });

            if (stopping_ && tasks_.empty()) {
                return;
            }

            task = tasks_.front();
            tasks_.pop();
        }

        if (task) {
            task();
        }
    }
}

} // namespace AsyncLoad
