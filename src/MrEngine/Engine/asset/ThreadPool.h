#pragma once

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace AsyncLoad {

class ThreadPool {
public:
    explicit ThreadPool(std::size_t threadCount = 0);
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    bool Enqueue(const std::function<void()>& task);
    void Stop();

private:
    void WorkerLoop();

    std::vector<std::thread> workers_;
    std::queue<std::function<void()> > tasks_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool stopping_;
};

} // namespace AsyncLoad
