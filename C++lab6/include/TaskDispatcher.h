#pragma once

#include "Domain.h"

#include <atomic>
#include <condition_variable>
#include <deque>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <variant>

struct UnloadPayload {
    std::shared_ptr<Truck> truck;
    struct Shared {
        std::mutex mutex;
        std::size_t index{0};
    };
    std::shared_ptr<Shared> shared;
    int dockId{};
};

struct PickingPayload {
    std::shared_ptr<Order> order;
    int workstationId{-1};
    int maxParallel{};
    struct Shared {
        std::mutex mutex;
        std::map<CategoryKey, int> remaining;
        bool completionRecorded{false}; // 确保每个订单只统计一次完成时间 / Гарантия, что время завершения заказа фиксируется только один раз
    };
    std::shared_ptr<Shared> shared;
};

enum class TaskKind {
    UnloadTruck,
    InventoryAudit,
    Picking
};

// 仓库调度中心发布的任务
// Задание, создаваемое диспетчером склада
struct Task {
    int id{};
    TaskKind kind{TaskKind::UnloadTruck};
    int maxParallelLoaders{1};
    std::string description;
    std::chrono::steady_clock::time_point createdAt{std::chrono::steady_clock::now()};
    std::atomic<int> activeLoaders{0};
    std::atomic<bool> completed{false};
    std::atomic<bool> promiseFulfilled{false};
    std::promise<void> completionPromise;
    std::shared_future<void> completionFuture;
    std::variant<UnloadPayload, InventoryRequest, PickingPayload> payload;
};

// 任务队列：负责线程安全地分发任务给搬运工线程
// Очередь задач: потокобезопасно раздает задания потокам-грузчикам
class TaskDispatcher {
public:
    TaskDispatcher() = default;

    void enqueue(const std::shared_ptr<Task>& task);
    std::shared_ptr<Task> acquireTask();
    void finishTask(const std::shared_ptr<Task>& task);
    void shutdown();
    size_t size() const;

private:
    std::shared_ptr<Task> acquireUnlocked();

    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::deque<std::shared_ptr<Task>> tasks_;
    bool stopping_{false};
};

