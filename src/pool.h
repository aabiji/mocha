#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <vector>

class ThreadPool
{
public:
    ThreadPool(int numThreads);
    void dispatch(std::function<void()> task);
    void terminate();
private:
    void threadLoop();

    bool stop;
    std::mutex guard;
    std::condition_variable var;

    std::vector<std::thread> threads;
    std::queue<std::function<void()>> tasks;
};
