#include "Loader.h"

#include "RandomGenerator.h"
#include "Warehouse.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <thread>

Loader::Loader(int id, Warehouse& warehouse)
    : id_(id), warehouse_(warehouse) {}

Loader::~Loader() {
    stop();
    join();
}

void Loader::start() {
    thread_ = std::thread(&Loader::run, this);
}

void Loader::stop() {
    stopping_.store(true);
}

void Loader::join() {
    if (thread_.joinable()) {
        thread_.join();
    }
}

// 工作主循环：休息 -> 等待任务 -> 根据任务类型执行
// Основной цикл работы: отдых -> ожидание задачи -> выполнение по типу задания
void Loader::run() {
    while (!stopping_.load()) {
        auto idleStart = std::chrono::steady_clock::now();
        auto task = warehouse_.dispatcher().acquireTask();
        if (!task) {
            break;
        }
        auto idleEnd = std::chrono::steady_clock::now();
        warehouse_.recordLoaderRest(id_, idleEnd - idleStart);

        bool success = false;

        //根据task->kind分派到具体处理函数：
        //handleUnload：卸货四步（取托盘 → 收货终端登记 → 搬运到存储区 → 存储终端登记）。  
        //handleInventory：占用存储终端，对给定品类做盘点查询。  
        //handlePicking：预订拣配工作台 → 从存储区扣货 → 尝试装车 → 更新订单状态、记录耗时。  
        switch (task->kind) {
        case TaskKind::UnloadTruck:
            success = handleUnload(task);
            break;
        case TaskKind::InventoryAudit:
            success = handleInventory(task);
            break;
        case TaskKind::Picking:
            success = handlePicking(task);
            break;
        }

        if (success) {
            warehouse_.recordTaskCompletion(id_, task->kind);
            task->completed.store(true);
            if (!task->promiseFulfilled.exchange(true)) {
                task->completionPromise.set_value();
            }
        }

        warehouse_.dispatcher().finishTask(task);
    }
}

// 卸货流程：从卡车取托盘 -> 扫描终端 -> 搬至存储区 -> 在存储终端登记
// Процесс разгрузки: взять поддон из машины -> отсканировать на терминале -> перевезти в зону хранения -> отметить размещение
bool Loader::handleUnload(const std::shared_ptr<Task>& task) {
    auto* payload = std::get_if<UnloadPayload>(&task->payload);
    if (!payload) {
        return false;
    }

    while (true) {
        Pallet pallet;
        {
            std::lock_guard lk(payload->shared->mutex);
            if (payload->shared->index >= payload->truck->pallets.size()) {
                break;
            }
            pallet = payload->truck->pallets[payload->shared->index++];
        }

        std::this_thread::sleep_for(warehouse_.operationDelay()); // 取下托盘

        if (!warehouse_.receivingTerminals().acquire_for(warehouse_.timeoutDuration())) {
            bool lastWorker = task->activeLoaders.load() == 1;
            if (lastWorker) {
                return false;
            }
            continue;
        }
        std::this_thread::sleep_for(warehouse_.operationDelay()); // 扫描终端
        warehouse_.receivingTerminals().release();

        bool lastWorker = task->activeLoaders.load() == 1;
        bool moved = waitForMove("移至存储区", lastWorker);
        if (!moved) {
            warehouse_.receivingTerminals().release();
            if (lastWorker) {
                return false;
            }
            continue;
        }

        if (!warehouse_.storageTerminals().acquire_for(warehouse_.timeoutDuration())) {
            continue;
        }
        warehouse_.storageZone().placePallet(pallet);
        std::this_thread::sleep_for(warehouse_.operationDelay());
        warehouse_.storageTerminals().release();
        warehouse_.receivingTerminals().release();
    }

    return true;
}

// 库存盘点流程：占用存储终端后逐一查询指定品类
// Процесс инвентаризации: занять терминал зоны хранения и последовательно запросить указанные категории
bool Loader::handleInventory(const std::shared_ptr<Task>& task) {
    auto* request = std::get_if<InventoryRequest>(&task->payload);
    if (!request) {
        return false;
    }

    if (!warehouse_.storageTerminals().acquire_for(warehouse_.timeoutDuration())) {
        return false;
    }

    for (const auto& category : request->categories) {
        auto records = warehouse_.storageZone().recordsForCategory(category);
        (void)records;
        std::this_thread::sleep_for(warehouse_.operationDelay());
    }

    warehouse_.storageTerminals().release();
    return true;
}

// 拣配流程：预订工作台 -> 拉取货物 -> 尝试发货 -> 统计完成时间
// Процесс комплектации: зарезервировать рабочее место -> забрать товар -> попытаться отгрузить -> зафиксировать время завершения заказа
bool Loader::handlePicking(const std::shared_ptr<Task>& task) {
    auto* payload = std::get_if<PickingPayload>(&task->payload);
    if (!payload) {
        return false;
    }

    auto reservation = warehouse_.packingZone().reserveWorkstation(warehouse_.timeoutDuration());
    if (!reservation.has_value()) {
        bool lastWorker = task->activeLoaders.load() == 1;
        return !lastWorker;
    }
    int workstationId = *reservation;

    warehouse_.updateOrderState(payload->order->id, "active");

    const auto workstationInfo = warehouse_.packingZone().info(workstationId);
    std::this_thread::sleep_for(warehouse_.operationDelay()); // 预订工作台

    bool allFulfilled = true;
    while (true) {
        CategoryKey selected{};
        int chunk = 0;
        {
            std::lock_guard sharedLock(payload->shared->mutex);
            auto it = std::find_if(payload->shared->remaining.begin(), payload->shared->remaining.end(),
                                   [](const auto& entry) { return entry.second > 0; });
            if (it == payload->shared->remaining.end()) {
                break;
            }
            selected = it->first;
            chunk = std::min(it->second, workstationInfo.dismantleSlots);
            it->second -= chunk;
        }

        int taken = warehouse_.storageZone().takeFromStorage(selected, chunk);
        if (taken == 0) {
            allFulfilled = false;
            std::lock_guard sharedLock(payload->shared->mutex);
            payload->shared->remaining[selected] += chunk;
            break;
        }

        if (taken < chunk) {
            std::lock_guard sharedLock(payload->shared->mutex);
            payload->shared->remaining[selected] += (chunk - taken);
        }

        std::this_thread::sleep_for(warehouse_.operationDelay());
    }

    bool shipped = false;
    int attempts = 0;
    const int maxAttempts = 8; // 放宽发货重试次数
    if (allFulfilled) {
        while (!shipped && attempts < maxAttempts) {
            attempts++;
            shipped = warehouse_.shippingZone().tryLoad(payload->order->destinationCity, 1, warehouse_.timeoutDuration());
            if (!shipped) {
                std::this_thread::sleep_for(warehouse_.operationDelay());
            }
        }
    }

    warehouse_.packingZone().releaseWorkstation(workstationId);

    if (!shipped) {
        warehouse_.updateOrderState(payload->order->id, "partial");
        return false;
    }

    // 为避免多个搬运工对同一订单重复记“完成一次”，用 shared 标记只统计一次
    bool recordThisLoader = false;
    {
        std::lock_guard sharedLock(payload->shared->mutex);
        if (!payload->shared->completionRecorded) {
            payload->shared->completionRecorded = true;
            recordThisLoader = true;
        }
    }

    if (recordThisLoader) {
        auto now = std::chrono::steady_clock::now();
        warehouse_.markOrderShipped(payload->order->id);
        warehouse_.recordOrderCompletion(
            payload->order->id,
            std::chrono::duration_cast<std::chrono::milliseconds>(now - payload->order->createdAt));
    }

    return true;
}

// 统一的搬运动作模拟，带 15% 失败概率与“最后一人”判定
// Унифицированное моделирование перемещения с 15% вероятностью неудачи и учетом «последнего работника»
bool Loader::waitForMove(const std::string& context, bool isLastWorker) {
    (void)context;
    auto workTime = warehouse_.operationDelay();
    std::this_thread::sleep_for(workTime);
    bool success = RandomGenerator::getRandom(0, 100) > 15;
    if (!success && isLastWorker) {
        return false;
    }
    return success;
}

