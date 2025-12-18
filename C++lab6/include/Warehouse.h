#pragma once

#include "Domain.h"
#include "TaskDispatcher.h"
#include "ZoneResources.h"

#include <atomic>
#include <future>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

class Loader;
class Manager;

// 仓库主控：持有所有区域资源、线程对象和统计数据
// Главный класс склада: хранит ресурсы зон, потоки работников и агрегированную статистику
class Warehouse {
public:
    explicit Warehouse(SimulationConfig config);
    ~Warehouse();

    void start();
    void stop();
    void wait();

    bool running() const { return running_.load(); }
    const SimulationConfig& config() const { return config_; }

    TaskDispatcher& dispatcher() { return dispatcher_; }
    StorageZone& storageZone() { return storageZone_; }
    PackingZone& packingZone() { return packingZone_; }
    ShippingZone& shippingZone() { return shippingZone_; }
    TimedResourcePool& receivingTerminals() { return receivingTerminals_; }
    TimedResourcePool& storageTerminals() { return storageTerminals_; }
    TimedResourcePool& packingTerminals() { return packingTerminals_; }
    TimedResourcePool& shippingTerminals() { return shippingTerminals_; }

    int nextTaskId();
    int nextOrderId();
    int nextTruckId();

    std::chrono::milliseconds operationDelay() const;
    std::chrono::milliseconds timeoutDuration() const;

    void recordLoaderRest(int loaderId, std::chrono::nanoseconds duration);
    void recordTaskCompletion(int loaderId, TaskKind kind);

    void registerOrder(const std::shared_ptr<Order>& order);
    void updateOrderState(int orderId, const std::string& state);
    void markOrderShipped(int orderId);
    void recordOrderCompletion(int orderId, std::chrono::milliseconds duration);

    std::shared_future<void> submitTask(const std::shared_ptr<Task>& task);

    WarehouseReports buildReports() const;

private:
    void startLoaders();
    void startManagers();
    std::vector<PackingZone::WorkstationInfo> generateWorkstations() const;

    SimulationConfig config_;
    TaskDispatcher dispatcher_;
    StorageZone storageZone_;
    PackingZone packingZone_;
    ShippingZone shippingZone_;
    TimedResourcePool receivingTerminals_;
    TimedResourcePool storageTerminals_;
    TimedResourcePool packingTerminals_;
    TimedResourcePool shippingTerminals_;

    std::vector<std::unique_ptr<Loader>> loaders_;
    std::vector<std::unique_ptr<Manager>> managers_;

    std::atomic<bool> running_{false};
    std::atomic<int> nextTaskId_{1};
    std::atomic<int> nextOrderId_{1};
    std::atomic<int> nextTruckId_{1};

    mutable std::mutex statsMutex_;
    std::map<int, LoaderStats> loaderStats_;
    OrderStateCounters orderStates_;
    std::map<int, std::chrono::steady_clock::time_point> orderCreationTime_;
    std::vector<OrderHistogramBucket> histogram_;
    std::map<int, std::string> orderStateMap_;
};

