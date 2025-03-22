#include "pool.h"

ThreadPool::ThreadPool(int numThreads)
{
    stop = false;
    for (int i = 0; i < numThreads; i++) {
        threads.push_back(std::thread(&ThreadPool::threadLoop, this));
    }
}

void ThreadPool::terminate()
{
    {
        std::unique_lock<std::mutex> lock(guard);
        stop = true;
        var.notify_all();
    }
    for (auto& t : threads) {
        t.join();
    }
}

void ThreadPool::dispatch(std::function<void()> task)
{
    std::unique_lock<std::mutex> lock(guard);
    tasks.push(task);
    var.notify_one();
}

void ThreadPool::threadLoop()
{
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(guard);
            var.wait(lock, [this]{ return !tasks.empty() || stop; });
            if (!tasks.empty()) {
                task = tasks.front();
                tasks.pop();
            }
        }
        if (stop) return;
        task();
    }
}
