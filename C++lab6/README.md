# 仓库并发仿真（第六题）

## 项目简介
- **核心目标**：利用 C++20 多线程、同步原语和自定义资源池，模拟物流仓库中从收货、存储、拣配到发货的全流程。
- **主要角色**：
  - `Manager`：周期性生成卸货、库存盘点、订单拣配任务，并随机通知发货卡车到港。
  - `Loader`：搬运工线程，从 `TaskDispatcher` 获取任务执行具体操作，包含 5 秒超时/重试逻辑。
  - `Warehouse`：集中管理各区域资源（终端、工作台、泊位）、统计数据与报告输出。
- **关键模块**：
  - `ZoneResources`：实现收货/存储/包装/发货区域的资源池，以及存储区货物记录。
  - `TaskDispatcher`：线程安全任务队列，保证任务的最大并行度限制，并为每个任务提供 `future/promise` 完成通知。
  - `RandomGenerator`：统一生成符合题意区间的随机数。

## 构建与运行
```bash
cmake -S . -B build
cmake --build build
```

### 运行示例
```bash
./build/Debug/warehouse_app.exe --fast --duration 5 --loaders 8 --managers 2
```

#### 常用参数
| 参数 | 说明 | 默认值 |
| --- | --- | --- |
| `--fast` | 动作以毫秒级耗时运行，适合调试 | 关闭 |
| `--duration <秒>` | 仿真总时间 | 20 |
| `--loaders <数量>` | 搬运工线程数 | 8 |
| `--managers <数量>` | 经理线程数 | 2 |

## 结果输出
程序退出前打印两份中文报告：
1. **订单状态报告**：各状态数量 + 完成时间直方图。
2. **搬运工绩效报告**：单个搬运工完成任务数量、休息时长、休息比例。

## 任务完成通知（future/promise）
- 每个 `Task` 内部都有 `std::promise<void>` 和共享的 `std::shared_future<void>`。
- 调用 `Warehouse::submitTask(const std::shared_ptr<Task>&)` 会返回该任务的 `shared_future`，可在需要时 `wait()` 或 `wait_for()` 观察任务是否完成（完成 = 至少一名搬运工成功执行并标记 `completed`）。
- 典型用法示例：
  ```cpp
  auto fut = warehouse.submitTask(task);
  fut.wait(); // 阻塞等待任务完成
  ```

## 目录结构
- `include/`：公共头文件（领域模型、各类对象声明）。
- `src/`：实现文件与 `main.cpp`。
- `第六题.md`：原始题目需求。
- `新手指导.md`：面向新手的学习说明（见同级文件）。





### 一、整体运行流程（从 `main` 到各个线程）

1. **程序入口 `main.cpp`**  
   - 解析命令行参数：`--fast`, `--duration`, `--loaders`, `--managers`。  
   - 构造 `SimulationConfig` 并创建 `Warehouse warehouse(config)`。  
   - 调用 `warehouse.start()`：启动所有搬运工(`Loader`)和经理(`Manager`)线程。  
   - 主线程 `sleep_for(simulationSeconds)` 等待仿真进行。  
   - 调用 `warehouse.stop()` 广播停止，`warehouse.wait()` 等待所有线程退出。  
   - 最后调用 `warehouse.buildReports()`，把结果打印出来（订单状态 + 直方图 + 搬运工绩效）。

2. **仓库主控 `Warehouse` 启动流程**（`Warehouse::start`）  
   - 创建 `StorageZone` / `PackingZone` / `ShippingZone`，以及 4 个 `TimedResourcePool`（收货、存储、包装、发货终端）。  
   - 创建 `loaderCount` 个 `Loader` 对象并启动 `std::thread`。  
   - 创建 `managerCount` 个 `Manager` 对象并启动 `std::thread`。  
   - `running_` 标记为 `true`，让各线程在循环中继续工作。

3. **经理线程 `Manager::run` 的循环**  
   每个经理线程在循环中做三类事（带节流策略）：
   - `scheduleTrucks()`：  
     - 随机生成一辆 `Truck`（10–100 托盘），每个托盘含不同类型/品类的货物。  
     - 把它封装成一个“卸货任务”（`TaskKind::UnloadTruck`，最多 3 个 Loader 并行）。  
     - 调用 `warehouse.submitTask(task)` 放入调度队列。
   - `scheduleInventory()`：  
     - 生成若干品类的 `InventoryRequest`，创建盘点任务（`TaskKind::InventoryAudit`，单线程执行）。  
     - 同样交给 `TaskDispatcher`。
   - `scheduleOrders()`（有节流）：  
     - 前半段仿真时间内，且队列压力未超阈值时才生成订单：  
       - 随机目的城市 + 若干 `(类型,品类)` 数量构成 `Order`。  
       - 登记状态(`waiting` → `queued`)并生成“拣配任务”（`TaskKind::Picking`），并限制 `maxParallelLoaders`。  
   - 此外周期性调用 `shippingZone().registerTruckArrival(...)` 模拟发货卡车到港。

4. **搬运工线程 `Loader::run` 的循环**  
   每个 Loader 做的是典型的“工作线程”模型：
   - 记录空闲开始时间，调用 `dispatcher().acquireTask()` 阻塞等待任务。  
   - 取到任务后，记录这段“休息时间”，写入统计。  
   - 根据 `task->kind` 分派到具体处理函数：
     - `handleUnload`：卸货四步（取托盘 → 收货终端登记 → 搬运到存储区 → 存储终端登记）。  
     - `handleInventory`：占用存储终端，对给定品类做盘点查询。  
     - `handlePicking`：预订拣配工作台 → 从存储区扣货 → 尝试装车 → 更新订单状态、记录耗时。  
   - 若任务“成功”，调用 `recordTaskCompletion` 更新搬运工绩效，并把 `task->completed = true`。  
   - 调用 `dispatcher().finishTask(task)`，减少活动计数、必要时从队列移除。

---

### 二、用到的同步原语及用途

1. **`std::mutex` + `std::lock_guard` / `std::unique_lock`**  
   - 用途：保护共享数据结构，保证同一时刻只有一个线程修改。  
   - 典型位置：  
     - `TaskDispatcher::tasks_` 队列：`enqueue` / `acquireTask` / `finishTask`。  
     - `StorageZone::records_` / `freeAddresses_` 写操作。  
     - `Warehouse` 中统计结构（`loaderStats_`, `orderStates_`, `orderStateMap_`）。

2. **`std::condition_variable`**  
   - 用途：线程间等待事件并被唤醒（避免 busy-wait）。  
   - 典型位置：  
     - `TaskDispatcher::acquireTask`：搬运工在没有可执行任务时等待，任务入队或状态变化后被唤醒。  
     - `TimedResourcePool::acquire_for`：在有资源可用前阻塞等待，最多等到 timeout。  
     - `PackingZone::reserveWorkstation`：等待有空闲的拣配工作台。  
     - `ShippingZone::tryLoad`：等待满足城市/容量要求的卡车到达。

3. **`std::shared_mutex` + `std::shared_lock`**  
   - 用途：读多写少场景，允许多个线程同时读，但写时独占。  
   - 典型位置：  
     - `StorageZone`：  
       - `placePallet` / `takeFromStorage` 用写锁。  
       - `recordsForCategory` / `totalsByCategory` 用读锁。

4. **`std::atomic`**  
   - 用途：无锁地更新简单整数/布尔值，避免竞态。  
   - 典型位置：  
     - `Task::activeLoaders`：当前参与此任务的 Loader 数量，用于控制并行度。  
     - `Task::completed`：任务是否已成功完成。  
     - `Task::promiseFulfilled`：标记 `promise` 是否已经 `set_value()`，避免多个线程重复触发。  
     - `Warehouse::running_`、`nextTaskId_`、`nextOrderId_`、`nextTruckId_`。

5. **`std::thread`**  
   - 用途：直接创建 OS 级工作线程。  
   - 典型位置：  
     - 每个 `Loader`、`Manager` 内部都有一个 `std::thread`，在 `start()` 中启动，`join()` 中等待结束。

---

### 三、基于线程的任务模型（thread-based model）

整体上是一个“线程池 + 任务队列”的手写版本，不过：  
- 不是传统意义上固定几个 worker，而是：  
  - `loaderCount` 个 **Loader 工作线程**（搬运工）。  
  - `managerCount` 个 **Manager 生产线程**（经理）。  
- 所有业务逻辑都围绕一个共享的 `TaskDispatcher`：

**流程：**
1. 经理线程负责：  
   - 生成 `Task`（卸货 / 盘点 / 拣配），填充 `payload`（`UnloadPayload` / `InventoryRequest` / `PickingPayload`）。  
   - 设置 `maxParallelLoaders` 控制可参与此任务的最大搬运工数量。  
   - 把任务交给 `Warehouse::submitTask` → `TaskDispatcher::enqueue`。

2. 搬运工线程负责：  
   - 不断向 `TaskDispatcher` 请求任务：`acquireTask()`。  
   - `TaskDispatcher` 内部用原子计数 + 遍历队列寻找“当前未满并发度的任务”，并 `activeLoaders++` 后交给 Loader。  
   - 搬运工执行具体动作（解包任务 `payload`），完成后调用 `finishTask()` 把自己从活跃计数中减掉。  
   - 当一个任务被标记 `completed` 且 `activeLoaders == 0` 时，从队列中彻底移除。

这完全是基于 `std::thread` 和手工同步原语构建的**线程模型**，展示了典型的多消费者（Loaders） / 多生产者（Managers）场景。

---

### 四、`future + promise` 多线程模型在本项目中的使用

题目要求中提到的第三点是：**熟悉基于 `future + promise` 对的多线程模型**。项目的实现方式是：

1. **在 `Task` 里嵌入 `promise/future`**（`include/TaskDispatcher.h`）  
   - 每个 `Task` 包含：
     - `std::promise<void> completionPromise;`
     - `std::shared_future<void> completionFuture;`
     - `std::atomic<bool> promiseFulfilled{false};`
   - 在 `TaskDispatcher::enqueue` 时，如果 `completionFuture` 还没初始化，就：  
     - 调用 `task->completionPromise.get_future().share()` 得到 `shared_future`，赋给 `completionFuture`。  
   - 这样，每个任务就自带一个“完成通知”的 `future`。

2. **任务完成时触发 `promise`**  
   - 在 `Loader::run` 中，任务执行成功后：  
     - 把 `task->completed = true`。  
     - 如果 `promiseFulfilled` 还没被设置，则调用 `completionPromise.set_value()`。  
   - 在 `TaskDispatcher::finishTask` 中，如果检查到任务已经 `completed` 且 `promise` 还未触发，也会做同样的事：  
     - 保证无论哪个线程先看到“任务已经完成”，都只会 `set_value()` 一次。

3. **对外暴露 `future`**（`Warehouse::submitTask`）  
   - `Warehouse::submitTask` 接收一个 `shared_ptr<Task>`，调用 `dispatcher.enqueue(task)` 后，返回 `task->completionFuture`。  
   - 调用者（理论上可以是某个上层管理线程或测试代码）可以：  
     ```cpp
     auto fut = warehouse.submitTask(task);
     // ... 做一些别的事情 ...
     fut.wait(); // 阻塞等待该任务完成
     // 或 fut.wait_for(std::chrono::seconds(2));
     ```
   - 因为是 `shared_future`，多个地方都可以同时等待同一个任务，无需担心“未来值被取走一次就失效”。

4. **意义与用法**  
   - 从**模型**角度，你看的是完整的“生产者（Manager）→ 任务队列（TaskDispatcher）→ 消费者（Loader）”流程，同时还具备“**对任务完成的统一观察点**”：  
     - 不用轮询任务状态，只需要等待 future。  
     - 这体现了 `future/promise` 在多线程协调中的经典用途：**单次事件的点对点/一对多通知**。
   - 在当前控制台版本中，我们主要还是靠报告来展示结果，future 更偏向“为扩展准备的接口”：  
     - 例如将来如果你写一个 GUI 或网络层，可以在提交任务后拿到 future，界面上显示“进行中…”，等 `fut.wait()` 之后变成“已完成”。

---

### 小结

- **运行流程**：`main` 启动 `Warehouse` → `Warehouse` 创建区块和线程 → `Manager` 不断生成任务 → `Loader` 从 `TaskDispatcher` 消费任务 → 所有动作在不同资源区上执行 → 仿真结束后汇总报告。
- **同步原语**：  
  - `mutex` / `condition_variable` 解决互斥和等待/唤醒；  
  - `shared_mutex` 提供高效读写并发；  
  - `atomic` 管理计数和状态；  
  - `thread` 明确每个角色的执行上下文。
- **多线程模型**：  
  - 基于线程的生产者/消费者 + 任务队列；  
  - `future + promise` 为每个任务提供一个完成事件，让外部可以优雅地等待，而不必主动查询。

