#pragma once

#include "TaskDispatcher.h"

#include <atomic>
#include <thread>

class Warehouse;

// 搬运工线程对象，循环从调度中心获取任务
// Поток грузчика: в цикле запрашивает задания у диспетчера и выполняет их
class Loader {
public:
    Loader(int id, Warehouse& warehouse);
    ~Loader();

    void start();
    void stop();
    void join();

private:
    void run();
    bool handleUnload(const std::shared_ptr<Task>& task);
    bool handleInventory(const std::shared_ptr<Task>& task);
    bool handlePicking(const std::shared_ptr<Task>& task);

    bool waitForMove(const std::string& context, bool isLastWorker);

    int id_;
    Warehouse& warehouse_;
    std::atomic<bool> stopping_{false};
    std::thread thread_;
    std::chrono::steady_clock::time_point idleSince_{};
};

