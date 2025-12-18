#include "Warehouse.h"

#include "Loader.h"
#include "Manager.h"
#include "RandomGenerator.h"

#include <algorithm>
#include <iostream>
#include <numeric>

// 初始化所有资源池与区域
// Инициализация всех ресурсных пулов и зон склада
Warehouse::Warehouse(SimulationConfig config)
    : config_(config),
      storageZone_(config.storageShelves, config.storageLayers, config.storageSpotsPerLayer),
      packingZone_(generateWorkstations()),
      shippingZone_(config.shippingBerths),
      receivingTerminals_(config.receivingTerminals, "Receiving"),
      storageTerminals_(config.storageTerminals, "Storage"),
      packingTerminals_(config.packingTerminals, "Packing"),
      shippingTerminals_(config.shippingTerminals, "Shipping") {}

Warehouse::~Warehouse() {
    stop();
    wait();
}

// 启动所有搬运工和经理线程
// Запуск потоков грузчиков и менеджеров
void Warehouse::start() {
    if (running_.exchange(true)) {
        return;
    }
    startLoaders();
    startManagers();
}

// 向线程广播停止信号并回收
// Послать потокам сигнал остановки и корректно их завершить
void Warehouse::stop() {
    if (!running_.exchange(false)) {
        dispatcher_.shutdown();
        return;
    }
    dispatcher_.shutdown();
    for (auto& loader : loaders_) {
        loader->stop();
    }
    for (auto& manager : managers_) {
        manager->stop();
    }
}

// 等待所有线程退出
// Ожидание завершения всех рабочих потоков
void Warehouse::wait() {
    for (auto& loader : loaders_) {
        loader->join();
    }
    for (auto& manager : managers_) {
        manager->join();
    }
}

int Warehouse::nextTaskId() {
    return nextTaskId_.fetch_add(1);
}

int Warehouse::nextOrderId() {
    return nextOrderId_.fetch_add(1);
}

int Warehouse::nextTruckId() {
    return nextTruckId_.fetch_add(1);
}

// 所有动作的耗时，fastMode 时为毫秒级
// Время выполнения одного действия; в режиме fast — миллисекунды
std::chrono::milliseconds Warehouse::operationDelay() const {
    if (config_.fastMode) {
        return std::chrono::milliseconds(RandomGenerator::getRandom(1, 5));
    }
    return std::chrono::milliseconds(RandomGenerator::getRandom(1000, 5000));
}

std::chrono::milliseconds Warehouse::timeoutDuration() const {
    if (config_.fastMode) {
        return std::chrono::milliseconds(5);
    }
    return std::chrono::seconds(5);
}

// 统计搬运工的休息总时长
// Учёт суммарного времени отдыха каждого грузчика
void Warehouse::recordLoaderRest(int loaderId, std::chrono::nanoseconds duration) {
    std::lock_guard lk(statsMutex_);
    loaderStats_[loaderId].loaderId = loaderId;
    loaderStats_[loaderId].restTime += duration;
}

// 统计搬运工完成的各类任务数量
// Учёт количества выполненных задач по типам для каждого грузчика
void Warehouse::recordTaskCompletion(int loaderId, TaskKind kind) {
    std::lock_guard lk(statsMutex_);
    loaderStats_[loaderId].loaderId = loaderId;
    switch (kind) {
    case TaskKind::UnloadTruck:
        loaderStats_[loaderId].tasksCompleted["UnloadTruck"]++;
        break;
    case TaskKind::InventoryAudit:
        loaderStats_[loaderId].tasksCompleted["InventoryAudit"]++;
        break;
    case TaskKind::Picking:
        loaderStats_[loaderId].tasksCompleted["Picking"]++;
        break;
    }
}

// 新订单到来时登记初始状态与时间
// Регистрация нового заказа: стартовое состояние и время создания
void Warehouse::registerOrder(const std::shared_ptr<Order>& order) {
    std::lock_guard lk(statsMutex_);
    orderStates_.waiting++;
    orderStateMap_[order->id] = "waiting";
    orderCreationTime_[order->id] = order->createdAt;
}

// 更新订单状态计数（等待/排队/拣配/部分发货/完成）
// Обновление статистики по состоянию заказов (ожидание/очередь/комплектация/частично/полностью отгружен)
void Warehouse::updateOrderState(int orderId, const std::string& state) {
    std::lock_guard lk(statsMutex_);
    auto decrement = [&](const std::string& prev) {
        if (prev == "waiting" && orderStates_.waiting > 0) {
            orderStates_.waiting--;
        } else if (prev == "queued" && orderStates_.pickingQueued > 0) {
            orderStates_.pickingQueued--;
        } else if (prev == "active" && orderStates_.activelyPicking > 0) {
            orderStates_.activelyPicking--;
        } else if (prev == "partial" && orderStates_.partiallyShipped > 0) {
            orderStates_.partiallyShipped--;
        } else if (prev == "complete" && orderStates_.completed > 0) {
            orderStates_.completed--;
        }
    };

    auto increment = [&](const std::string& next) {
        if (next == "waiting") {
            orderStates_.waiting++;
        } else if (next == "queued") {
            orderStates_.pickingQueued++;
        } else if (next == "active") {
            orderStates_.activelyPicking++;
        } else if (next == "partial") {
            orderStates_.partiallyShipped++;
        } else if (next == "complete") {
            orderStates_.completed++;
        }
    };

    auto it = orderStateMap_.find(orderId);
    if (it != orderStateMap_.end()) {
        decrement(it->second);
        if (state.empty()) {
            orderStateMap_.erase(it);
        } else {
            it->second = state;
            increment(state);
        }
    } else if (!state.empty()) {
        orderStateMap_[orderId] = state;
        increment(state);
    }
}

void Warehouse::markOrderShipped(int orderId) {
    updateOrderState(orderId, "complete");
}

// 将订单完成耗时纳入直方图
// Добавление времени выполнения заказа в гистограмму
void Warehouse::recordOrderCompletion(int orderId, std::chrono::milliseconds duration) {
    std::lock_guard lk(statsMutex_);
    constexpr std::chrono::milliseconds bucketSize{1000};
    int index = static_cast<int>(duration.count() / bucketSize.count());
    if (index >= static_cast<int>(histogram_.size())) {
        histogram_.resize(index + 1);
        for (int i = 0; i <= index; ++i) {
            histogram_[i].bucketStart = bucketSize * i;
            histogram_[i].bucketEnd = bucketSize * (i + 1);
        }
    }
    histogram_[index].count++;
    orderCreationTime_.erase(orderId);
}

std::shared_future<void> Warehouse::submitTask(const std::shared_ptr<Task>& task) {
    dispatcher_.enqueue(task);
    return task->completionFuture;
}

// 汇总报告：订单状态 + 完成直方图 + 搬运工统计
// Формирование итоговых отчетов: по заказам и по эффективности грузчиков
WarehouseReports Warehouse::buildReports() const {
    std::lock_guard lk(statsMutex_);
    WarehouseReports reports;
    reports.orders.states = orderStates_;
    reports.orders.completionHistogram = histogram_;
    for (const auto& [id, stats] : loaderStats_) {
        reports.loaders.stats.push_back(stats);
    }
    const auto totalRest = std::accumulate(loaderStats_.begin(), loaderStats_.end(), 0.0,
                                           [](double acc, const auto& entry) {
                                               return acc + static_cast<double>(entry.second.restTime.count());
                                           });
    double ratio = 0.0;
    if (!loaderStats_.empty() && config_.simulationSeconds > 0) {
        double totalBudget = static_cast<double>(loaderStats_.size()) *
                             static_cast<double>(config_.simulationSeconds) * 1'000'000'000.0;
        ratio = totalRest / totalBudget;
    }
    reports.loaders.restRatio = ratio;
    return reports;
}

// 根据配置生成搬运工线程
// Создание и запуск потоков грузчиков в соответствии с конфигурацией
void Warehouse::startLoaders() {
    loaders_.reserve(config_.loaderCount);
    for (int i = 0; i < config_.loaderCount; ++i) {
        loaders_.push_back(std::make_unique<Loader>(i, *this));
        loaders_.back()->start();
    }
}

// 根据配置生成经理线程
// Создание и запуск потоков менеджеров в соответствии с конфигурацией
void Warehouse::startManagers() {
    managers_.reserve(config_.managerCount);
    for (int i = 0; i < config_.managerCount; ++i) {
        managers_.push_back(std::make_unique<Manager>(i, *this));
        managers_.back()->start();
    }
}

// 随机生成包装工作台配置（拆垛/包装槽位数）
// Случайная конфигурация рабочих мест упаковки (число мест разбора и упаковки)
std::vector<PackingZone::WorkstationInfo> Warehouse::generateWorkstations() const {
    std::vector<PackingZone::WorkstationInfo> stations;
    stations.reserve(config_.packingStations);
    auto gen = RandomGenerator::getGenerator();
    for (int i = 0; i < config_.packingStations; ++i) {
        PackingZone::WorkstationInfo info;
        info.dismantleSlots = RandomGenerator::getRandom(gen, 1, 3);
        info.packingSlots = RandomGenerator::getRandom(gen, 2, 4);
        stations.push_back(info);
    }
    return stations;
}

