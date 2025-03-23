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
    ThreadPool(int numThreads);

    void dispatch(Task task);
    void terminate();
private:
    void threadLoop();

    bool stop;
    std::mutex guard;
    std::condition_variable var;

    std::vector<std::thread> threads;
    std::queue<Task> tasks;
};
