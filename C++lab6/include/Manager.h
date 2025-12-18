#pragma once

#include "Domain.h"

#include <atomic>
#include <thread>

class Warehouse;

// 经理线程：周期性生成卸货/盘点/拣配任务并发来发往城市的卡车
// Поток менеджера: периодически создает задачи разгрузки, инвентаризации и комплектации, а также уведомляет о прибывающих/отбывающих машинах
class Manager {
public:
    Manager(int id, Warehouse& warehouse);
    ~Manager();

    void start();
    void stop();
    void join();

private:
    void run();
    void scheduleTrucks();
    void scheduleInventory();
    void scheduleOrders();

    Pallet randomPallet();
    std::shared_ptr<Truck> randomTruck();
    std::shared_ptr<Order> randomOrder();
    InventoryRequest randomInventoryRequest();

    int id_;
    Warehouse& warehouse_;
    std::atomic<bool> stopping_{false};
    std::thread thread_;
};

