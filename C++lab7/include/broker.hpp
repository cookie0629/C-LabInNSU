#pragma once

#include <chrono>
#include <filesystem>
#include <map>
#include <optional>
#include <queue>
#include <random>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include <yaml-cpp/yaml.h>
#include <zmq.hpp>

namespace broker {

// Durability: 持久性类型，内存或磁盘。
// Durability: тип сохранности данных — в памяти или на диске.
enum class Durability { Memory, Disk };

// Order: 消息顺序，FIFO、无序或有序。
// Order: порядок сообщений — FIFO (очередь), без порядка или упорядоченный.
enum class Order { FIFO, Unordered, Sorted };

// ProducerQos和ConsumerQos: 生产者和消费者的QoS级别。
// ProducerQos и ConsumerQos:уровни качества обслуживания (QoS) для производителей и потребителей.
enum class ProducerQos { FireAndForget, RequireAck };
enum class ConsumerQos { FireAndForget, RequireAck, Resume };

// QueueConfig: 队列配置，包含名称、持久性、顺序、排序键和生存时间。
// QueueConfig:настройки очереди, включая название, стойкость хранения, порядок, ключ сортировки и TTL (время жизни).
struct QueueConfig {
    std::string name;                           // 队列名称
    Durability durability{Durability::Memory};  // 持久化类型，默认内存
    Order order{Order::FIFO};                   // 消息顺序，默认FIFO
    
    std::optional<std::string> sort_key;              // 可选排序键
    std::optional<std::chrono::seconds> ttl;          // 可选生存时间
};

// Message: 消息结构，包含ID、有效载荷、创建时间、生存时间和属性。
// Message:структура сообщения, содержащая ID, полезную нагрузку (payload), время создания, TTL и атрибуты.
struct Message {
    std::string id;                                       // 消息唯一标识符
    std::string payload;                                  // 已JSON序列化的负载
    std::chrono::system_clock::time_point created;        // 消息创建时间
    std::optional<std::chrono::seconds> ttl;              // 可选生存时间
    std::map<std::string, std::string> attributes;        // 扩展属性（键值对）
};

// ConsumerSession: 消费者会话，记录消费者状态，包括队列、服务质量、游标、待确认消息和截止时间。
// ConsumerSession:сессия потребителя, фиксирующая его состояние: очередь, QoS, курсор, неподтверждённые сообщения и срок действия.
struct ConsumerSession {
    std::string id;                                     // ZeroMQ身份标识
    std::string queue;                                  // 订阅的队列名称
    ConsumerQos qos;                                    // 消费者的QoS级别
    std::chrono::milliseconds ack_timeout{5000};        // ACK超时时间（默认5秒）
    size_t cursor{0};                                   // 游标位置（用于FireAndForget和Resume模式）
    std::optional<Message> pending;                     // 待确认的消息（如果有）
    std::chrono::system_clock::time_point deadline{};   // ACK截止时间
};

/*
每个队列对应一个QueueStore实例，负责消息的存储、检索、重新入队、删除和清理过期消息。
Каждая очередь соответствует экземпляру QueueStore, отвечающему за хранение, извлечение, 
повторную постановку в очередь, удаление и очистку просроченных сообщений.
*/
class QueueStore {
public:
    // 构造函数: 根据配置创建队列，如果是磁盘持久化，则创建存储目录并加载已有消息。
    // Конструктор: создание очереди на основе конфигурации, если используется сохранение на диск, 
    // то создаётся каталог хранения и загружаются существующие сообщения.
    explicit QueueStore(QueueConfig cfg, std::filesystem::path storage_root);

    // 获取队列配置
    // Получение конфигурации очереди
    const QueueConfig& config() const { return cfg_; }

    // enqueue: 将消息加入队列，如果是排序队列则重新排序，如果需要则持久化。
    // enqueue: добавление сообщения в очередь, если очередь отсортирована, выполняется пересортировка, 
    // при необходимости сохраняется на диск.
    void enqueue(Message msg);

    // fetch_for_ack: 根据队列顺序（有序或随机）取出一个消息（用于需要确认的消费者），并持久化。
    // fetch_for_ack: извлечение одного сообщения в соответствии с порядком очереди (упорядоченный или случайный) 
    // для потребителей, требующих подтверждения, с сохранением на диск.
    std::optional<Message> fetch_for_ack();
    
    // peek_at: 查看指定索引的消息（用于无需确认的消费者），不删除。
    // peek_at: просмотр сообщения по указанному индексу (для потребителей без подтверждения), без удаления.
    std::optional<Message> peek_at(size_t index) const;

    // requeue: 将消息重新放入队列（用于ACK超时）。
    // requeue: повторная постановка сообщения в очередь (для случаев тайм-аута ACK).
    void requeue(Message msg);
    
    // drop: 根据消息ID删除消息。
    // drop: удаление сообщения по его ID.
    void drop(const std::string& id);
    
    // cleanup_expired: 清理过期消息。
    // cleanup_expired: очистка просроченных сообщений.
    void cleanup_expired();

private:
    // sort_if_needed: 如果队列是排序类型，则按创建时间排序（目前只按创建时间，但可以根据sort_key扩展）。
    // sort_if_needed: если очередь отсортирована, выполняется сортировка по времени создания 
    // (в настоящее время только по времени создания, но может быть расширена по sort_key).
    void sort_if_needed();
    
    // persist_all: 将消息持久化到磁盘（如果配置为磁盘持久化）。
    // persist_all: сохранение сообщений на диск (если настроена дисковая сохранность).
    void persist_all();
    
    // load_from_disk: 从磁盘加载消息（简化版本，仅从messages.json读取）。
    // load_from_disk: загрузка сообщений с диска (упрощённая версия, чтение только из messages.json).
    void load_from_disk();

    QueueConfig cfg_;                       // 队列配置
    std::vector<Message> messages_;         // 消息存储容器
    std::filesystem::path storage_dir_;     // 磁盘存储目录路径
    std::mt19937_64 rng_{std::random_device{}()};  // 随机数生成器（用于无序队列）
};

/*
代理的主类，处理网络连接、配置加载、生产者和消费者请求。
Основной класс брокера, обрабатывающий сетевое подключение, загрузку конфигурации, 
запросы производителей и потребителей.
*/
class BrokerApp {
public:
    // 配置选项结构体
    // Структура параметров конфигурации
    struct Options {
        std::string config_path;                      // YAML配置文件路径
        std::string producer_endpoint{"tcp://*:5555"}; // 生产者端点，默认端口5555
        std::string consumer_endpoint{"tcp://*:5556"}; // 消费者端点，默认端口5556
        std::filesystem::path storage_root{"storage"}; // 存储根目录
        std::chrono::milliseconds sweep_interval{1000}; // 清理间隔，默认1秒
    };

    // 构造函数: 加载配置，绑定ZeroMQ套接字。
    // Конструктор: загрузка конфигурации, привязка сокетов ZeroMQ.
    explicit BrokerApp(Options opts);
    
    // run: 主循环，使用zmq::poll轮询生产者和消费者套接字，处理请求并定期进行清理（sweep）。
    // run: главный цикл, использует zmq::poll для опроса сокетов производителей и потребителей, 
    // обработки запросов и регулярной очистки (sweep).
    void run();

private:
    // load_config: 从YAML文件加载队列配置。
    // load_config: загрузка конфигурации очередей из YAML файла.
    void load_config();
    
    // handle_producer: 处理生产者请求（生产消息）。
    // handle_producer: обработка запросов производителей (создание сообщений).
    void handle_producer(zmq::message_t& identity, const std::string& body);
    
    // handle_consumer: 处理消费者请求（订阅、取消订阅、获取消息、确认）。
    // handle_consumer: обработка запросов потребителей (подписка, отписка, получение сообщений, подтверждение).
    void handle_consumer(zmq::message_t& identity, const std::string& body);
    
    // sweep: 定期清理过期消息，并检查消费者ACK超时，将超时消息重新入队。
    // sweep: регулярная очистка просроченных сообщений и проверка тайм-аутов ACK потребителей, 
    // повторная постановка в очередь сообщений с истёкшим сроком.
    void sweep();

    // 生成唯一消息ID
    // Генерация уникального ID сообщения
    std::string make_message_id();
    
    // 获取当前时间的ISO格式字符串
    // Получение текущего времени в формате ISO строки
    std::string now_iso() const;

    Options opts_;                                    // 代理配置选项
    zmq::context_t ctx_{1};                           // ZeroMQ上下文（单线程）
    zmq::socket_t producer_{ctx_, zmq::socket_type::router};  // 生产者ROUTER套接字
    zmq::socket_t consumer_{ctx_, zmq::socket_type::router};  // 消费者ROUTER套接字
    std::unordered_map<std::string, QueueStore> queues_;      // 队列存储映射（队列名→QueueStore）
    std::unordered_map<std::string, ConsumerSession> sessions_;// 消费者会话映射（消费者ID→会话）
    std::mt19937_64 rng_{std::random_device{}()};             // 随机数生成器（用于生成消息ID）
};

// 枚举转字符串的辅助函数
// Вспомогательные функции для преобразования перечислений в строки
std::string to_string(Durability d);
std::string to_string(Order o);
std::string to_string(ProducerQos q);
std::string to_string(ConsumerQos q);

// 字符串转枚举的解析函数
// Функции разбора для преобразования строк в перечисления
Durability durability_from(const std::string& s);
Order order_from(const std::string& s);
ProducerQos producer_qos_from(const std::string& s);
ConsumerQos consumer_qos_from(const std::string& s);

// JSON字符串转义函数
// Функция экранирования строк JSON
std::string json_escape(const std::string& in);

// 构建JSON响应字符串
// Создание строки JSON-ответа
std::string build_json_response(const std::map<std::string, std::string>& kv);

}  // namespace broker

