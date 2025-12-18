#include "regex_engine.hpp"

#include <stdexcept> 

namespace regex_engine {

// ===== Atom =====
// Atom 类实现了对“一个模式单元（+量词）”的匹配逻辑。
// Класс Atom реализует сопоставление одного элемента шаблона с квантификатором.

Atom::Atom(Kind kind, char literal, Quantifier q)
    : kind_(kind), literal_(literal), quant_(q) {}

Atom::Atom(const std::vector<CharClassRange>& ranges, Quantifier q)
    : kind_(Kind::CharClass), ranges_(ranges), quant_(q) {}

// 判断单个字符是否与当前原子匹配（不考虑量词）。
// Проверяет, подходит ли один символ этому атому (без учёта квантификатора).
bool Atom::matchesSingle(char c) const {
    switch (kind_) {
    case Kind::Any:
        return true;
    case Kind::Literal:
        return c == literal_;
    case Kind::CharClass:
        for (const auto& r : ranges_) {
            if (c >= r.from && c <= r.to) {
                return true;
            }
        }
        return false;
    }
    return false;
}

// 根据量词进行匹配：从 text[pos] 开始，尽量匹配该原子多次。
// С учётом квантификатора пытается многократно сопоставить атом,
// начиная с text[pos].
bool Atom::match(const std::string& text, std::size_t pos,
                 std::size_t& outPos) const {
    std::size_t count = 0;
    std::size_t i = pos;

    // First, match as many as possible up to max
    while (i < text.size() && count < quant_.max &&
           matchesSingle(text[i])) {
        ++count;
        ++i;
    }

    if (count < quant_.min) {
        return false;
    }

    // 此处的实现只根据自身量词做“局部决定”：
    // - 对于 * / +（贪婪）：尽量吃多。
    // - 对于 ?（懒惰）：尽量吃少（min）。
    //
    // Реализация принимает локальное решение:
    // - для * / + (жадные) берём максимум;
    // - для ? (ленивый) берём минимум.
    if (!quant_.greedy) {
        // lazy: use the minimal allowed count
        outPos = pos + std::min(count, quant_.min);
        return true;
    }

    // greedy: use maximal count we have
    outPos = i;
    return true;
}

// ===== Sequence =====
// Sequence 依次尝试匹配所有子节点。
// Sequence по очереди сопоставляет все дочерние узлы.

void Sequence::add(NodePtr node) {
    nodes_.push_back(std::move(node));
}

bool Sequence::match(const std::string& text, std::size_t pos,
                     std::size_t& outPos) const {
    std::size_t current = pos;
    for (const auto& node : nodes_) {
        std::size_t nextPos = current;
        if (!node->match(text, current, nextPos)) {
            return false;
        }
        current = nextPos;
    }
    outPos = current;
    return true;
}

// ===== Parser =====
// Parser 负责把模式字符串分解为 AST 结构。
// Parser отвечает за разбор строки шаблона в структуру AST.

Parser::Parser(const std::string& pattern)
    : pattern_(pattern) {}

bool Parser::eof() const {
    return pos_ >= pattern_.size();
}

char Parser::peek() const {
    if (eof()) {
        return '\0';
    }
    return pattern_[pos_];
}

char Parser::get() {
    if (eof()) {
        throw std::runtime_error("Unexpected end of pattern");
    }
    return pattern_[pos_++];
}

NodePtr Parser::parse() {
    // 当前语法不支持“或”等高级结构，因此整个正则看成一个顺序序列。
    // В упрощённом синтаксисе всё выражение — одна последовательность без «или».
    return parseSequence();
}

NodePtr Parser::parseSequence() {
    //通过工厂式 `std::make_shared<...>` 创建节点，而不是裸 `new`，体现现代 C++ RAII 风格
    auto seq = std::make_shared<Sequence>();
    while (!eof()) {
        seq->add(parseAtom());
    }
    return seq;
}

Atom::Quantifier Parser::parseQuantifier() {
    Atom::Quantifier q;
    q.min = 1;
    q.max = 1;
    q.greedy = true;

    if (eof()) {
        return q;
    }

    char c = peek();
    if (c == '*') {
        get();
        q.min = 0;
        q.max = static_cast<std::size_t>(-1);
        q.greedy = true; // greedy
    } else if (c == '+') {
        get();
        q.min = 1;
        q.max = static_cast<std::size_t>(-1);
        q.greedy = true; // greedy
    } else if (c == '?') {
        get();
        q.min = 0;
        q.max = 1;
        q.greedy = false; // lazy
    }

    return q;
}

std::vector<Atom::CharClassRange> Parser::parseCharClass() {
    std::vector<Atom::CharClassRange> ranges;

    if (get() != '[') { // consume '['
        throw std::runtime_error("Expected '['");
    }

    char prev = '\0';
    bool hasPrev = false;

    while (!eof()) {
        char c = get();
        if (c == ']') {
            break;
        }

        if (c == '-' && hasPrev && !eof() && peek() != ']') {
            // 处理范围 prev-next，例如 a-d。
            // Обработка диапазона prev-next, например a-d.
            char to = get();
            if (prev > to) {
                throw std::runtime_error("Invalid range in char class");
            }
            ranges.push_back({prev, to});
            hasPrev = false;
        } else {
            // 单个字符。
            // Одиночный символ.
            if (hasPrev) {
                ranges.push_back({prev, prev});
            }
            prev = c;
            hasPrev = true;
        }
    }

    if (hasPrev) {
        ranges.push_back({prev, prev});
    }

    if (ranges.empty()) {
        throw std::runtime_error("Empty character class");
    }

    return ranges;
}

// 解析一个“原子 + 其后的量词”。
// Разобрать один «атом + квантификатор».
NodePtr Parser::parseAtom() {
    if (eof()) {
        throw std::runtime_error("Unexpected end of pattern in atom");
    }

    char c = peek();
    Atom::Quantifier q;

    if (c == '.') {
        get();
        q = parseQuantifier();
        return std::make_shared<Atom>(Atom::Kind::Any, '\0', q);
    }

    if (c == '[') {
        auto ranges = parseCharClass();
        q = parseQuantifier();
        return std::make_shared<Atom>(ranges, q);
    }

    // literal 普通字面字符。
    // обычный литерал.
    get();
    q = parseQuantifier();
    return std::make_shared<Atom>(Atom::Kind::Literal, c, q);
}

// ===== Public API =====
// 公共接口：从模式字符串构造 AST。
// Публичный интерфейс: построить AST из строки шаблона.
NodePtr compile(const std::string& pattern) {
    Parser p(pattern);
    return p.parse();
}

bool full_match(const std::string& pattern, const std::string& text) {
    // 先编译模式，再从位置 0 开始整串匹配。
    // Сначала компилируем шаблон, затем пытаемся сопоставить строку с нуля.
    NodePtr root = compile(pattern);
    std::size_t pos = 0;
    if (!root->match(text, 0, pos)) {
        return false;
    }
    // 只有当消费到字符串末尾才算“完全匹配”。
    // Строка считается подходящей, только если была потреблена целиком.
    return pos == text.size();
}

}
