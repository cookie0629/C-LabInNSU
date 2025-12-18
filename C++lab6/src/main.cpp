#include "Warehouse.h"

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

namespace {

// 输出管理层所需的两份报告：订单状态与搬运工绩效
// Вывод двух отчетов для менеджеров: по состоянию заказов и по работе грузчиков
void printReports(const WarehouseReports& reports) {
    std::cout << "\n=== 订单状态报告 ===\n";
    std::cout << "等待中: " << reports.orders.states.waiting << '\n';
    std::cout << "拣配排队: " << reports.orders.states.pickingQueued << '\n';
    std::cout << "主动拣配: " << reports.orders.states.activelyPicking << '\n';
    std::cout << "部分已发货: " << reports.orders.states.partiallyShipped << '\n';
    std::cout << "完全已发货: " << reports.orders.states.completed << '\n';

    std::cout << "\n完成用时直方图:\n";
    for (const auto& bucket : reports.orders.completionHistogram) {
        std::cout << "[" << bucket.bucketStart.count() << "ms - "
                  << bucket.bucketEnd.count() << "ms]: " << bucket.count << '\n';
    }

    std::cout << "\n=== 搬运工绩效报告 ===\n";
    for (const auto& loader : reports.loaders.stats) {
        std::cout << "Loader #" << loader.loaderId << " 休息时间: "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(loader.restTime).count()
                  << "ms\n";
        for (const auto& [type, count] : loader.tasksCompleted) {
            std::cout << "  " << type << ": " << count << '\n';
        }
    }
    std::cout << "平均休息比例: " << reports.loaders.restRatio << '\n';
}
} // namespace

// 程序入口：解析命令行参数 -> 启动仓库 -> 等待指定时间 -> 打印报告
// Точка входа: разбор аргументов -> запуск симуляции склада -> ожидание -> печать отчетов
int main(int argc, char** argv) {
    SimulationConfig config;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--fast") {
            config.fastMode = true;
        } else if (arg == "--duration" && i + 1 < argc) {
            config.simulationSeconds = std::stoi(argv[++i]);
        } else if (arg == "--loaders" && i + 1 < argc) {
            config.loaderCount = std::stoi(argv[++i]);
        } else if (arg == "--managers" && i + 1 < argc) {
            config.managerCount = std::stoi(argv[++i]);
        }
    }

    Warehouse warehouse(config);
    warehouse.start();

    std::this_thread::sleep_for(std::chrono::seconds(config.simulationSeconds));

    warehouse.stop();
    warehouse.wait();

    auto reports = warehouse.buildReports();
    printReports(reports);

    return 0;
}

