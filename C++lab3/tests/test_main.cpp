#include "regex_engine.hpp"

#include <iostream>
#include <string>

using regex_engine::full_match;

// 简单的“手写测试框架”：用 main 返回值表示测试是否通过。
// Простой «ручной» тестовый фреймворк: main возвращает код успеха/ошибки.
int main() {
    struct Case {
        std::string pattern; // 正则模式 / шаблон
        std::string text;    // 待匹配文本 / строка
        bool expected;       // 期望结果 / ожидаемый результат
    };

    // 一组覆盖基础语法的测试用例。
    // Набор тестов, покрывающих базовый синтаксис.
    const Case cases[] = {
        {"a", "a", true},
        {"a", "b", false},
        {".", "x", true},
        {".", "", false},
        {"[abc]", "a", true},
        {"[a-c]", "b", true},
        {"[a-c]", "d", false},
        {"a*", "", true},
        {"a*", "aaaa", true},
        {"a+", "", false},
        {"a+", "aaaa", true},
        {"[ab]*", "abba", true},
        {"[ab]+", "", false},
        // Greedy * 示例 / пример: "aa" 不满足 / не удовлетворяет "a*a"
        {"a*a", "aa", false},
        // Greedy + 示例 / пример: "aa" 不满足 / не удовлетворяет "a+a"
        {"a+a", "aa", false},
        // Lazy ? 示例 / пример: "a" 满足 / удовлетворяет "a?a"
        {"a?a", "a", true},
        // 在当前引擎中 "aa" 不满足 "a?a" / в текущем движке "aa" не соответствует "a?a"
        {"a?a", "aa", false},
        {"a?a", "", false},
        // 组合表达式 / составное выражение
        {"a.[b-d]+c?", "axbcc", true},
        // "axb" 匹配: 'a' + 'x' + 'b' + 空的 c? /
        // "axb" сопоставляется как 'a' + 'x' + 'b' + пустой c?
        {"a.[b-d]+c?", "axb", true},
    };

    int failed = 0;
    int total = sizeof(cases) / sizeof(cases[0]);

    for (const auto& c : cases) {
        bool got = false;
        try {
            got = full_match(c.pattern, c.text);
        } catch (const std::exception& e) {
            std::cerr << "Exception for pattern '" << c.pattern
                      << "': " << e.what() << '\n';
            got = false;
        }
        if (got != c.expected) {
            ++failed;
            std::cerr << "FAIL pattern='" << c.pattern
                      << "' text='" << c.text
                      << "' expected=" << (c.expected ? "true" : "false")
                      << " got=" << (got ? "true" : "false") << '\n';
        }
    }

    if (failed == 0) {
        std::cout << "All " << total << " tests passed.\n";
        return 0;
    }

    std::cerr << failed << " of " << total << " tests failed.\n";
    return 1;
}


