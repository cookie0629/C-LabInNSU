#pragma once

/**
 * @file Domain.hpp
 * @brief 领域模型定义文件 / Файл определения доменной модели
 * 
 * 本文件集中定义题目描述中的所有"领域数据类型"，以便 BankSystem
 * 能够用统一的数据结构保存输入和运行时状态。
 * 
 * Данный файл централизованно определяет все "типы данных доменной модели"
 * из описания задачи, чтобы BankSystem мог использовать единую структуру
 * данных для сохранения входных данных и состояния выполнения.
 */

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace domain {

/**
 * @brief 最大记录数限制 / Максимальное количество записей
 * 
 * 题目要求：所有表格数据以固定容量的 C 风格数组保存，这里给出统一限制。
 * Требование задачи: все табличные данные сохраняются в массивах C-стиля
 * фиксированной ёмкости, здесь указано единое ограничение.
 */
constexpr std::size_t kMaxRecords = 100'000;

/**
 * @brief 输入字符串最大长度 / Максимальная длина входной строки
 * 
 * 输入字符串最大长度，方便 scanf 读入。
 * Максимальная длина входной строки для удобного чтения через scanf.
 */
constexpr std::size_t kMaxString = 100;

/**
 * @brief 日期时间结构 / Структура даты и времени
 * 
 * 表示模拟中的日期和时间点。
 * day: 从模拟开始算起的天数序号（无符号整数）
 * hour: 一天中的小时（24小时制，0-23）
 * minute: 当前小时的分钟（0-59）
 * 
 * Представляет дату и момент времени в симуляции.
 * day: порядковый номер дня с начала симуляции (беззнаковое целое)
 * hour: час дня (24-часовой формат, 0-23)
 * minute: минута текущего часа (0-59)
 */
struct DateTime {
    unsigned long long day{};      ///< 天数序号 / Порядковый номер дня
    unsigned long long hour{};     ///< 小时 / Час
    unsigned long long minute{};    ///< 分钟 / Минута
};

inline bool operator<(const DateTime &lhs, const DateTime &rhs) {
    if (lhs.day != rhs.day) return lhs.day < rhs.day;
    if (lhs.hour != rhs.hour) return lhs.hour < rhs.hour;
    return lhs.minute < rhs.minute;
}

inline bool operator==(const DateTime &lhs, const DateTime &rhs) {
    return lhs.day == rhs.day && lhs.hour == rhs.hour && lhs.minute == rhs.minute;
}

/**
 * @brief 账户类型枚举 / Перечисление типов счетов
 * 
 * Debit: 借记账户 / Дебетовый счёт
 * Deposit: 存款账户 / Депозитный счёт
 * Credit: 贷款账户 / Кредитный счёт
 */
enum class AccountKind {
    Debit,      ///< 借记账户 / Дебетовый счёт
    Deposit,    ///< 存款账户 / Депозитный счёт
    Credit      ///< 贷款账户 / Кредитный счёт
};

/**
 * @brief 货币类型枚举 / Перечисление валют
 * 
 * 支持的货币：RUB（卢布）、YUAN（人民币）、USD（美元）、EUR（欧元）
 * Поддерживаемые валюты: RUB (рубль), YUAN (юань), USD (доллар), EUR (евро)
 */
enum class Currency {
    RUB,        ///< 卢布 / Рубль
    YUAN,       ///< 人民币 / Юань
    USD,        ///< 美元 / Доллар США
    EUR,        ///< 欧元 / Евро
    Unknown     ///< 未知货币 / Неизвестная валюта
};

/**
 * @brief 客户类型枚举 / Перечисление типов клиентов
 * 
 * Individual: 个人客户 / Физическое лицо
 * VipIndividual: 优先个人客户 / VIP физическое лицо
 * Legal: 法人实体 / Юридическое лицо
 * VipLegal: 优先法人实体 / VIP юридическое лицо
 * NotClient: 非客户 / Не клиент
 */
enum class CustomerKind {
    Individual,     ///< 个人客户 / Физическое лицо
    VipIndividual,  ///< 优先个人客户 / VIP физическое лицо
    Legal,          ///< 法人实体 / Юридическое лицо
    VipLegal,       ///< 优先法人实体 / VIP юридическое лицо
    NotClient       ///< 非客户 / Не клиент
};

/**
 * @brief 工作岗位类型枚举 / Перечисление типов рабочих мест
 * 
 * ClientManager: 客户服务窗口 / Окно обслуживания клиентов
 * CashDesk: 现金柜台 / Касса
 * CurrencyExchange: 货币兑换点 / Пункт обмена валют
 * VipManager: 优先客户经理 / Менеджер VIP-клиентов
 */
enum class WorkplaceKind {
    ClientManager,     ///< 客户服务窗口 / Окно обслуживания клиентов
    CashDesk,           ///< 现金柜台 / Касса
    CurrencyExchange,   ///< 货币兑换点 / Пункт обмена валют
    VipManager,         ///< 优先客户经理 / Менеджер VIP-клиентов
    Unknown             ///< 未知类型 / Неизвестный тип
};

/**
 * @brief 银行账户结构 / Структура банковского счёта
 * 
 * 题目要求记录账号、类型、余额、币种与活动状态。
 * id: 唯一账号（无符号整数）
 * type: 账户类型（字符串："debit", "deposit", "credit"）
 * balance: 账户资金量（货币单位，double类型，需四舍五入到小数点后3位）
 * currency: 账户货币（字符串："RUB", "YUAN", "USD", "EUR"）
 * active: 账户是否处于活动状态（true表示账户有效，false表示已关闭）
 * 
 * Требование задачи: запись номера счёта, типа, баланса, валюты и статуса активности.
 * id: уникальный номер счёта (беззнаковое целое)
 * type: тип счёта (строка: "debit", "deposit", "credit")
 * balance: сумма средств на счёте (денежная единица, тип double, округление до 3 знаков)
 * currency: валюта счёта (строка: "RUB", "YUAN", "USD", "EUR")
 * active: активен ли счёт (true - счёт действителен, false - закрыт)
 */
struct Account {
    unsigned long long id{};        ///< 唯一账号 / Уникальный номер счёта
    std::string type;               ///< 账户类型 / Тип счёта
    double balance{};               ///< 账户余额 / Баланс счёта
    std::string currency;           ///< 账户货币 / Валюта счёта
    bool active{true};              ///< 是否活动 / Активен ли счёт
};

/**
 * @brief 存款结构 / Структура депозита
 * 
 * 存款、贷款、客户等结构均严格对应题目中的表结构，不做额外精简。
 * id: 唯一存款编号
 * rate: 利率（浮点数，以百分比表示，例如 6.320 表示年利率 6.32%）
 * type: 存款类型（字符串，如 "Compounded Daily Remaining"）
 * createdDay: 存款创建日期（从模拟开始算起的天数序号）
 * durationDays: 存款总期限（天数）
 * 
 * Структуры депозитов, кредитов, клиентов строго соответствуют таблицам из задачи.
 * id: уникальный номер депозита
 * rate: процентная ставка (число с плавающей точкой, в процентах, например 6.320 = 6.32% годовых)
 * type: тип депозита (строка, например "Compounded Daily Remaining")
 * createdDay: дата создания депозита (порядковый номер дня с начала симуляции)
 * durationDays: общий срок депозита (в днях)
 */
struct Deposit {
    unsigned long long id{};            ///< 唯一存款编号 / Уникальный номер депозита
    double rate{};                      ///< 利率 / Процентная ставка
    std::string type;                   ///< 存款类型 / Тип депозита
    unsigned long long createdDay{};    ///< 创建日期 / Дата создания
    unsigned long long durationDays{};  ///< 期限（天数）/ Срок (в днях)
};

/**
 * @brief 贷款结构 / Структура кредита
 * 
 * id: 唯一贷款编号
 * rate: 利率（浮点数，以百分比表示）
 * amount: 贷款金额（货币单位）
 * type: 贷款类型（字符串，如 "Charged Daily"）
 * 
 * id: уникальный номер кредита
 * rate: процентная ставка (число с плавающей точкой, в процентах)
 * amount: сумма кредита (денежная единица)
 * type: тип кредита (строка, например "Charged Daily")
 */
struct Loan {
    unsigned long long id{};    ///< 唯一贷款编号 / Уникальный номер кредита
    double rate{};              ///< 利率 / Процентная ставка
    double amount{};            ///< 贷款金额 / Сумма кредита
    std::string type;           ///< 贷款类型 / Тип кредита
};

/**
 * @brief 客户结构 / Структура клиента
 * 
 * 客户数据：包含唯一编号、姓名/组织名以及客户分类字符串。
 * id: 客户唯一编号
 * name: 姓名父名/组织名称（字符串，最多99个字符）
 * type: 客户类型（字符串："Individual Client", "VIP Individual Client" 等）
 * 
 * Данные клиента: уникальный номер, имя/название организации и строка классификации.
 * id: уникальный номер клиента
 * name: имя отчество/название организации (строка, максимум 99 символов)
 * type: тип клиента (строка: "Individual Client", "VIP Individual Client" и т.д.)
 */
struct Client {
    unsigned long long id{};    ///< 客户唯一编号 / Уникальный номер клиента
    std::string name;           ///< 姓名/组织名 / Имя/название организации
    std::string type;           ///< 客户类型 / Тип клиента
};

/**
 * @brief 客户账户关联结构 / Структура связи клиента и счёта
 * 
 * 客户与账户/存款/贷款之间的多对多关系。
 * clientId: 客户唯一编号
 * accountId: 唯一账号
 * depositId: 唯一存款编号（如果该账户关联存款，否则为0）
 * loanId: 唯一贷款编号（如果该账户关联贷款，否则为0）
 * isLoanAccount: 是否为贷款账户
 * isDepositAccount: 是否为存款账户
 * 
 * Связь многие-ко-многим между клиентами и счетами/депозитами/кредитами.
 * clientId: уникальный номер клиента
 * accountId: уникальный номер счёта
 * depositId: уникальный номер депозита (если счёт связан с депозитом, иначе 0)
 * loanId: уникальный номер кредита (если счёт связан с кредитом, иначе 0)
 * isLoanAccount: является ли счёт кредитным
 * isDepositAccount: является ли счёт депозитным
 */
struct ClientAccount {
    unsigned long long clientId{};      ///< 客户编号 / Номер клиента
    unsigned long long accountId{};      ///< 账户编号 / Номер счёта
    unsigned long long depositId{};      ///< 存款编号 / Номер депозита
    unsigned long long loanId{};        ///< 贷款编号 / Номер кредита
    bool isLoanAccount{false};          ///< 是否贷款账户 / Является ли кредитным счётом
    bool isDepositAccount{false};       ///< 是否存款账户 / Является ли депозитным счётом
};

/**
 * @brief 银行内部账户结构 / Структура внутреннего банковского счёта
 * 
 * accountId: 唯一账号（用于存储银行内部资金，如手续费、利息等）
 * accountId: уникальный номер счёта (для хранения внутренних средств банка, комиссий, процентов)
 */
struct BankAccount {
    unsigned long long accountId{};    ///< 账户编号 / Номер счёта
};

/**
 * @brief 客户债务结构 / Структура долга клиента
 * 
 * clientId: 客户唯一编号
 * accountId: 唯一账号
 * loanId: 唯一贷款编号
 * 
 * clientId: уникальный номер клиента
 * accountId: уникальный номер счёта
 * loanId: уникальный номер кредита
 */
struct ClientDebt {
    unsigned long long clientId{};  ///< 客户编号 / Номер клиента
    unsigned long long accountId{};  ///< 账户编号 / Номер счёта
    unsigned long long loanId{};     ///< 贷款编号 / Номер кредита
};

/**
 * @brief 工作岗位定义结构 / Структура определения рабочего места
 * 
 * type: 工作岗位类型（字符串："Client Manager", "Cash Desk" 等）
 * count: 分行中该岗位数量
 * 
 * type: тип рабочего места (строка: "Client Manager", "Cash Desk" и т.д.)
 * count: количество рабочих мест данного типа в отделении
 */
struct WorkplaceDefinition {
    std::string type;                   ///< 岗位类型 / Тип рабочего места
    unsigned long long count{};         ///< 数量 / Количество
};

/**
 * @brief 汇率结构 / Структура курса обмена валют
 * 
 * 汇率：客户出售/购买币种 + 兑换系数。
 * fromCurrency: 客户出售的货币
 * toCurrency: 客户购买的货币
 * ratio: 兑换系数（客户购买的货币与客户出售的货币之比）
 * 
 * Курс обмена: валюта продажи/покупки клиентом + коэффициент обмена.
 * fromCurrency: валюта, которую продаёт клиент
 * toCurrency: валюта, которую покупает клиент
 * ratio: коэффициент обмена (отношение покупаемой валюты к продаваемой)
 */
struct ExchangeRate {
    std::string fromCurrency;   ///< 出售货币 / Продаваемая валюта
    std::string toCurrency;     ///< 购买货币 / Покупаемая валюта
    double ratio{};             ///< 兑换系数 / Коэффициент обмена
};

/**
 * @brief 货币金额工具结构 / Вспомогательная структура для денежных сумм
 * 
 * Money 结构只是一个工具壳，用于集中处理题目要求的"三位小数四舍五入"逻辑。
 * 题目要求的四舍五入公式：(((long long int)(x * 10000) + 5 * (x < 0.0 ? -1 : 1)) / 10) / 1000.0
 * 示例：round(999999.0004) == 999999.000; round(-999999.0005) == -999999.001
 * 
 * Структура Money - это инструментальная оболочка для централизованной обработки
 * логики "округления до трёх знаков после запятой", требуемой задачей.
 * Формула округления из задачи: (((long long int)(x * 10000) + 5 * (x < 0.0 ? -1 : 1)) / 10) / 1000.0
 * Примеры: round(999999.0004) == 999999.000; round(-999999.0005) == -999999.001
 */
struct Money {
    double value{};     ///< 金额值 / Значение суммы

    /**
     * @brief 四舍五入到小数点后3位 / Округление до 3 знаков после запятой
     * 
     * @param x 待四舍五入的数值 / Значение для округления
     * @return 四舍五入后的结果 / Результат округления
     */
    static double round3(double x) {
        const auto sign = x < 0.0 ? -1.0 : 1.0;
        long long scaled = static_cast<long long>(x * 10000.0 + 5.0 * sign);
        return static_cast<double>(scaled / 10) / 1000.0;
    }

    /**
     * @brief 规范化金额值 / Нормализация значения суммы
     * 
     * 将 value 四舍五入到小数点后3位。
     * Округляет value до 3 знаков после запятой.
     */
    void normalize() { value = round3(value); }
};

/**
 * @brief 根据字符串解析货币类型 / Парсинг типа валюты из строки
 * 
 * @param text 货币字符串（"RUB", "YUAN", "USD", "EUR"）/ Строка валюты
 * @return 对应的货币枚举值 / Соответствующее значение перечисления валют
 */
inline Currency parseCurrency(std::string_view text) {
    if (text == "RUB") return Currency::RUB;
    if (text == "YUAN") return Currency::YUAN;
    if (text == "USD") return Currency::USD;
    if (text == "EUR") return Currency::EUR;
    return Currency::Unknown;
}

/**
 * @brief 将货币枚举转换回字符串 / Преобразование перечисления валют в строку
 * 
 * 主要用于输出或调试。
 * В основном для вывода или отладки.
 * 
 * @param c 货币枚举值 / Значение перечисления валют
 * @return 货币字符串 / Строка валюты
 */
inline const char *toString(Currency c) {
    switch (c) {
        case Currency::RUB: return "RUB";
        case Currency::YUAN: return "YUAN";
        case Currency::USD: return "USD";
        case Currency::EUR: return "EUR";
        default: return "UNKNOWN";
    }
}

/**
 * @brief 将完整的客户类型描述解析为枚举 / Парсинг полного описания типа клиента в перечисление
 * 
 * @param text 客户类型字符串 / Строка типа клиента
 * @return 对应的客户类型枚举值 / Соответствующее значение перечисления типа клиента
 */
inline CustomerKind parseCustomerKind(std::string_view text) {
    if (text == "Individual Client") return CustomerKind::Individual;
    if (text == "VIP Individual Client") return CustomerKind::VipIndividual;
    if (text == "Legal Entity") return CustomerKind::Legal;
    if (text == "VIP Legal Entity") return CustomerKind::VipLegal;
    if (text == "Not a Client") return CustomerKind::NotClient;
    return CustomerKind::NotClient;
}

/**
 * @brief 处理 Personal Appeal 输入中的简短类型标识 / Обработка краткого идентификатора типа из Personal Appeal
 * 
 * Personal Appeal 事件中使用简化的类型标识（"Individual" 或 "Legal Entity"），
 * 而不是完整的客户类型描述。
 * 
 * В событии Personal Appeal используется упрощённый идентификатор типа
 * ("Individual" или "Legal Entity"), а не полное описание типа клиента.
 * 
 * @param text 简短类型标识 / Краткий идентификатор типа
 * @return 对应的客户类型枚举值 / Соответствующее значение перечисления типа клиента
 */
inline CustomerKind parseClientToken(std::string_view text) {
    if (text == "Individual") return CustomerKind::Individual;
    if (text == "Legal Entity") return CustomerKind::Legal;
    return CustomerKind::NotClient;
}

/**
 * @brief 将客户类型枚举转换为字符串 / Преобразование перечисления типа клиента в строку
 * 
 * @param kind 客户类型枚举值 / Значение перечисления типа клиента
 * @return 客户类型字符串 / Строка типа клиента
 */
inline const char *toString(CustomerKind kind) {
    switch (kind) {
        case CustomerKind::Individual: return "Individual Client";
        case CustomerKind::VipIndividual: return "VIP Individual Client";
        case CustomerKind::Legal: return "Legal Entity";
        case CustomerKind::VipLegal: return "VIP Legal Entity";
        case CustomerKind::NotClient: default: return "Not a Client";
    }
}

/**
 * @brief 判断客户类型是否为VIP / Проверка, является ли тип клиента VIP
 * 
 * @param kind 客户类型枚举值 / Значение перечисления типа клиента
 * @return true 如果是VIP客户 / true если VIP-клиент
 */
inline bool isVip(CustomerKind kind) {
    return kind == CustomerKind::VipIndividual || kind == CustomerKind::VipLegal;
}

/**
 * @brief 解析金额的整数部分和小数部分 / Парсинг целой и дробной частей суммы
 * 
 * 题目给定金额格式：整数部分与三位小数分开提供，这里进行组合并四舍五入。
 * 例如：major=1999, minor=999 -> 1999.999
 * 
 * Формат суммы из задачи: целая часть и три знака после запятой предоставляются отдельно,
 * здесь они объединяются и округляются.
 * Например: major=1999, minor=999 -> 1999.999
 * 
 * @param major 整数部分 / Целая часть
 * @param minor 小数部分（0-999）/ Дробная часть (0-999)
 * @return 组合后的金额值（已四舍五入到3位小数）/ Объединённое значение суммы (округлено до 3 знаков)
 */
inline double parseMoneyParts(unsigned long long major, unsigned long long minor) {
    return Money::round3(static_cast<double>(major) + static_cast<double>(minor) / 1000.0);
}

/**
 * @brief 金额格式化结构 / Структура форматирования суммы
 * 
 * 用于输出金额时分离符号、整数部分和小数部分。
 * Для разделения знака, целой и дробной частей при выводе суммы.
 */
struct MoneyFormat {
    bool negative{};                    ///< 是否为负数 / Отрицательное ли число
    unsigned long long major{};         ///< 整数部分 / Целая часть
    unsigned long long minor{};         ///< 小数部分（0-999）/ Дробная часть (0-999)
};

/**
 * @brief 格式化金额用于输出 / Форматирование суммы для вывода
 * 
 * 输出前统一格式化金额，保留符号与 3 位小数。
 * 将 double 类型的金额转换为 MoneyFormat 结构，便于使用 printf 输出。
 * 
 * Единое форматирование суммы перед выводом, сохранение знака и 3 знаков после запятой.
 * Преобразование суммы типа double в структуру MoneyFormat для удобного вывода через printf.
 * 
 * @param amount 待格式化的金额 / Сумма для форматирования
 * @return 格式化后的结构 / Отформатированная структура
 */
inline MoneyFormat formatMoney(double amount) {
    MoneyFormat fmt{};
    fmt.negative = amount < 0.0;
    double absValue = fmt.negative ? -amount : amount;
    absValue = Money::round3(absValue);
    fmt.major = static_cast<unsigned long long>(std::floor(absValue + 1e-9));
    double fraction = absValue - static_cast<double>(fmt.major);
    fmt.minor = static_cast<unsigned long long>(std::llround(fraction * 1000.0));
    if (fmt.minor >= 1000) {
        fmt.major += 1;
        fmt.minor -= 1000;
    }
    return fmt;
}

}  // namespace domain

