#include "Domain.hpp"

#include <cassert>
#include <cmath>

// 题目对金额处理有严格的四舍五入公式。
// 该测试验证 Money::round3 与 parseMoneyParts 的边界行为。
int main() {
    using domain::Money;

    const double roundedPositive = Money::round3(999999.0004);
    assert(std::abs(roundedPositive - 999999.0) < 1e-9);

    const double roundedNegative = Money::round3(-999999.0005);
    assert(std::abs(roundedNegative + 999999.001) < 1e-9);

    const double fromParts = domain::parseMoneyParts(1999, 999);
    assert(std::abs(fromParts - 1999.999) < 1e-9);

    return 0;
}

