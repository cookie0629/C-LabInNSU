#include "ZoneResources.h"

#include "RandomGenerator.h"

#include <algorithm>
#include <sstream>

// 构造函数：初始化资源池名称和总容量，当前可用数量 = 容量
// Конструктор: инициализирует имя пула и его емкость, доступное количество = capacity
TimedResourcePool::TimedResourcePool(int capacity, std::string name)
    : name_(std::move(name)), capacity_(capacity), available_(capacity) {}

// 申请一个资源，最长等待 timeout；成功返回 true，超时仍未获得则返回 false
// Попытка захватить один ресурс с ожиданием не дольше timeout; true — успех, false — тайм-аут
bool TimedResourcePool::acquire_for(std::chrono::milliseconds timeout) {
    std::unique_lock lk(mutex_);
    if (cv_.wait_for(lk, timeout, [&] { return available_ > 0; })) {
        --available_;
        return true;
    }
    return false;
}

// 释放一个资源，并唤醒一个正在等待的线程
// Освобождение одного ресурса и пробуждение ожидающего потока
void TimedResourcePool::release() {
    {
        std::lock_guard lk(mutex_);
        ++available_;
        if (available_ > capacity_) {
            available_ = capacity_;
        }
    }
    cv_.notify_one();
}

// 创建存储区，将货架/层/托盘位编码成字符串地址放入空闲列表
// Инициализация зоны хранения: формирование списка свободных адресов в формате "Sx-Ly-Pz"
StorageZone::StorageZone(int shelves, int layers, int spotsPerLayer) {
    for (int s = 0; s < shelves; ++s) {
        for (int l = 0; l < layers; ++l) {
            for (int p = 0; p < spotsPerLayer; ++p) {
                std::ostringstream oss;
                oss << "S" << s << "-L" << l << "-P" << p;
                freeAddresses_.push_back(oss.str());
            }
        }
    }
}

// 从空闲列表中随机选择一个地址；若已满，则退化为 OVERFLOW-N
// Случайный выбор свободного адреса; при переполнении возвращаем служебный OVERFLOW-N
std::string StorageZone::allocateAddress() {
    if (freeAddresses_.empty()) {
        return "OVERFLOW-" + std::to_string(records_.size());
    }
    auto idx = RandomGenerator::getRandom(0, static_cast<int>(freeAddresses_.size()) - 1);
    std::string addr = freeAddresses_[idx];
    freeAddresses_.erase(freeAddresses_.begin() + idx);
    return addr;
}

// 将托盘放入存储区：加写锁，分配新地址并插入记录
// Размещение поддона в зоне хранения: блокировка на запись, выбор адреса и сохранение записи
std::string StorageZone::placePallet(const Pallet& pallet) {
    std::unique_lock lk(mutex_);
    StorageRecord rec;
    rec.address = allocateAddress();
    rec.pallet = pallet;
    records_.push_back(rec);
    return rec.address;
}

// 从存储区扣减指定类型+品类的货物，最多 amountRequested 件
// Списание товара из зоны хранения по типу и категории, не более amountRequested единиц
int StorageZone::takeFromStorage(const CategoryKey& category, int amountRequested) {
    std::unique_lock lk(mutex_);
    int remaining = amountRequested;
    for (auto it = records_.begin(); it != records_.end() && remaining > 0; ) {
        auto qtyIt = it->pallet.categoryQuantities.find(category.category);
        if (qtyIt != it->pallet.categoryQuantities.end() && it->pallet.type == category.type) {
            int take = std::min(qtyIt->second, remaining);
            qtyIt->second -= take;
            remaining -= take;
            if (qtyIt->second == 0) {
                it->pallet.categoryQuantities.erase(qtyIt);
            }
            if (it->pallet.categoryQuantities.empty()) {
                freeAddresses_.push_back(it->address);
                it = records_.erase(it);
                continue;
            }
        }
        ++it;
    }
    return amountRequested - remaining;
}

// 返回所有包含指定品类的存储记录快照（只读）
// Возвращает срез всех записей, содержащих указанную категорию (только для чтения)
std::vector<StorageRecord> StorageZone::recordsForCategory(const CategoryKey& category) const {
    std::shared_lock lk(mutex_);
    std::vector<StorageRecord> result;
    for (const auto& rec : records_) {
        if (rec.pallet.type != category.type) {
            continue;
        }
        auto it = rec.pallet.categoryQuantities.find(category.category);
        if (it != rec.pallet.categoryQuantities.end()) {
            result.push_back(rec);
        }
    }
    return result;
}

// 按 (类型, 品类) 聚合统计存储区中的总件数
// Агрегированная статистика количества по (типу, категории) в зоне хранения
std::map<CategoryKey, int> StorageZone::totalsByCategory() const {
    std::shared_lock lk(mutex_);
    std::map<CategoryKey, int> totals;
    for (const auto& rec : records_) {
        for (const auto& [category, qty] : rec.pallet.categoryQuantities) {
            totals[{rec.pallet.type, category}] += qty;
        }
    }
    return totals;
}

// 初始化包装区：记录每个工作台配置，并将占用标记初始化为 false
// Инициализация участка упаковки: сохраняем конфигурацию мест, все помечены как свободные
PackingZone::PackingZone(const std::vector<WorkstationInfo>& stations)
    : stations_(stations), occupied_(stations.size(), false) {}

// 预订一个空闲工作台，超时未等到则返回 std::nullopt
// Резервирование свободного рабочего места с тайм-аутом; при неудаче возвращает std::nullopt
std::optional<int> PackingZone::reserveWorkstation(std::chrono::milliseconds timeout) {
    std::unique_lock lk(mutex_);
    if (!cv_.wait_for(lk, timeout, [&] {
            return std::any_of(occupied_.begin(), occupied_.end(), [](bool occ) { return !occ; });
        })) {
        return std::nullopt;
    }
    for (std::size_t i = 0; i < occupied_.size(); ++i) {
        if (!occupied_[i]) {
            occupied_[i] = true;
            return static_cast<int>(i);
        }
    }
    return std::nullopt;
}

// 释放指定 ID 的工作台，并唤醒等待的线程
// Освобождение рабочего места по идентификатору и пробуждение ожидающего потока
void PackingZone::releaseWorkstation(int workstationId) {
    if (workstationId < 0 || workstationId >= static_cast<int>(occupied_.size())) {
        return;
    }
    {
        std::lock_guard lk(mutex_);
        occupied_[workstationId] = false;
    }
    cv_.notify_one();
}

// 查询某个工作台的配置（拆垛位/包装位数量），非法 ID 时返回默认 1/1
// Получение конфигурации рабочего места; при некорректном ID возвращает значение по умолчанию 1/1
PackingZone::WorkstationInfo PackingZone::info(int workstationId) const {
    if (workstationId < 0 || workstationId >= static_cast<int>(stations_.size())) {
        return WorkstationInfo{1, 1};
    }
    return stations_[workstationId];
}

// 当前实现中 docks 参数只作为占位，用于潜在的容量限制
// В текущей реализации docks используется только как формальный параметр (потенциальное ограничение)
ShippingZone::ShippingZone(int docks) {
    (void)docks;
}

// 注册新到达的卡车：分配泊位 ID，记录城市与总托盘位数
// Регистрация прихода машины: выделение ID рампы, сохранение города и общего числа мест
void ShippingZone::registerTruckArrival(const std::string& city, int slots) {
    std::lock_guard lk(mutex_);
    DockInfo dock;
    dock.id = nextDockId_++;
    dock.city = city;
    dock.slots = slots;
    dock.occupied = 0;
    docks_[dock.id] = dock;
    cv_.notify_all();
}

// 卡车离开：删除对应泊位记录，并唤醒等待线程
// Уход машины: удаление записи о рампе и уведомление ожидающих потоков
void ShippingZone::registerTruckDeparture(int dockId) {
    std::lock_guard lk(mutex_);
    docks_.erase(dockId);
    cv_.notify_all();
}

// 尝试向前往指定城市的卡车装载若干托盘；支持在 timeout 内等待合适的泊位出现
// Попытка загрузить указанное число поддонов на машину, идущую в нужный город; ждем не дольше timeout
bool ShippingZone::tryLoad(const std::string& city, int pallets, std::chrono::milliseconds timeout) {
    std::unique_lock lk(mutex_);
    auto tryMatch = [&]() -> bool {
        int dockToErase = -1;
        for (auto& [id, dock] : docks_) {
            if (dock.city == city && dock.occupied + pallets <= dock.slots) {
                dock.occupied += pallets;
                if (dock.occupied == dock.slots) {
                    dockToErase = id;
                }
                if (dockToErase >= 0) {
                    docks_.erase(dockToErase);
                }
                return true;
            }
        }
        return false;
    };
    if (tryMatch()) {
        return true;
    }
    if (!cv_.wait_for(lk, timeout, tryMatch)) {
        return false;
    }
    return true;
}

// 返回当前所有泊位状态的快照，用于调试或可视化
// Возвращает снимок состояния всех рамп для отладки или визуализации
std::vector<ShippingZone::DockInfo> ShippingZone::docksSnapshot() const {
    std::lock_guard lk(mutex_);
    std::vector<DockInfo> result;
    for (const auto& [id, dock] : docks_) {
        result.push_back(dock);
    }
    return result;
}

