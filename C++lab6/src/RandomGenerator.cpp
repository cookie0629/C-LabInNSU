#include "RandomGenerator.h"

#include <random>

namespace {
std::random_device& device() {
    static std::random_device rd{};
    return rd;
}
} // namespace

std::mt19937 RandomGenerator::getGenerator() {
    return std::mt19937{device()()};
}

int RandomGenerator::getRandom(int min, int max) {
    auto gen = getGenerator();
    return getDist(min, max)(gen);
}

int RandomGenerator::getRandom(std::mt19937& gen, int min, int max) {
    return getDist(min, max)(gen);
}

std::uniform_int_distribution<> RandomGenerator::getDist(int min, int max) {
    return std::uniform_int_distribution<>(min, max);
}

