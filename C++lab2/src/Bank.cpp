#include "Bank.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <string_view>

namespace {
constexpr std::size_t kLineBuffer = 512;
int kindIndex(domain::CustomerKind kind) {
    switch (kind) {
        case domain::CustomerKind::Individual: return 0;
        case domain::CustomerKind::VipIndividual: return 1;
        case domain::CustomerKind::Legal: return 2;
        case domain::CustomerKind::VipLegal: return 3;
        default: return -1;
    }
}

// 策略模式：根据客户类型返回不同的客户对象 / Стратегия: в зависимости от типа клиента возвращает соответствующий объект
bool isOperationAllowed(domain::CustomerKind kind, std::string_view operation) {
    // 操作权限表：每行对应一种操作，列对应客户类型（Individual, VipIndividual, Legal, VipLegal）
    // Таблица прав операций: каждая строка - операция, столбцы - типы клиентов
    static const std::unordered_map<std::string_view, std::array<bool, 4>> table = {
        {"Balance Inquiry", {true, true, true, true}},
        {"Create Account", {true, true, true, true}},
        {"Close Account", {true, true, true, true}},
        {"Withdraw Funds", {true, true, true, true}},
        {"Top-up Founds", {true, true, true, true}},
        {"Currency Exchange", {true, true, false, true}},  // Legal Entity 不能办理货币兑换
    };
    const auto idx = kindIndex(kind);
    if (idx < 0) return false;
    auto it = table.find(operation);
    if (it == table.end()) return false;
    return it->second[idx];
}

/**
 * @brief 去除行末换行符 / Удаление символов перевода строки в конце строки
 * 
 * 去除 fgets 读取的行末 \n/\r，保证后续解析稳定。
 * Удаляет \n/\r в конце строки, прочитанной fgets, для стабильности последующего парсинга.
 * 
 * @param line 待处理的字符串 / Обрабатываемая строка
 */
void trimLine(std::string& line) {
    while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) {
        line.pop_back();
    }
}

/**
 * @brief 将货币类型映射到数组下标 / Преобразование типа валюты в индекс массива
 * 
 * 由于只涉及 4 种货币，用下标 0~3 直接索引计数数组能换取更好的性能。
 * RUB -> 0, YUAN -> 1, USD -> 2, EUR -> 3
 * 
 * Поскольку задействовано только 4 валюты, использование индексов 0~3 для
 * прямого индексирования массива подсчёта даёт лучшую производительность.
 * RUB -> 0, YUAN -> 1, USD -> 2, EUR -> 3
 * 
 * @param currency 货币类型 / Тип валюты
 * @return 数组下标（0-3），无效货币返回 -1 / Индекс массива (0-3), -1 для недопустимой валюты
 */
int currencyIndex(domain::Currency currency) {
    switch (currency) {
        case domain::Currency::RUB: return 0;
        case domain::Currency::YUAN: return 1;
        case domain::Currency::USD: return 2;
        case domain::Currency::EUR: return 3;
        default: return -1;
    }
}

//模板方法模式 (Template Method Pattern)
/**
 算法框架固定：循环读取+错误处理的框架不变
具体步骤可变：每种数据类型有不同的解析逻辑
代码复用：避免在每个readXXX函数中重复循环代
 */
template <typename Fn>
void repeatRead(unsigned long long count, Fn &&fn) {
    for (unsigned long long i = 0; i < count; ++i) {
        if (!fn(i)) {
            std::fprintf(stderr, "Failed to read entry %llu\n", i);
            std::exit(EXIT_FAILURE);
        }
    }
}
}  // namespace

namespace bank {

BankSystem::BankSystem() = default;

void BankSystem::loadInitialData() {
    /**
     * 输入部分严格按照题目给出的顺序依次出现，
     * 每个 readXxx 函数都只负责解析一种数据表。
     * 
     * Входная часть строго следует порядку, указанному в задаче,
     * каждая функция readXxx отвечает только за парсинг одной таблицы данных.
     */
    readAccounts();         // 读取账户表 / Чтение таблицы счетов
    readDeposits();         // 读取存款表 / Чтение таблицы депозитов
    readLoans();            // 读取贷款表 / Чтение таблицы кредитов
    readClients();          // 读取客户表 / Чтение таблицы клиентов
    readClientAccounts();   // 读取客户账户关联表 / Чтение таблицы связей клиентов и счетов
    readBankAccounts();     // 读取银行内部账户表 / Чтение таблицы внутренних банковских счетов
    readClientDebts();      // 读取客户债务表 / Чтение таблицы долгов клиентов
    readWorkplaces();       // 读取工作岗位表 / Чтение таблицы рабочих мест
    readExchangeRates();    // 读取汇率表 / Чтение таблицы курсов обмена валют
    buildDerivedState();    // 构建派生状态（索引、映射等）/ Построение производного состояния (индексы, карты и т.д.)
}

void BankSystem::run() {
    char buffer[kLineBuffer];
    while (std::fgets(buffer, sizeof(buffer), stdin)) {
        if (std::strlen(buffer) < 5) continue; 
        processEvent(buffer);
    }
}

void BankSystem::readAccounts() {
    // “Accounts %llu” 表头 + count 行记录。
    unsigned long long count{};
    if (std::scanf("Accounts %llu\n", &count) != 1) {
        std::fprintf(stderr, "Failed to read Accounts count\n");
        std::exit(EXIT_FAILURE);
    }
    repeatRead(count, [this](unsigned long long) {
        unsigned long long id{};
        char type[domain::kMaxString]{};
        unsigned long long major{};
        unsigned long long minor{};
        char currency[domain::kMaxString]{};
        if (std::scanf("%llu # %99[a-zA-Z0-9/_ ] # %llu.%llu # %99[a-zA-Z0-9/_ ]\n",
                       &id,
                       type,
                       &major,
                       &minor,
                       currency) != 5) {
            return false;
        }
        domain::Account account{};
        account.id = id;
        account.type = type;
        account.balance = domain::parseMoneyParts(major, minor);
        account.currency = currency;
        accounts_.emplace(id, std::move(account));
        return true;
    });
}

void BankSystem::readDeposits() {
    // “Debits” 对应存款列表：包含利率、类型、创建日期与期限。
    unsigned long long count{};
    if (std::scanf("Debits %llu\n", &count) != 1) {
        std::fprintf(stderr, "Failed to read Debits count\n");
        std::exit(EXIT_FAILURE);
    }
    repeatRead(count, [this](unsigned long long) {
        unsigned long long id{};
        double rate{};
        char type[domain::kMaxString]{};
        unsigned long long created{};
        unsigned long long duration{};
        if (std::scanf("%llu # %le # %99[a-zA-Z0-9/_ ] # %llu # %llu\n",
                       &id,
                       &rate,
                       type,
                       &created,
                       &duration) != 5) {
            return false;
        }
        domain::Deposit deposit{};
        deposit.id = id;
        deposit.rate = rate;
        deposit.type = type;
        deposit.createdDay = created;
        deposit.durationDays = duration;
        deposits_.emplace(id, std::move(deposit));
        return true;
    });
}

void BankSystem::readLoans() {
    // “Credits” 对应贷款列表。
    unsigned long long count{};
    if (std::scanf("Credits %llu\n", &count) != 1) {
        std::fprintf(stderr, "Failed to read Credits count\n");
        std::exit(EXIT_FAILURE);
    }
    repeatRead(count, [this](unsigned long long) {
        unsigned long long id{};
        double rate{};
        unsigned long long major{};
        unsigned long long minor{};
        char type[domain::kMaxString]{};
        if (std::scanf("%llu # %le # %llu.%llu # %99[a-zA-Z0-9/_ ]\n",
                       &id,
                       &rate,
                       &major,
                       &minor,
                       type) != 5) {
            return false;
        }
        domain::Loan loan{};
        loan.id = id;
        loan.rate = rate;
        loan.amount = domain::parseMoneyParts(major, minor);
        loan.type = type;
        loans_.emplace(id, std::move(loan));
        return true;
    });
}

void BankSystem::readClients() {
    // 客户主数据：ID + 姓名 + 类型。
    unsigned long long count{};
    if (std::scanf("Clients %llu\n", &count) != 1) {
        std::fprintf(stderr, "Failed to read Clients count\n");
        std::exit(EXIT_FAILURE);
    }
    repeatRead(count, [this](unsigned long long) {
        unsigned long long id{};
        char name[domain::kMaxString]{};
        char type[domain::kMaxString]{};
        if (std::scanf("%llu # %99[a-zA-Z0-9/_ ] # %99[a-zA-Z0-9/_ ]\n",
                       &id,
                       name,
                       type) != 3) {
            return false;
        }
        domain::Client client{};
        client.id = id;
        client.name = name;
        client.type = type;
        clients_.emplace(id, std::move(client));
        return true;
    });
}

void BankSystem::readClientAccounts() {
    // “Client Debit” 建立客户与账户/存款的映射。
    unsigned long long count{};
    if (std::scanf("Client Debit %llu\n", &count) != 1) {
        std::fprintf(stderr, "Failed to read Client Debit count\n");
        std::exit(EXIT_FAILURE);
    }
    repeatRead(count, [this](unsigned long long) {
        unsigned long long clientId{};
        unsigned long long accountId{};
        unsigned long long depositId{};
        if (std::scanf("%llu # %llu # %llu\n", &clientId, &accountId, &depositId) != 3) {
            return false;
        }
        domain::ClientAccount rel{};
        rel.clientId = clientId;
        rel.accountId = accountId;
        rel.depositId = depositId;
        rel.isDepositAccount = depositId != 0;
        clientAccounts_[clientId].push_back(rel);
        return true;
    });
}

void BankSystem::readBankAccounts() {
    // 每个币种对应一个银行内部账户，保存手续费/利息。
    unsigned long long count{};
    if (std::scanf("Bank Accounts %llu\n", &count) != 1) {
        std::fprintf(stderr, "Failed to read Bank Accounts count\n");
        std::exit(EXIT_FAILURE);
    }
    repeatRead(count, [this](unsigned long long) {
        unsigned long long accountId{};
        if (std::scanf("%llu\n", &accountId) != 1) {
            return false;
        }
        auto it = accounts_.find(accountId);
        if (it == accounts_.end()) {
            std::fprintf(stderr, "Unknown bank internal account %llu\n", accountId);
            return false;
        }
        bankInternalAccounts_[it->second.currency] = accountId;
        return true;
    });
}

void BankSystem::readClientDebts() {
    // “Client Credit” 建立客户与贷款账户之间的关系。
    unsigned long long count{};
    if (std::scanf("Client Credit %llu\n", &count) != 1) {
        std::fprintf(stderr, "Failed to read Client Credit count\n");
        std::exit(EXIT_FAILURE);
    }
    repeatRead(count, [this](unsigned long long) {
        unsigned long long clientId{};
        unsigned long long accountId{};
        unsigned long long loanId{};
        if (std::scanf("%llu # %llu # %llu\n", &clientId, &accountId, &loanId) != 3) {
            return false;
        }
        domain::ClientAccount rel{};
        rel.clientId = clientId;
        rel.accountId = accountId;
        rel.loanId = loanId;
        rel.isLoanAccount = loanId != 0;
        clientAccounts_[clientId].push_back(rel);
        return true;
    });
}

void BankSystem::readWorkplaces() {
    // 记录各个岗位的数量，未来用于排队/统计。
    unsigned long long count{};
    if (std::scanf("Work Places %llu\n", &count) != 1) {
        std::fprintf(stderr, "Failed to read Work Places count\n");
        std::exit(EXIT_FAILURE);
    }
    repeatRead(count, [this](unsigned long long) {
        char type[domain::kMaxString]{};
        unsigned long long amount{};
        if (std::scanf("%99[a-zA-Z0-9/_ ] # %llu\n", type, &amount) != 2) {
            return false;
        }
        domain::WorkplaceDefinition def{};
        def.type = type;
        def.count = amount;
        workplaces_.push_back(def);
        return true;
    });
}

void BankSystem::readExchangeRates() {
    // 货币兑换比率，稍后可用于兑换/跨币种操作。
    unsigned long long count{};
    if (std::scanf("Exchange Rates %llu\n", &count) != 1) {
        std::fprintf(stderr, "Failed to read Exchange Rates count\n");
        std::exit(EXIT_FAILURE);
    }
    repeatRead(count, [this](unsigned long long) {
        char fromCur[domain::kMaxString]{};
        char toCur[domain::kMaxString]{};
        double ratio{};
        if (std::scanf("%99[a-zA-Z0-9/_ ] # %99[a-zA-Z0-9/_ ] # %le\n",
                       fromCur,
                       toCur,
                       &ratio) != 3) {
            return false;
        }
        exchangeRates_.emplace(std::make_pair(std::string(fromCur), std::string(toCur)), ratio);
        return true;
    });
}

void BankSystem::buildDerivedState() {
    // nextAccountId_/nextClientId_ 用于生成新的实体编号。
    nextAccountId_ = 1;
    for (const auto& [id, _] : accounts_) {
        nextAccountId_ = std::max(nextAccountId_, id + 1);
    }
    nextClientId_ = 1;
    for (const auto& [id, client] : clients_) {
        nextClientId_ = std::max(nextClientId_, id + 1);
        clientNameToId_[client.name] = id;
        accountCountByCurrency_[id];
    }

    accountOwners_.clear();
    accountCountByCurrency_.clear();
    for (const auto& [clientId, relations] : clientAccounts_) {
        for (const auto& rel : relations) {
            accountOwners_[rel.accountId] = clientId;
            auto accountIt = accounts_.find(rel.accountId);
            if (accountIt == accounts_.end()) continue;
            auto currency = domain::parseCurrency(accountIt->second.currency);
            const auto idx = currencyIndex(currency);
            if (idx >= 0) {
                accountCountByCurrency_[clientId][idx]++;
            }
        }
    }
}

domain::Client* BankSystem::findClientByName(const std::string& name) {
    auto it = clientNameToId_.find(name);
    if (it == clientNameToId_.end()) return nullptr;
    auto clientIt = clients_.find(it->second);
    if (clientIt == clients_.end()) return nullptr;
    return &clientIt->second;
}

//工厂方法模式 (Factory Method Pattern):根据客户类型创建客户对象。
domain::Client* BankSystem::ensureClientByName(const std::string& name, domain::CustomerKind fallbackType) {
    // 若客户不存在，则在 Personal Appeal 环节即时创建（仅允许特定业务）。
    if (auto existing = findClientByName(name)) {
        return existing;
    }
    domain::Client client{};
    client.id = nextClientId_++;
    client.name = name;
    client.type = domain::toString(fallbackType);
    auto [it, inserted] = clients_.emplace(client.id, std::move(client));
    (void)inserted;
    clientNameToId_[name] = it->second.id;
    accountCountByCurrency_[it->second.id];
    return &it->second;
}

const domain::Account* BankSystem::findAccount(unsigned long long accountId) const {
    auto it = accounts_.find(accountId);
    if (it == accounts_.end()) return nullptr;
    return &it->second;
}

domain::Account* BankSystem::findAccount(unsigned long long accountId) {
    auto it = accounts_.find(accountId);
    if (it == accounts_.end()) return nullptr;
    return &it->second;
}

bool BankSystem::accountBelongsToClient(unsigned long long clientId, unsigned long long accountId) const {
    auto it = accountOwners_.find(accountId);
    if (it == accountOwners_.end()) return false;
    return it->second == clientId;
}

domain::CustomerKind BankSystem::clientKind(const domain::Client& client) const {
    return domain::parseCustomerKind(client.type);
}

// 命令模式，将事件处理委托给具体的命令对象。
void BankSystem::processEvent(const char* buffer) {
    unsigned long long day{};
    unsigned long long hour{};
    unsigned long long minute{};
    char payload[kLineBuffer]{};
    if (std::sscanf(buffer, "%llu # %llu:%llu # %255[^\n]\n", &day, &hour, &minute, payload) != 4) {
        std::fprintf(stderr, "Unsupported event: %s", buffer);
        return;
    }

    if (std::strncmp(payload, "Start of Bank Day", 17) == 0) {
        handleStartOfDay(day, hour, minute);
        return;
    }
    if (std::strncmp(payload, "End of Bank Day", 15) == 0) {
        handleEndOfDay(day, hour, minute);
        return;
    }
    if (std::strncmp(payload, "Personal Appeal", 15) == 0) {
        handlePersonalAppeal(day, hour, minute, payload);
        return;
    }

    // TODO: implement remaining event handlers following the specification
}

// 工厂方法模式 (Factory Method Pattern):根据事件类型创建事件对象。
void BankSystem::handlePersonalAppeal(unsigned long long day,
                                      unsigned long long hour,
                                      unsigned long long minute,
                                      const char* payload) {
    // Personal Appeal 消息第一行包含客户姓名、类型标识与操作数量。
    // 随后的 N 行是真正的操作描述（例如 Balance Inquiry / Create Account）。
    char name[domain::kMaxString]{};
    char typeToken[domain::kMaxString]{};
    unsigned long long operationCount{};
    if (std::sscanf(payload,
                    "Personal Appeal # %99[a-zA-Z0-9/_ ] # %99[a-zA-Z0-9/_ ] # %llu",
                    name,
                    typeToken,
                    &operationCount) != 3) {
        logError("Malformed Personal Appeal");
        return;
    }

    std::vector<std::string> operations;
    operations.reserve(operationCount);
    for (unsigned long long i = 0; i < operationCount; ++i) {
        char opLine[kLineBuffer]{};
        if (!std::fgets(opLine, sizeof(opLine), stdin)) {
            logError("Unexpected end of input while reading operations");
            return;
        }
        std::string opString(opLine);
        trimLine(opString);
        operations.emplace_back(std::move(opString));
    }

    std::string nameStr(name);
    domain::Client* client = findClientByName(nameStr);
    domain::CustomerKind kind = domain::CustomerKind::NotClient;

    if (client) {
        kind = clientKind(*client);
    } else {
        kind = domain::parseClientToken(typeToken);
        if (kind == domain::CustomerKind::NotClient) {
            std::printf("%llu # %llu:%llu # Client error. Wrong operation for new client\n", day, hour, minute);
            return;
        }
        bool allowed = false;
        for (const auto& op : operations) {
            if (op.rfind("Create Account", 0) == 0 || op.rfind("Request Debit Card", 0) == 0) {
                allowed = true;
                break;
            }
        }
        if (!allowed) {
            std::printf("%llu # %llu:%llu # Client error. Wrong operation for new client\n", day, hour, minute);
            return;
        }
        client = ensureClientByName(nameStr, kind);
    }

    // 逐条处理客户请求：未实现的操作统一返回“Service not available”。
    for (const auto& operation : operations) {
        if (operation.rfind("Balance Inquiry", 0) == 0) {
            unsigned long long accountId{};
            if (std::sscanf(operation.c_str(), "Balance Inquiry # %llu", &accountId) != 1) {
                std::printf("%llu # %llu:%llu # Client error. Unknown account\n", day, hour, minute);
                continue;
            }
            if (!isOperationAllowed(kind, "Balance Inquiry")) {
                std::printf("%llu # %llu:%llu # Service not available\n", day, hour, minute);
                continue;
            }
            handleBalanceInquiry(*client, accountId, day, hour, minute);
        } else if (operation.rfind("Create Account", 0) == 0) {
            char currency[domain::kMaxString]{};
            if (std::sscanf(operation.c_str(), "Create Account # %99[a-zA-Z0-9/_ ]", currency) != 1) {
                std::printf("%llu # %llu:%llu # Client error. Unknown currency\n", day, hour, minute);
                continue;
            }
            if (!isOperationAllowed(kind, "Create Account")) {
                std::printf("%llu # %llu:%llu # Service not available\n", day, hour, minute);
                continue;
            }
            handleCreateAccount(*client, kind, currency, day, hour, minute);
        } else {
            std::printf("%llu # %llu:%llu # Service not available\n", day, hour, minute);
        }
    }
}

void BankSystem::handleStartOfDay(unsigned long long day,
                                  unsigned long long hour,
                                  unsigned long long minute) {
    currentTime_ = {day, 8, 0};
    bankDayStarted_ = true;
    bankDayClosed_ = false;
    (void)hour;
    (void)minute;
}

void BankSystem::handleEndOfDay(unsigned long long day,
                                unsigned long long hour,
                                unsigned long long minute) {
    (void)hour;
    (void)minute;
    if (!bankDayStarted_) {
        logError("Bank day end without start");
        return;
    }
    currentTime_.day = day;
    currentTime_.hour = 19;
    currentTime_.minute = 0;
    bankDayClosed_ = true;
}

void BankSystem::handleBalanceInquiry(domain::Client& client,
                                      unsigned long long accountId,
                                      unsigned long long day,
                                      unsigned long long hour,
                                      unsigned long long minute) {
    (void)client;
    auto account = findAccount(accountId);
    if (!account) {
        std::printf("%llu # %llu:%llu # Client error. Unknown account\n", day, hour, minute);
        return;
    }
    // 权限校验：题目要求只能查询属于自己的账户。
    auto owner = accountOwners_.find(accountId);
    if (owner == accountOwners_.end() || owner->second != client.id) {
        std::printf("%llu # %llu:%llu # Client error. Access denied\n", day, hour, minute);
        return;
    }
    auto fmt = domain::formatMoney(account->balance);
    std::printf("%llu # %llu:%llu # Balance of %llu # %s%llu.%03llu\n",
                day,
                hour,
                minute,
                accountId,
                fmt.negative ? "-" : "",
                fmt.major,
                fmt.minor);
}

//  策略模式 (Strategy Pattern)：根据客户类型和货币类型，选择不同的开户费率。
static double accountOpeningFee(domain::CustomerKind kind, domain::Currency currency) {
    struct Rule {
        domain::CustomerKind kind;
        domain::Currency currency;
        double fee;
    };

    static const Rule rules[] = {
        {domain::CustomerKind::Individual, domain::Currency::RUB, 10000.0},
        {domain::CustomerKind::VipIndividual, domain::Currency::RUB, 4000.0},
        {domain::CustomerKind::Legal, domain::Currency::RUB, 25000.0},
        {domain::CustomerKind::VipLegal, domain::Currency::RUB, 15000.0},
        {domain::CustomerKind::Individual, domain::Currency::YUAN, 2000.0},
        {domain::CustomerKind::VipIndividual, domain::Currency::YUAN, 1000.0},
        {domain::CustomerKind::Legal, domain::Currency::YUAN, 5000.0},
        {domain::CustomerKind::VipLegal, domain::Currency::YUAN, 2000.0},
        {domain::CustomerKind::Individual, domain::Currency::USD, 100.0},
        {domain::CustomerKind::VipIndividual, domain::Currency::USD, 50.0},
        {domain::CustomerKind::Legal, domain::Currency::USD, 200.0},
        {domain::CustomerKind::VipLegal, domain::Currency::USD, 100.0},
        {domain::CustomerKind::Individual, domain::Currency::EUR, 100.0},
        {domain::CustomerKind::VipIndividual, domain::Currency::EUR, 50.0},
        {domain::CustomerKind::Legal, domain::Currency::EUR, 200.0},
        {domain::CustomerKind::VipLegal, domain::Currency::EUR, 100.0},
    };

    for (const auto& rule : rules) {
        if (rule.kind == kind && rule.currency == currency) return rule.fee;
    }
    return 0.0;
}

// 策略模式 (Strategy Pattern)：根据客户类型和货币类型，选择不同的账户限额。
static unsigned int accountLimit(domain::CustomerKind kind, domain::Currency currency) {
    struct LimitRule {
        domain::CustomerKind kind;
        domain::Currency currency;
        unsigned int limit;
    };

    static const LimitRule rules[] = {
        {domain::CustomerKind::Individual, domain::Currency::RUB, 3},
        {domain::CustomerKind::VipIndividual, domain::Currency::RUB, 5},
        {domain::CustomerKind::Legal, domain::Currency::RUB, 10},
        {domain::CustomerKind::VipLegal, domain::Currency::RUB, 25},
        {domain::CustomerKind::Individual, domain::Currency::YUAN, 1},
        {domain::CustomerKind::VipIndividual, domain::Currency::YUAN, 5},
        {domain::CustomerKind::Legal, domain::Currency::YUAN, 5},
        {domain::CustomerKind::VipLegal, domain::Currency::YUAN, 15},
        {domain::CustomerKind::Individual, domain::Currency::USD, 1},
        {domain::CustomerKind::VipIndividual, domain::Currency::USD, 3},
        {domain::CustomerKind::Legal, domain::Currency::USD, 5},
        {domain::CustomerKind::VipLegal, domain::Currency::USD, 10},
        {domain::CustomerKind::Individual, domain::Currency::EUR, 1},
        {domain::CustomerKind::VipIndividual, domain::Currency::EUR, 3},
        {domain::CustomerKind::Legal, domain::Currency::EUR, 5},
        {domain::CustomerKind::VipLegal, domain::Currency::EUR, 10},
    };

    for (const auto& rule : rules) {
        if (rule.kind == kind && rule.currency == currency) return rule.limit;
    }
    return 0;
}

void BankSystem::handleCreateAccount(domain::Client& client,
                                     domain::CustomerKind kind,
                                     const std::string& currencyStr,
                                     unsigned long long day,
                                     unsigned long long hour,
                                     unsigned long long minute) {
    // 1. 校验币种 + 账户数量限制
    auto currency = domain::parseCurrency(currencyStr);
    if (currency == domain::Currency::Unknown) {
        std::printf("%llu # %llu:%llu # Client error. Unknown currency\n", day, hour, minute);
        return;
    }

    const auto allowedAccounts = accountLimit(kind, currency);
    const auto currencyIdx = currencyIndex(currency);
    if (currencyIdx < 0) {
        std::printf("%llu # %llu:%llu # Client error. Unknown currency\n", day, hour, minute);
        return;
    }
    auto& perCurrency = accountCountByCurrency_[client.id];
    auto currentCount = perCurrency[currencyIdx];
    if (allowedAccounts > 0 && currentCount >= allowedAccounts) {
        std::printf("%llu # %llu:%llu # Client error. Active account limit reached\n", day, hour, minute);
        return;
    }

    // 2. 计算开户手续费，开户当天账户余额为负（题目说明）
    double fee = accountOpeningFee(kind, currency);
    double balance = -fee;

    unsigned long long accountId = nextAccountId_++;
    domain::Account newAccount{};
    newAccount.id = accountId;
    newAccount.type = "deposit";
    newAccount.balance = balance;
    newAccount.currency = currencyStr;
    newAccount.active = true;
    accounts_.emplace(accountId, newAccount);

    // 3. 建立客户-账户关系并更新各类辅助索引
    domain::ClientAccount rel{};
    rel.clientId = client.id;
    rel.accountId = accountId;
    clientAccounts_[client.id].push_back(rel);
    accountOwners_[accountId] = client.id;
    perCurrency[currencyIdx] = currentCount + 1;

    // 4. 银行内部账户收取手续费：若余额不足需要触发“Bank defaulted”
    auto bankAccountIt = bankInternalAccounts_.find(currencyStr);
    if (bankAccountIt == bankInternalAccounts_.end()) {
        std::printf("%llu # %llu:%llu # Bank defaulted\n", day, hour, minute);
        std::exit(EXIT_FAILURE);
    }
    auto bankAccount = findAccount(bankAccountIt->second);
    if (bankAccount) {
        bankAccount->balance += fee;
    }

    logAccountTransfer(day, hour, minute, accountId, bankAccountIt->second, fee);

    // 5. 正常输出题目要求的响应格式
    auto fmt = domain::formatMoney(newAccount.balance);
    std::printf("%llu # %llu:%llu # Account Created %llu # %s%llu.%03llu\n",
                day,
                hour,
                minute,
                accountId,
                fmt.negative ? "-" : "",
                fmt.major,
                fmt.minor);
}

void BankSystem::logError(const char* message) {
    std::fprintf(stderr, "%s\n", message);
}

void BankSystem::logAccountTransfer(unsigned long long day,
                                    unsigned long long hour,
                                    unsigned long long minute,
                                    unsigned long long fromAccount,
                                    unsigned long long toAccount,
                                    double amount) const {
    // stderr 日志格式严格遵循题目要求。
    auto fmt = domain::formatMoney(amount);
    std::fprintf(stderr,
                 "%llu # %llu:%llu # %llu -> %llu # %s%llu.%03llu\n",
                 day,
                 hour,
                 minute,
                 fromAccount,
                 toAccount,
                 fmt.negative ? "-" : "",
                 fmt.major,
                 fmt.minor);
}

}  // namespace bank

