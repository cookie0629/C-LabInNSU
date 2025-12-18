#pragma once

#include <random>

// 线程安全的随机数工具，方便统一切换调度策略
//Потокобезопасный инструмент для обработки случайных чисел, 
// облегчающий унифицированное переключение стратегий планирования
class RandomGenerator {
public:
    static std::mt19937 getGenerator();
    static int getRandom(int min, int max);
    static int getRandom(std::mt19937& gen, int min, int max);
    static std::uniform_int_distribution<> getDist(int min, int max);
};

