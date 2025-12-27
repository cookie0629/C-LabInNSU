#pragma once

/**
 * @file Bank.hpp
 * @brief 银行系统主类定义 / Определение основного класса банковской системы
 * 
 * BankSystem 负责模拟题目描述的"银行分行"。它承担读取初始数据、
 * 跟踪银行运行状态、处理事件以及输出结果的全部工作。
 * 
 * BankSystem отвечает за симуляцию "банковского отделения" из описания задачи.
 * Он выполняет всю работу по чтению начальных данных, отслеживанию состояния
 * работы банка, обработке событий и выводу результатов.
 */

#include "Domain.hpp"

#include <array>
#include <map>
#include <optional>
#include <set>

namespace bank {

/**
 * @brief 银行系统主类 / Основной класс банковской системы
 * 
 * 该类封装了银行分行的所有业务逻辑，包括：
 * - 初始数据的读取和解析
 * - 银行营业日的管理
 * - 客户业务请求的处理
 * - 账户、存款、贷款的管理
 * - 交易日志的记录
 * 
 * Данный класс инкапсулирует всю бизнес-логику банковского отделения, включая:
 * - чтение и парсинг начальных данных
 * - управление банковскими днями
 * - обработку запросов клиентов
 * - управление счетами, депозитами, кредитами
 * - запись журнала транзакций
 */
class BankSystem {
public:
    /**
     * @brief 构造函数 / Конструктор
     * 
     * 初始化银行系统，设置默认状态。
     * Инициализирует банковскую систему, устанавливает состояние по умолчанию.
     */
    BankSystem();

    /**
     * @brief 加载初始数据 / Загрузка начальных данных
     * 
     * 按照题目要求的顺序读取所有初始数据表：
     * Accounts, Debits, Credits, Clients, Client Debit, Bank Accounts,
     * Client Credit, Work Places, Exchange Rates
     * 
     * Читает все начальные таблицы данных в порядке, требуемом задачей:
     */
    void loadInitialData();

    /**
     * @brief 运行事件循环 / Запуск цикла обработки событий
     * 
     * 从标准输入读取事件并逐一处理，直到输入结束。
     * Читает события из стандартного ввода и обрабатывает их по одному до конца ввода.
     */
    void run();

private:
    // ==================== 类型别名 / Псевдонимы типов ====================
    using AccountMap = std::unordered_map<unsigned long long, domain::Account>;
    using ClientMap = std::unordered_map<unsigned long long, domain::Client>;
    using DepositMap = std::unordered_map<unsigned long long, domain::Deposit>;
    using LoanMap = std::unordered_map<unsigned long long, domain::Loan>;

    // ==================== 核心数据存储 / Основное хранилище данных ====================
    AccountMap accounts_;          //所有账户的映射（账号 -> 账户信息）/ Карта всех счетов (номер -> информация)
    ClientMap clients_;            //所有客户的映射（客户ID -> 客户信息）/ Карта всех клиентов (ID -> информация)
    DepositMap deposits_;         //所有存款的映射（存款ID -> 存款信息）/ Карта всех депозитов (ID -> информация)
    LoanMap loans_;               //所有贷款的映射（贷款ID -> 贷款信息）/ Карта всех кредитов (ID -> информация)

    // ==================== 关联关系数据 / Данные связей ====================
    /**
     * @brief 客户账户关联映射 / Карта связей клиентов и счетов
     * 
     * 保存客户与账户/存款/贷款之间的关联关系。
     * 键：客户ID，值：该客户拥有的所有账户/存款/贷款关联列表。
     * 
     * Сохраняет связи между клиентами и счетами/депозитами/кредитами.
     * Ключ: ID клиента, значение: список всех связей счетов/депозитов/кредитов клиента.
     */
    std::unordered_map<unsigned long long, std::vector<domain::ClientAccount>> clientAccounts_;

    /**
     * @brief 银行内部账户映射 / Карта внутренних банковских счетов
     * 
     * 币种 -> 银行内部账户编号。
     * 每个币种对应一个银行内部账户，用于存储手续费、利息等银行资金。
     * 
     * Валюта -> номер внутреннего банковского счёта.
     * Каждой валюте соответствует один внутренний банковский счёт для хранения
     * комиссий, процентов и других банковских средств.
     */
    std::unordered_map<std::string, unsigned long long> bankInternalAccounts_;

    /**
     * @brief 工作岗位定义列表 / Список определений рабочих мест
     * 
     * 存储分行中各种工作岗位的类型和数量。
     * Хранит типы и количество различных рабочих мест в отделении.
     */
    std::vector<domain::WorkplaceDefinition> workplaces_;

    /**
     * @brief 汇率映射 / Карта курсов обмена валют
     * 
     * 键：(出售货币, 购买货币)，值：兑换系数。
     * Ключ: (продаваемая валюта, покупаемая валюта), значение: коэффициент обмена.
     */
    std::map<std::pair<std::string, std::string>, double> exchangeRates_;

    // ==================== 派生状态数据 / Производные данные состояния ====================
    /**
     * @brief 账户所有者映射 / Карта владельцев счетов
     * 
     * 快速判断一个账户属于哪个客户，便于权限校验。
     * 键：账户ID，值：客户ID。
     * 
     * Быстрая проверка, какому клиенту принадлежит счёт, для проверки прав доступа.
     * Ключ: ID счёта, значение: ID клиента.
     */
    std::unordered_map<unsigned long long, unsigned long long> accountOwners_;

    /**
     * @brief 客户姓名到ID的映射 / Карта имени клиента к ID
     * 
     * 支持按照"姓 名 父名"直接定位客户记录。
     * 键：客户姓名（字符串），值：客户ID。
     * 
     * Поддержка прямой локализации записи клиента по "фамилия имя отчество".
     * Ключ: имя клиента (строка), значение: ID клиента.
     */
    std::unordered_map<std::string, unsigned long long> clientNameToId_;

    /**
     * @brief 按币种统计的账户数量 / Количество счетов по валютам
     * 
     * 每个客户在不同币种上的活跃账户数，方便执行数量限制。
     * 键：客户ID，值：数组[0]=RUB账户数, [1]=YUAN, [2]=USD, [3]=EUR。
     * 
     * Количество активных счетов каждого клиента по разным валютам для ограничения количества.
     * Ключ: ID клиента, значение: массив [0]=счетов RUB, [1]=YUAN, [2]=USD, [3]=EUR.
     */
    std::unordered_map<unsigned long long, std::array<unsigned long long, 4>> accountCountByCurrency_;

    /**
     * @brief 下一个可用的账户ID / Следующий доступный ID счёта
     * 
     * 用于生成新账户时分配唯一编号。
     * Для присвоения уникального номера при создании нового счёта.
     */
    unsigned long long nextAccountId_{1};

    /**
     * @brief 下一个可用的客户ID / Следующий доступный ID клиента
     * 
     * 用于注册新客户时分配唯一编号。
     * Для присвоения уникального номера при регистрации нового клиента.
     */
    unsigned long long nextClientId_{1};

    // ==================== 运行时状态 / Состояние выполнения ====================
    domain::DateTime currentTime_{};      ///< 当前模拟时间 / Текущее время симуляции
    bool bankDayStarted_{false};          ///< 银行营业日是否已开始 / Начался ли банковский день
    bool bankDayClosed_{false};           ///< 银行营业日是否已结束 / Завершился ли банковский день

    // ==================== 输入阶段：按题目顺序读取各个数据表 / Фаза ввода: чтение таблиц данных ====================
    /**
     * @brief 读取账户数据表 / Чтение таблицы счетов
     * 
     * 格式："Accounts %llu\n" 后跟 count 行账户记录。
     * Формат: "Accounts %llu\n" затем count строк записей счетов.
     */
    void readAccounts();

    /**
     * @brief 读取存款数据表 / Чтение таблицы депозитов
     * 
     * 格式："Debits %llu\n" 后跟 count 行存款记录。
     * Формат: "Debits %llu\n" затем count строк записей депозитов.
     */
    void readDeposits();

    /**
     * @brief 读取贷款数据表 / Чтение таблицы кредитов
     * 
     * 格式："Credits %llu\n" 后跟 count 行贷款记录。
     * Формат: "Credits %llu\n" затем count строк записей кредитов.
     */
    void readLoans();

    /**
     * @brief 读取客户数据表 / Чтение таблицы клиентов
     * 
     * 格式："Clients %llu\n" 后跟 count 行客户记录。
     * Формат: "Clients %llu\n" затем count строк записей клиентов.
     */
    void readClients();

    /**
     * @brief 读取客户账户关联表 / Чтение таблицы связей клиентов и счетов
     * 
     * 格式："Client Debit %llu\n" 后跟 count 行关联记录。
     * Формат: "Client Debit %llu\n" затем count строк записей связей.
     */
    void readClientAccounts();

    /**
     * @brief 读取银行内部账户表 / Чтение таблицы внутренних банковских счетов
     * 
     * 格式："Bank Accounts %llu\n" 后跟 count 行账户ID。
     * Формат: "Bank Accounts %llu\n" затем count строк ID счетов.
     */
    void readBankAccounts();

    /**
     * @brief 读取客户债务表 / Чтение таблицы долгов клиентов
     * 
     * 格式："Client Credit %llu\n" 后跟 count 行债务记录。
     * Формат: "Client Credit %llu\n" затем count строк записей долгов.
     */
    void readClientDebts();

    /**
     * @brief 读取工作岗位表 / Чтение таблицы рабочих мест
     * 
     * 格式："Work Places %llu\n" 后跟 count 行岗位定义。
     * Формат: "Work Places %llu\n" затем count строк определений рабочих мест.
     */
    void readWorkplaces();

    /**
     * @brief 读取汇率表 / Чтение таблицы курсов обмена валют
     * 
     * 格式："Exchange Rates %llu\n" 后跟 count 行汇率记录。
     * Формат: "Exchange Rates %llu\n" затем count строк записей курсов обмена.
     */
    void readExchangeRates();

    /**
     * @brief 构建派生状态 / Построение производного состояния
     * 
     * 生成 accountOwners_、clientNameToId_、accountCountByCurrency_ 等派生结构，
     * 这些结构用于快速查找和权限校验。
     * 
     * Генерирует производные структуры accountOwners_, clientNameToId_,
     * accountCountByCurrency_ и т.д., используемые для быстрого поиска и проверки прав.
     */
    void buildDerivedState();

    // ==================== 运行阶段：事件驱动处理 / Фаза выполнения: обработка событий ====================
    /**
     * @brief 处理单个事件 / Обработка одного события
     * 
     * 解析事件字符串并分发到相应的处理函数。
     * Парсит строку события и направляет в соответствующую функцию обработки.
     * 
     * @param buffer 事件字符串缓冲区 / Буфер строки события
     */
    void processEvent(const char* buffer);

    /**
     * @brief 处理银行营业日开始事件 / Обработка события начала банковского дня
     * 
     * 无论事件何时到达，银行营业日均在 8:00 开始。
     * Независимо от времени прибытия события, банковский день всегда начинается в 8:00.
     */
    void handleStartOfDay(unsigned long long day, unsigned long long hour, unsigned long long minute);

    /**
     * @brief 处理银行营业日结束事件 / Обработка события окончания банковского дня
     * 
     * 无论事件何时到达，银行营业日均在 19:00 结束。
     * 需要处理每日复利、每日计息等业务。
     * Независимо от времени прибытия события, банковский день всегда заканчивается в 19:00.
     * Необходимо обработать ежедневные проценты и т.д.
     */
    void handleEndOfDay(unsigned long long day, unsigned long long hour, unsigned long long minute);

    /**
     * @brief 处理客户亲临办理事件 / Обработка события личного обращения клиента
     * 
     * 处理客户到银行办理业务的请求，包括查询余额、开户、销户等操作。
     * Обрабатывает запросы клиентов на банковские операции, включая запрос баланса,
     * открытие счёта, закрытие счёта и т.д.
     */
    void handlePersonalAppeal(unsigned long long day,
                              unsigned long long hour,
                              unsigned long long minute,
                              const char* payload);

    // ==================== 工具函数：复用查找/校验逻辑 / Вспомогательные функции: поиск/проверка ====================
    /**
     * @brief 确保客户存在，不存在则创建 / Гарантирует существование клиента, создаёт при отсутствии
     * 
     * 如果客户不存在，则根据 fallbackType 创建新客户记录。
     * 用于 Personal Appeal 中处理新客户注册。
     * 
     * Если клиент не существует, создаёт новую запись клиента согласно fallbackType.
     * Используется для регистрации новых клиентов в Personal Appeal.
     * 
     * @param name 客户姓名 / Имя клиента
     * @param fallbackType 新客户的默认类型 / Тип по умолчанию для нового клиента
     * @return 客户指针（可能为新创建的）/ Указатель на клиента (возможно, только что созданного)
     */
    domain::Client* ensureClientByName(const std::string& name, domain::CustomerKind fallbackType);

    /**
     * @brief 根据姓名查找客户 / Поиск клиента по имени
     * 
     * @param name 客户姓名 / Имя клиента
     * @return 客户指针，未找到返回 nullptr / Указатель на клиента, nullptr если не найден
     */
    domain::Client* findClientByName(const std::string& name);

    /**
     * @brief 查找账户（const版本）/ Поиск счёта (const версия)
     * 
     * @param accountId 账户ID / ID счёта
     * @return 账户指针，未找到返回 nullptr / Указатель на счёт, nullptr если не найден
     */
    const domain::Account* findAccount(unsigned long long accountId) const;

    /**
     * @brief 查找账户（非const版本）/ Поиск счёта (не const версия)
     * 
     * @param accountId 账户ID / ID счёта
     * @return 账户指针，未找到返回 nullptr / Указатель на счёт, nullptr если не найден
     */
    domain::Account* findAccount(unsigned long long accountId);

    /**
     * @brief 检查账户是否属于指定客户 / Проверка принадлежности счёта клиенту
     * 
     * @param clientId 客户ID / ID клиента
     * @param accountId 账户ID / ID счёта
     * @return true 如果账户属于该客户 / true если счёт принадлежит клиенту
     */
    bool accountBelongsToClient(unsigned long long clientId, unsigned long long accountId) const;

    /**
     * @brief 获取客户的类型枚举值 / Получение значения перечисления типа клиента
     * 
     * @param client 客户对象 / Объект клиента
     * @return 客户类型枚举值 / Значение перечисления типа клиента
     */
    domain::CustomerKind clientKind(const domain::Client& client) const;

    // ==================== 业务操作处理函数 / Функции обработки бизнес-операций ====================
    /**
     * @brief 处理查询账户余额操作 / Обработка операции запроса баланса счёта
     * 
     * 客户可以查询其拥有的任何账户的余额。需要权限校验。
     * Клиент может запросить баланс любого своего счёта. Требуется проверка прав.
     */
    void handleBalanceInquiry(domain::Client& client,
                              unsigned long long accountId,
                              unsigned long long day,
                              unsigned long long hour,
                              unsigned long long minute);

    /**
     * @brief 处理开户操作 / Обработка операции открытия счёта
     * 
     * 客户可以开立结算账户。需要检查账户数量限制、计算手续费、创建账户和存款记录。
     * Клиент может открыть расчётный счёт. Требуется проверка ограничений количества,
     * расчёт комиссии, создание записей счёта и депозита.
     */
    void handleCreateAccount(domain::Client& client,
                             domain::CustomerKind kind,
                             const std::string& currency,
                             unsigned long long day,
                             unsigned long long hour,
                             unsigned long long minute);

    // ==================== 统一日志输出，满足题目"交易日志"要求 / Единый вывод логов ====================
    /**
     * @brief 记录错误消息 / Запись сообщения об ошибке
     * 
     * 输出到 stderr。
     * Вывод в stderr.
     * 
     * @param message 错误消息 / Сообщение об ошибке
     */
    void logError(const char* message);

    /**
     * @brief 记录账户间转账日志 / Запись лога перевода между счетами
     * 
     * 按照题目要求的格式输出到 stderr，记录所有资金流动。
     * 格式："%llu # %llu:%llu # %llu -> %llu # %llu.%llu\n"
     * 
     * Вывод в stderr в формате, требуемом задачей, для записи всех денежных потоков.
     * Формат: "%llu # %llu:%llu # %llu -> %llu # %llu.%llu\n"
     */
    void logAccountTransfer(unsigned long long day,
                            unsigned long long hour,
                            unsigned long long minute,
                            unsigned long long fromAccount,
                            unsigned long long toAccount,
                            double amount) const;
};

}  // namespace bank
