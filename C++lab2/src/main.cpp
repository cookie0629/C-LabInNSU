/**
 * @file main.cpp
 * @brief 程序入口文件 / Файл точки входа программы
 * 
 * 程序入口：负责构造 BankSystem、读取初始数据并进入事件循环。
 * 题目要求所有异常都需视为失败，因此这里捕获 std::exception 并报告到 stderr。
 * 
 * Точка входа программы: отвечает за создание BankSystem, чтение начальных данных
 * и вход в цикл обработки событий. Задача требует, чтобы все исключения считались
 * ошибкой, поэтому здесь перехватывается std::exception и выводится в stderr.
 */

#include "Bank.hpp"

#include <cstdlib>
#include <exception>
#include <iostream>

/**
 * @brief 主函数 / Главная функция
 * 
 * 程序执行流程：
 * 1. 创建 BankSystem 实例
 * 2. 调用 loadInitialData() 读取所有初始数据
 * 3. 调用 run() 进入事件循环，处理所有事件直到输入结束
 * 
 * Порядок выполнения программы:
 * 1. Создание экземпляра BankSystem
 * 2. Вызов loadInitialData() для чтения всех начальных данных
 * 3. Вызов run() для входа в цикл обработки событий до конца ввода
 * 
 * @return EXIT_SUCCESS 成功 / при успехе, EXIT_FAILURE 失败 / при ошибке
 */
int main() {
    try {
        bank::BankSystem bankSystem;        // 创建银行系统实例 / Создание экземпляра банковской системы
        bankSystem.loadInitialData();       // 加载初始数据 / Загрузка начальных данных
        bankSystem.run();                    // 运行事件循环 / Запуск цикла обработки событий
    } catch (const std::exception &ex) {
        // 捕获所有标准异常并输出到 stderr / Перехват всех стандартных исключений и вывод в stderr
        std::fprintf(stderr, "Unhandled exception: %s\n", ex.what());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

