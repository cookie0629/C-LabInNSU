#include "Manager.h"

#include "Loader.h"
#include "RandomGenerator.h"
#include "TaskDispatcher.h"
#include "Warehouse.h"

#include <chrono>
#include <thread>

namespace {
const std::vector<std::string> Cities = {"上海", "北京", "广州", "深圳", "成都"};

CargoType randomCargoType(std::mt19937& gen) {
    int value = RandomGenerator::getRandom(gen, 0, 2);
    return static_cast<CargoType>(value);
}

int capacityForType(CargoType type) {
    switch (type) {
    case CargoType::Light:
        return 100;
    case CargoType::Medium:
        return 30;
    case CargoType::Heavy:
        return 4;
    }
    return 10;
}
} // namespace

Manager::Manager(int id, Warehouse& warehouse)
    : id_(id), warehouse_(warehouse) {}

Manager::~Manager() {
    stop();
    join();
}

void Manager::start() {
    thread_ = std::thread(&Manager::run, this);
}

void Manager::stop() {
    stopping_.store(true);
}

void Manager::join() {
    if (thread_.joinable()) {
        thread_.join();
    }
}

// 周期性循环：创建任务 + 安排卡车到离港
// Основной цикл менеджера: создаем новые задачи и периодически добавляем машины в зону отгрузки
void Manager::run() {
    auto startTime = std::chrono::steady_clock::now();
    auto nextShipping = startTime;
    const auto halfDuration = std::chrono::milliseconds(
        static_cast<int64_t>(warehouse_.config().simulationSeconds * 1000 / 2));

    while (!stopping_.load() && warehouse_.running()) {
        const auto now = std::chrono::steady_clock::now();
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime);

        scheduleTrucks();
        scheduleInventory();

        // 节流订单：运行前半段生成，且当队列压力不过高时才继续生成
        //Заказы, позволяющие снизить затраты: формируются в первой половине процесса и 
        // продолжают формироваться только тогда, 
        // когда нагрузка на очередь не слишком высока.
        if (elapsed < halfDuration) {
            if (warehouse_.dispatcher().size() < 200) {
                scheduleOrders();
            }
        }

        if (now >= nextShipping) {
            auto city = Cities[RandomGenerator::getRandom(0, static_cast<int>(Cities.size()) - 1)];
            warehouse_.shippingZone().registerTruckArrival(city, RandomGenerator::getRandom(10, 100));
            nextShipping = now + std::chrono::seconds(warehouse_.config().fastMode ? 1 : 5);
        }

        std::this_thread::sleep_for(warehouse_.operationDelay());
    }
}

// 生成卸货任务：每辆卡车允许最多 3 名搬运工并行
// Формирование задач разгрузки: для одной машины одновременно может работать не более 3 грузчиков
void Manager::scheduleTrucks() {
    for (int i = 0; i < warehouse_.config().trucksPerManagerCycle; ++i) {
        auto truck = randomTruck();
        auto task = std::make_shared<Task>();
        task->id = warehouse_.nextTaskId();
        task->kind = TaskKind::UnloadTruck;
        task->maxParallelLoaders = 3;

        UnloadPayload payload;
        payload.truck = truck;
        payload.shared = std::make_shared<UnloadPayload::Shared>();
        payload.dockId = RandomGenerator::getRandom(0, warehouse_.config().receivingBerths - 1);
        task->payload = payload;
        task->description = "卸货任务 #" + std::to_string(truck->id);

        warehouse_.submitTask(task); // 返回的 future 可用于等待任务完成 / Возвращаемое future можно при желании ожидать
    }
}

// 随机指定若干品类触发盘点任务
// Формирование задач инвентаризации по случайному набору категорий
void Manager::scheduleInventory() {
    auto request = randomInventoryRequest();
    auto task = std::make_shared<Task>();
    task->id = warehouse_.nextTaskId();
    task->kind = TaskKind::InventoryAudit;
    task->maxParallelLoaders = 1;
    task->payload = request;
    task->description = "库存盘点任务 #" + std::to_string(request.id);
    warehouse_.submitTask(task); // future 可在需要时等待 / future можно ожидать при необходимости
}

// 随机生成订单 -> 注册状态 -> 发布拣配任务
// Генерация случайного заказа -> регистрация его состояния -> публикация задачи комплектации
void Manager::scheduleOrders() {
    auto order = randomOrder();
    warehouse_.registerOrder(order);
    warehouse_.updateOrderState(order->id, "queued");
    auto task = std::make_shared<Task>();
    task->id = warehouse_.nextTaskId();
    task->kind = TaskKind::Picking;
    task->maxParallelLoaders = RandomGenerator::getRandom(2, 4);

    PickingPayload payload;
    payload.order = order;
    payload.maxParallel = task->maxParallelLoaders;
    payload.shared = std::make_shared<PickingPayload::Shared>();
    payload.shared->remaining = order->required;
    task->payload = payload;

    task->description = "拣配任务 #" + std::to_string(order->id);
    warehouse_.submitTask(task); // future 可在需要时等待 / future можно ожидать при необходимости
}

// 生成一块随机托盘，满足不同类型的容量限制
// Формирование случайного поддона с учетом ограничений по типам груза
Pallet Manager::randomPallet() {
    static thread_local auto gen = RandomGenerator::getGenerator();
    Pallet pallet;
    pallet.type = randomCargoType(gen);
    pallet.capacity = capacityForType(pallet.type);
    int categories = 0;
    switch (pallet.type) {
    case CargoType::Light:
        categories = 3;
        break;
    case CargoType::Medium:
        categories = 2;
        break;
    case CargoType::Heavy:
        categories = 3;
        break;
    }
    int items = RandomGenerator::getRandom(gen, 2, pallet.capacity);
    while (items > 0) {
        int category = RandomGenerator::getRandom(gen, 0, categories - 1);
        int load = std::min(items, RandomGenerator::getRandom(gen, 1, pallet.capacity / categories + 1));
        pallet.categoryQuantities[category] += load;
        items -= load;
    }
    return pallet;
}

// 生成随机卡车及其托盘列表
// Генерация случайной машины и списка поддонов в ней
std::shared_ptr<Truck> Manager::randomTruck() {
    static thread_local auto gen = RandomGenerator::getGenerator();
    auto truck = std::make_shared<Truck>();
    truck->id = warehouse_.nextTruckId();
    truck->city = Cities[RandomGenerator::getRandom(gen, 0, static_cast<int>(Cities.size()) - 1)];
    int pallets = RandomGenerator::getRandom(gen, 10, 100);
    truck->pallets.reserve(pallets);
    for (int i = 0; i < pallets; ++i) {
        truck->pallets.push_back(randomPallet());
    }
    truck->totalSlots = pallets;
    return truck;
}

// 生成随机订单及各品类需求
// Генерация случайного заказа и требуемых количеств по категориям
std::shared_ptr<Order> Manager::randomOrder() {
    static thread_local auto gen = RandomGenerator::getGenerator();
    auto order = std::make_shared<Order>();
    order->id = warehouse_.nextOrderId();
    order->destinationCity = Cities[RandomGenerator::getRandom(gen, 0, static_cast<int>(Cities.size()) - 1)];
    order->createdAt = std::chrono::steady_clock::now();
    int entries = RandomGenerator::getRandom(gen, 1, 5);
    for (int i = 0; i < entries; ++i) {
        CategoryKey key;
        key.type = randomCargoType(gen);
        int categories = key.type == CargoType::Medium ? 2 : 3;
        key.category = RandomGenerator::getRandom(gen, 0, categories - 1);
        order->required[key] += RandomGenerator::getRandom(gen, 1, 30);
    }
    return order;
}

// 构建库存盘点需求
// Формирование запроса инвентаризации по нескольким категориям
InventoryRequest Manager::randomInventoryRequest() {
    static thread_local auto gen = RandomGenerator::getGenerator();
    InventoryRequest request;
    request.id = warehouse_.nextTaskId();
    int entries = RandomGenerator::getRandom(gen, 1, 4);
    for (int i = 0; i < entries; ++i) {
        CategoryKey key;
        key.type = randomCargoType(gen);
        int categories = key.type == CargoType::Medium ? 2 : 3;
        key.category = RandomGenerator::getRandom(gen, 0, categories - 1);
        request.categories.push_back(key);
    }
    return request;
}

