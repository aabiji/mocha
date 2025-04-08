#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <vector>

using Task = std::function<void()>;

class ThreadPool
{
public:
    ThreadPool(int numThreads)
    {
        stop = false;
        for (int i = 0; i < numThreads; i++) {
            threads.push_back(std::thread(&ThreadPool::threadLoop, this));
        }
    }

    void dispatch(Task task)
    {
        std::unique_lock<std::mutex> lock(guard);
        tasks.push(task);
        var.notify_one();
    }

    void terminate()
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
private:
    void threadLoop()
    {
        while (true) {
            Task task;
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

    bool stop;
    std::mutex guard;
    std::condition_variable var;

    std::vector<std::thread> threads;
    std::queue<Task> tasks;
};
