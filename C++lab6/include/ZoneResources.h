#pragma once

#include "Domain.h"

#include <condition_variable>
#include <map>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

// 可以设置超时时间的共享资源池，用于模拟终端数量限制
// Обобщенный пул ресурсов с тайм-аутом, моделирует ограниченное количество терминалов
class TimedResourcePool {
public:
    TimedResourcePool(int capacity, std::string name);

    bool acquire_for(std::chrono::milliseconds timeout);
    void release();
    int capacity() const noexcept { return capacity_; }

private:
    const std::string name_;
    int capacity_;
    int available_;
    std::mutex mutex_;
    std::condition_variable cv_;
};

// 存储区的一条记录，包含地址 + 托盘内容
// Запись в зоне хранения: адрес ячейки и содержимое поддона
struct StorageRecord {
    std::string address;
    Pallet pallet;
};

// 存储区：负责分配地址、统计货物与取货
// Зона хранения: выделяет адреса, учитывает остатки и отдает товар
class StorageZone {
public:
    StorageZone(int shelves, int layers, int spotsPerLayer);

    std::string placePallet(const Pallet& pallet);
    int takeFromStorage(const CategoryKey& category, int amountRequested);
    std::vector<StorageRecord> recordsForCategory(const CategoryKey& category) const;
    std::map<CategoryKey, int> totalsByCategory() const;

private:
    std::string allocateAddress();

    mutable std::shared_mutex mutex_;
    std::vector<StorageRecord> records_;
    std::vector<std::string> freeAddresses_;
};

// 包装区：管理拣配工作台的占用情况
// Участок упаковки: управляет занятостью рабочих мест комплектации
class PackingZone {
public:
    struct WorkstationInfo {
        int dismantleSlots;
        int packingSlots;
    };

    explicit PackingZone(const std::vector<WorkstationInfo>& stations);

    std::optional<int> reserveWorkstation(std::chrono::milliseconds timeout);
    void releaseWorkstation(int workstationId);
    WorkstationInfo info(int workstationId) const;

private:
    std::vector<WorkstationInfo> stations_;
    std::vector<bool> occupied_;
    std::mutex mutex_;
    std::condition_variable cv_;
};

// 发货区：跟踪泊位/卡车到达与装载状态
// Зона отгрузки: отслеживает рампы, приходящие/уходящие машины и загрузку поддонов
class ShippingZone {
public:
    struct DockInfo {
        int id{};
        std::string city;
        int slots{};
        int occupied{};
    };

    ShippingZone(int docks);

    void registerTruckArrival(const std::string& city, int slots);
    void registerTruckDeparture(int dockId);
    bool tryLoad(const std::string& city, int pallets, std::chrono::milliseconds timeout);
    std::vector<DockInfo> docksSnapshot() const;

private:
    int nextDockId_{0};
    std::map<int, DockInfo> docks_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
};

