#include "TaskDispatcher.h"

#include <algorithm>
#include <thread>

// 保护共享数据结构，保证同一时刻只有一个线程修改
// Добавить новую задачу в очередь и разбудить ожидающий поток-грузчик
void TaskDispatcher::enqueue(const std::shared_ptr<Task>& task) {
    {
        std::lock_guard lk(mutex_);
        if (!task->completionFuture.valid()) {
            task->completionFuture = task->completionPromise.get_future().share();
        }
        tasks_.push_back(task);
    }
    cv_.notify_one();
}

// 线程间等待事件并被唤醒
// Блокирующее получение подходящей задачи потоком-грузчиком
std::shared_ptr<Task> TaskDispatcher::acquireTask() {
    std::unique_lock lk(mutex_);
    auto task = acquireUnlocked();
    while (!task && !stopping_) {
        cv_.wait(lk);
        task = acquireUnlocked();
    }
    if (stopping_) {
        return nullptr;
    }
    return task;
}

// 搬运工在没有可执行任务时等待，任务入队或状态变化后被唤醒
// Просмотр очереди в поиске задачи, где не достигнут лимит параллельных исполнителей
std::shared_ptr<Task> TaskDispatcher::acquireUnlocked() {
    for (auto& task : tasks_) {
        if (!task || task->completed.load()) {
            continue;
        }
        const int active = task->activeLoaders.load();
        if (active < task->maxParallelLoaders) {
            task->activeLoaders.fetch_add(1);
            return task;
        }
    }
    return nullptr;
}

// 任务执行完减少计数，必要时移除队列
// После выполнения задачи уменьшить счетчик и при необходимости удалить задачу из очереди
void TaskDispatcher::finishTask(const std::shared_ptr<Task>& task) {
    if (!task) {
        return;
    }

    {
        std::lock_guard lk(mutex_);
        task->activeLoaders.fetch_sub(1);
        if (task->completed.load() && !task->promiseFulfilled.exchange(true)) {
            task->completionPromise.set_value();
        }
        if (task->completed.load() && task->activeLoaders.load() == 0) {
            tasks_.erase(std::remove(tasks_.begin(), tasks_.end(), task), tasks_.end());
        }
    }
    cv_.notify_all();
}

// 通知所有等待线程准备退出
// Сообщить всем ожидающим потокам о завершении работы диспетчера
void TaskDispatcher::shutdown() {
    {
        std::lock_guard lk(mutex_);
        stopping_ = true;
    }
    cv_.notify_all();
}

size_t TaskDispatcher::size() const {
    std::lock_guard lk(mutex_);
    return tasks_.size();
}

