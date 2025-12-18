#pragma once

#include <memory>   // std::shared_ptr 用于安全管理动态内存 / управление динамической памятью
#include <string>
#include <vector>

namespace regex_engine {

// 基础 AST 节点：任何可以在字符串上尝试匹配的节点。
// Базовый узел AST: любой объект, который умеет сопоставлять часть строки.
class Node {
public:
    virtual ~Node() = default;
    virtual bool match(const std::string& text, std::size_t pos,
                       std::size_t& outPos) const = 0;
};

// 管理 AST 节点的生命周期 !
using NodePtr = std::shared_ptr<Node>;

// 原子节点：一个基本单元（字面量字符 / '.' / 字符组），可以有量词 *, + 或 ?。
// Узел-атом: базовый элемент (литерал, '.', или класс символов) с квантификатором *, +, ?.
class Atom : public Node {
public:
    enum class Kind { Literal, Any, CharClass };

    struct Quantifier {
        // min <= occurrences <= max (max == npos => infinity)
        // 出现次数区间；max=(size_t)-1 表示“任意多次”。
        // диапазон количества повторений; max=(size_t)-1 означает «без верхнего предела».
        std::size_t min = 1;
        std::size_t max = 1;
        bool greedy = true; // true — жадный, false — ленивый.
    };

    struct CharClassRange {
        char from; // 起始字符 / начальный символ
        char to;   // 结束字符 / конечный символ
    };

    // 构造字面量或 '.' 的原子。
    // Конструктор атома для литерала или точки '.'.
    Atom(Kind kind, char literal, Quantifier q);
    // 构造字符组 [ ... ] 的原子。
    // Конструктор атома для класса символов [ ... ].
    Atom(const std::vector<CharClassRange>& ranges, Quantifier q);

    bool match(const std::string& text, std::size_t pos,
               std::size_t& outPos) const override;

private:
    Kind kind_;
    char literal_{};
    std::vector<CharClassRange> ranges_;
    Quantifier quant_;

    // 判断单个字符是否能被该原子匹配。
    // Проверяет, подходит ли один символ этому атому.
    bool matchesSingle(char c) const;
};

// 序列节点：包含多个子节点，要求按顺序全部成功匹配。
// Узел-последовательность: несколько дочерних узлов, которые должны
// совпасть друг за другом.
class Sequence : public Node {
public:
    // 往序列末尾添加一个节点。
    // Добавить узел в конец последовательности.
    void add(NodePtr node);

    bool match(const std::string& text, std::size_t pos,
               std::size_t& outPos) const override;

private:
    std::vector<NodePtr> nodes_;
};

// 解析器：把模式字符串解析成 AST 结构。
// Парсер: преобразует строку-шаблон в структуру AST.
class Parser {
public:
    explicit Parser(const std::string& pattern);

    // 解析整个表达式。若模式非法，抛出 std::runtime_error。
    // Разобрать полное выражение. При ошибке бросает std::runtime_error.
    NodePtr parse();

private:
    std::string pattern_;
    std::size_t pos_{0};

    // 是否到达模式字符串结尾。
    // Достигнут ли конец строки шаблона.
    bool eof() const;
    // 查看当前字符但不移动位置。
    // Посмотреть текущий символ без сдвига.
    char peek() const;
    // 读取当前字符并向前移动。
    // Считать символ и увеличить позицию.
    char get();

    // 解析一个由若干原子组成的序列。
    // Разобрать последовательность атомов.
    NodePtr parseSequence();
    // 解析一个原子及其量词。
    // Разобрать один атом и его квантификатор.
    NodePtr parseAtom();
    // 解析量词符号 (*, +, ?)（如果存在）。
    // Разобрать квантификатор (*, +, ?), если он есть.
    Atom::Quantifier parseQuantifier();
    // 解析字符组 [ ... ]。
    // Разобрать класс символов [ ... ].
    std::vector<Atom::CharClassRange> parseCharClass();
};

// 公共 API：编译模式字符串。
// Публичный API: компиляция шаблона.
NodePtr compile(const std::string& pattern);
// 公共 API：检查 text 是否“整体匹配”模式。
// Публичный API: проверить, полностью ли строка text соответствует шаблону.
bool full_match(const std::string& pattern, const std::string& text);

}


