#include "regex_engine.hpp"

#include <iostream>
#include <string>

// 协议 / Протокол:
// 1) 第一行: 正则表达式模式 (pattern)。
//    Первая строка: шаблон регулярного выражения.
// 2) 后续每一行: 要检查的字符串。
//    Все последующие строки: проверяемые строки.
// 3) 对每个字符串输出一行 "true" 或 "false"。
//    Для каждой строки выводится "true" или "false".
int main() {
    std::ios::sync_with_stdio(false); // 加速 C++ I/O / ускорение ввода-вывода

    std::string pattern;
    if (!std::getline(std::cin, pattern)) {
        // 如果连第一行模式都读不到，直接退出。
        // Если не удалось прочитать шаблон — просто выходим.
        return 0;
    }

    std::string line;
    while (std::getline(std::cin, line)) {
        bool ok = false;
        try {
            // 使用我们的引擎检查整串是否匹配模式。
            // Используем движок, чтобы проверить полное совпадение строки с шаблоном.
            ok = regex_engine::full_match(pattern, line);
        } catch (const std::exception&) {
            // 如果模式非法或匹配过程中抛出异常，则统一视为 false。
            // Если шаблон некорректен или произошла ошибка — считаем результат ложным.
            ok = false;
        }
        std::cout << (ok ? "true" : "false") << '\n';
    }

    return 0;
}

