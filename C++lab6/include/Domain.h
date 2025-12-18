#pragma once

#include <chrono>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

// 货物类型，用于决定托盘容量
// Тип груза, определяющий максимально допустимую загрузку поддона
enum class CargoType {
    Light = 0,
    Medium = 1,
    Heavy = 2
};

struct CategoryKey {
    CargoType type{};
    int category{};

    bool operator<(const CategoryKey& other) const noexcept {
        if (type != other.type) {
            return static_cast<int>(type) < static_cast<int>(other.type);
        }
        return category < other.category;
    }
};

// 仓库内部调度的托盘，记录货物类型和各品类数量
// Поддон внутри склада: хранит тип груза и количество по категориям
struct Pallet {
    CargoType type{};
    std::map<int, int> categoryQuantities;
    int capacity{};
};

struct Truck {
    int id{};
    std::string city;
    std::vector<Pallet> pallets;
    int totalSlots{};
};

struct Order {
    int id{};
    std::string destinationCity;
    std::map<CategoryKey, int> required;
    std::chrono::steady_clock::time_point createdAt{};
};

struct InventoryRequest {
    int id{};
    std::vector<CategoryKey> categories;
};

// 仿真配置，可通过命令行覆盖部分字段
// Конфигурация симуляции; часть полей можно переопределить через аргументы командной строки
struct SimulationConfig {
    bool fastMode{false};            // 快速模式：操作延迟1-5毫秒（正常模式1-5秒）
    int loaderCount{8};              // 搬运工数量：模拟仓库工作人员数（2-1000）
    int managerCount{2};             // 经理数量：生成任务的工作人员数（1-20）
    int simulationSeconds{20};       // 模拟运行时间（秒）
    int trucksPerManagerCycle{1};    // 每经理循环生成的卡车数量
    // 收货区
    int receivingBerths{6};          // 收货泊位数：卡车同时卸货的位置数（4-40）
    int receivingTerminals{6};       // 收货终端数：库存管理系统终端数

    // 发货区
    int shippingBerths{6};           // 发货泊位数：卡车同时装货的位置数（4-40）
    int shippingTerminals{6};        // 发货终端数

    // 包装区
    int packingStations{4};          // 包装工作台数量（2-20）
    int packingTableCapacityMin{2};  // 工作台最小容量（暂未使用）
    int packingTableCapacityMax{3};  // 工作台最大容量（暂未使用）
    int packingTerminals{4};         // 包装终端数

    // 存储区
    int storageShelves{50};          // 货架数量（10-500）
    int storageLayers{6};           // 每货架层数（最大6）
    int storageSpotsPerLayer{10};   // 每层托盘位数（10）
    int storageTerminals{4};        // 存储终端数（1-10）
};

struct LoaderStats {
    int loaderId{};
    std::map<std::string, int> tasksCompleted;
    std::chrono::nanoseconds restTime{0};
};

struct OrderStateCounters {
    int waiting{0};
    int pickingQueued{0};
    int activelyPicking{0};
    int partiallyShipped{0};
    int completed{0};
};

struct OrderHistogramBucket {
    std::chrono::milliseconds bucketStart{};
    std::chrono::milliseconds bucketEnd{};
    int count{0};
};

struct OrderReport {
    OrderStateCounters states;
    std::vector<OrderHistogramBucket> completionHistogram;
};

struct LoaderReport {
    std::vector<LoaderStats> stats;
    double restRatio{0.0};
};

// 汇总出来的两份报告体
// Структура, объединяющая два отчета: по заказам и по грузчикам
struct WarehouseReports {
    OrderReport orders;
    LoaderReport loaders;
};

