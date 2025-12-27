#include "broker.hpp"

#include <fstream>
#include <iomanip>
#include <sstream>
#include <iostream>

namespace broker {

namespace {
// Проверяет, истек ли срок действия сообщения
bool expired(const Message& m) {
    if (!m.ttl) return false;
    const auto deadline = m.created + *m.ttl;
    return std::chrono::system_clock::now() > deadline;
}
}

// 转换为字符串的辅助函数
// Вспомогательные функции для преобразования в строку
std::string to_string(Durability d) { return d == Durability::Disk ? "disk" : "memory"; }
std::string to_string(Order o) {
    switch (o) {
        case Order::FIFO: return "fifo";
        case Order::Unordered: return "unordered";
        case Order::Sorted: return "sorted";
    }
    return "fifo";
}
std::string to_string(ProducerQos q) {
    return q == ProducerQos::FireAndForget ? "fire_and_forget" : "require_ack";
}
std::string to_string(ConsumerQos q) {
    switch (q) {
        case ConsumerQos::FireAndForget: return "fire_and_forget";
        case ConsumerQos::RequireAck: return "require_ack";
        case ConsumerQos::Resume: return "resume";
    }
    return "fire_and_forget";
}

// 从字符串解析枚举值
// Разбор значений перечислений из строк
Durability durability_from(const std::string& s) { return s == "disk" ? Durability::Disk : Durability::Memory; }
Order order_from(const std::string& s) {
    if (s == "unordered") return Order::Unordered;
    if (s == "sorted") return Order::Sorted;
    return Order::FIFO;
}
ProducerQos producer_qos_from(const std::string& s) {
    return s == "require_ack" ? ProducerQos::RequireAck : ProducerQos::FireAndForget;
}
ConsumerQos consumer_qos_from(const std::string& s) {
    if (s == "require_ack") return ConsumerQos::RequireAck;
    if (s == "resume") return ConsumerQos::Resume;
    return ConsumerQos::FireAndForget;
}

// JSON字符串转义函数
// Функция экранирования строк JSON
std::string json_escape(const std::string& in) {
    std::ostringstream oss;
    for (char c : in) {
        switch (c) {
            case '\"': oss << "\\\""; break;   // 双引号
            case '\\': oss << "\\\\"; break;   // 反斜杠
            case '\n': oss << "\\n"; break;    // 换行符
            case '\r': oss << "\\r"; break;    // 回车符
            case '\t': oss << "\\t"; break;    // 制表符
            default: oss << c; break;
        }
    }
    return oss.str();
}

// 构建JSON响应字符串
// Создание строки JSON-ответа
std::string build_json_response(const std::map<std::string, std::string>& kv) {
    std::ostringstream oss;
    oss << "{";
    bool first = true;
    for (const auto& [k, v] : kv) {
        if (!first) oss << ",";
        first = false;
        oss << "\"" << json_escape(k) << "\":\"" << json_escape(v) << "\"";
    }
    oss << "}";
    return oss.str();
}

// QueueStore构造函数
// Конструктор QueueStore
QueueStore::QueueStore(QueueConfig cfg, std::filesystem::path storage_root)
    : cfg_(std::move(cfg)) {
    // 设置存储目录路径
    // Установка пути к каталогу хранения
    storage_dir_ = storage_root / cfg_.name;
    if (cfg_.durability == Durability::Disk) {
        // 创建目录并加载磁盘数据
        // Создание каталога и загрузка данных с диска
        std::filesystem::create_directories(storage_dir_);
        load_from_disk();
    }
}

// 消息入队
// Постановка сообщения в очередь
void QueueStore::enqueue(Message msg) {
    messages_.push_back(std::move(msg));    // 添加到消息列表
    sort_if_needed();                       // 如果需要，进行排序
    persist_all();                          // 持久化到磁盘
}

// 获取消息（用于需要确认的消费者）
// Получение сообщения (для потребителей с подтверждением)
std::optional<Message> QueueStore::fetch_for_ack() {
    cleanup_expired();                     // 清理过期消息
    if (messages_.empty()) return std::nullopt;
    
    Message msg;
    if (cfg_.order == Order::Unordered) {
        // 无序队列：随机选择消息
        // Неупорядоченная очередь: случайный выбор сообщения
        std::uniform_int_distribution<size_t> dist(0, messages_.size() - 1);
        auto idx = dist(rng_);
        msg = messages_[idx];
        messages_.erase(messages_.begin() + idx);
    } else {
        // FIFO或排序队列：取第一条消息
        // FIFO или отсортированная очередь: взять первое сообщение
        msg = messages_.front();
        messages_.erase(messages_.begin());
    }
    persist_all();                         // 更新持久化存储
    return msg;
}

// 查看指定索引的消息（不删除）
// Просмотр сообщения по указанному индексу (без удаления)
std::optional<Message> QueueStore::peek_at(size_t index) const {
    if (index >= messages_.size()) return std::nullopt;
    return messages_[index];
}

// 消息重新入队（用于ACK超时）
// Повторная постановка сообщения в очередь (для тайм-аута ACK)
void QueueStore::requeue(Message msg) {
    messages_.insert(messages_.begin(), std::move(msg));  // 插入到队列开头
    sort_if_needed();                                      // 重新排序
    persist_all();                                         // 持久化
}

// 根据ID删除消息
// Удаление сообщения по ID
void QueueStore::drop(const std::string& id) {
    auto it = std::remove_if(messages_.begin(), messages_.end(), 
                             [&](const Message& m) { return m.id == id; });
    if (it != messages_.end()) {
        messages_.erase(it, messages_.end());  // 删除匹配的消息
        persist_all();                         // 更新持久化存储
    }
}

// 清理过期消息
// Очистка просроченных сообщений
void QueueStore::cleanup_expired() {
    const auto before = messages_.size();  // 记录清理前的数量
    // 使用remove-erase惯用法删除过期消息
    // Использование идиомы remove-erase для удаления просроченных сообщений
    messages_.erase(std::remove_if(messages_.begin(), messages_.end(), expired), messages_.end());
    if (messages_.size() != before) persist_all();  // 如果数量变化，更新持久化
}

// 如果需要，对消息进行排序
// Сортировка сообщений, если необходимо
void QueueStore::sort_if_needed() {
    if (cfg_.order != Order::Sorted) return;  // 只有排序队列需要排序
    // 按创建时间排序（目前实现，可根据sort_key扩展）
    // Сортировка по времени создания (текущая реализация, можно расширить по sort_key)
    std::sort(messages_.begin(), messages_.end(),
              [](const Message& a, const Message& b) { return a.created < b.created; });
}

// 将所有消息持久化到磁盘
// Сохранение всех сообщений на диск
void QueueStore::persist_all() {
    if (cfg_.durability != Durability::Disk) return;  // 只持久化磁盘队列
    
    std::ofstream f(storage_dir_ / "messages.json", std::ios::trunc);
    f << "[\n";
    for (size_t i = 0; i < messages_.size(); ++i) {
        const auto& m = messages_[i];
        f << "  {\"id\":\"" << json_escape(m.id) << "\","
          << "\"payload\":\"" << json_escape(m.payload) << "\","
          << "\"created\":" << std::chrono::duration_cast<std::chrono::milliseconds>(
              m.created.time_since_epoch()).count();
        if (m.ttl) f << ",\"ttl_ms\":" << m.ttl->count();
        f << "}";
        if (i + 1 != messages_.size()) f << ",";  // 最后一个元素后不加逗号
        f << "\n";
    }
    f << "]";
}

// 从磁盘加载消息（简化的JSON解析）
// Загрузка сообщений с диска (упрощённый JSON парсер)
void QueueStore::load_from_disk() {
    const auto path = storage_dir_ / "messages.json";
    if (!std::filesystem::exists(path)) return;  // 文件不存在则返回
    
    // 简化的恢复逻辑：假设文件是由persist_all()生成的
    // Упрощённая логика восстановления: предполагается, что файл создан persist_all()
    std::ifstream f(path);
    std::string line;
    while (std::getline(f, line)) {
        const auto id_pos = line.find("\"id\":\"");
        const auto payload_pos = line.find("\"payload\":\"");
        if (id_pos == std::string::npos || payload_pos == std::string::npos) continue;
        
        // 提取ID和负载
        // Извлечение ID и полезной нагрузки
        auto id_start = id_pos + 6;
        auto id_end = line.find('"', id_start);
        auto payload_start = payload_pos + 11;
        auto payload_end = line.find('"', payload_start);
        
        Message m;
        m.id = line.substr(id_start, id_end - id_start);
        m.payload = line.substr(payload_start, payload_end - payload_start);
        m.created = std::chrono::system_clock::now();  // 设置为当前时间（简化处理）
        
        messages_.push_back(std::move(m));
    }
}

// BrokerApp构造函数
// Конструктор BrokerApp
BrokerApp::BrokerApp(Options opts) : opts_(std::move(opts)) {
    // 读取YAML文件，创建队列存储实例
    // Чтение YAML файла, создание экземпляров хранилища очередей
    load_config();
    // 绑定ZeroMQ套接字
    // Привязка сокетов ZeroMQ
    producer_.bind(opts_.producer_endpoint);
    // 创建消费者ROUTER套接字
    // Создание сокета ROUTER для потребителей
    consumer_.bind(opts_.consumer_endpoint);
}

// 主运行循环
// Главный цикл выполнения
void BrokerApp::run() {
    std::chrono::system_clock::time_point last_sweep{};  // 上次清理时间

    // Lambda函数：接收消息体（处理多部分消息）
    // Лямбда-функция: получение тела сообщения (обработка многочастных сообщений)
    auto recv_body = [](zmq::socket_t& sock) {
        zmq::message_t part;
        std::string body;
        while (true) {
            auto ok = sock.recv(part, zmq::recv_flags::none);
            if (!ok) break;
            body = part.to_string();
            const bool more = sock.get(zmq::sockopt::rcvmore);  // 检查是否还有更多部分
            if (!more) break;
        }
        return body;
    };

    while (true) {
        // 设置轮询项（生产者、消费者套接字）
        // Настройка элементов опроса (сокеты производителя и потребителя)
        zmq::pollitem_t items[] = {
            {static_cast<void*>(producer_), 0, ZMQ_POLLIN, 0},
            {static_cast<void*>(consumer_), 0, ZMQ_POLLIN, 0},
        };
        
        // 1. 轮询套接字（等待客户端连接/消息）
        // 1. Опрос сокетов (ожидание подключения/сообщений от клиентов)
        zmq::poll(items, 2, opts_.sweep_interval.count());
        
        // 2. 处理生产者消息
        // 2. Обработка сообщений от производителей
        if (items[0].revents & ZMQ_POLLIN) {
            zmq::message_t id;
            auto ok = producer_.recv(id, zmq::recv_flags::none);
            if (!ok) continue;
            auto body = recv_body(producer_);
            std::cout << "[producer] recv: " << body << std::endl;
            handle_producer(id, body);
        }
        
        // 3. 处理消费者消息
        // 3. Обработка сообщений от потребителей
        if (items[1].revents & ZMQ_POLLIN) {
            zmq::message_t id;
            auto ok = consumer_.recv(id, zmq::recv_flags::none);
            if (!ok) continue;
            auto body = recv_body(consumer_);
            std::cout << "[consumer] recv: " << body << std::endl;
            handle_consumer(id, body);
        }

        auto now = std::chrono::system_clock::now();
        // 4. 定期清理任务
        // 4. Регулярные задачи очистки
        if (now - last_sweep >= opts_.sweep_interval) {
            sweep();        // 执行清理
            last_sweep = now;  // 更新时间戳
        }
    }
}

// 加载YAML配置文件
// Загрузка конфигурации из YAML файла
void BrokerApp::load_config() {
    YAML::Node root = YAML::LoadFile(opts_.config_path);
    // 解析队列配置
    // Разбор конфигурации очередей
    auto queues = root["queues"];
    if (!queues) throw std::runtime_error("queues not defined in config");
    
    for (const auto& q : queues) {
        QueueConfig cfg;
        cfg.name = q["name"].as<std::string>();
        cfg.durability = durability_from(q["durability"].as<std::string>("memory"));
        cfg.order = order_from(q["order"].as<std::string>("fifo"));
        
        if (q["message_ttl"]) cfg.ttl = std::chrono::seconds(q["message_ttl"].as<int>());
        if (q["sort_key"]) cfg.sort_key = q["sort_key"].as<std::string>();
        
        // 创建队列存储实例
        // Создание экземпляра хранилища очереди
        queues_.emplace(cfg.name, QueueStore{cfg, opts_.storage_root});
    }
}

// 生成唯一消息ID
// Генерация уникального ID сообщения
std::string BrokerApp::make_message_id() {
    std::uniform_int_distribution<uint64_t> dist;
    std::ostringstream oss;
    oss << std::hex << dist(rng_);  // 使用16进制格式
    return oss.str();
}

// 获取当前时间的ISO格式字符串
// Получение строки ISO формата текущего времени
std::string BrokerApp::now_iso() const {
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    gmtime_s(&tm, &t);   // Windows安全版本
#else
    gmtime_r(&t, &tm);   // POSIX可重入版本
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%FT%TZ");  // ISO 8601格式
    return oss.str();
}

// 处理生产者请求
// Обработка запросов от производителей
void BrokerApp::handle_producer(zmq::message_t& identity, const std::string& body) {
    // 解析JSON请求（简化解析，使用子串查找）
    // Разбор JSON запроса (упрощённый разбор, поиск подстрок)
    auto qpos = body.find("\"queue\":\"");
    auto ppos = body.find("\"payload\":\"");
    
    if (qpos == std::string::npos || ppos == std::string::npos) {
        // 请求格式错误
        // Неверный формат запроса
        const auto resp = build_json_response({{"status", "error"}, {"reason", "bad_request"}});
        zmq::message_t empty;
        producer_.send(identity, zmq::send_flags::sndmore);   // 发送身份帧
        producer_.send(empty, zmq::send_flags::sndmore);      // 发送空分隔帧
        producer_.send(zmq::buffer(resp), zmq::send_flags::none);  // 发送响应
        return;
    }
    
    // 提取队列名称和负载
    // Извлечение имени очереди и полезной нагрузки
    auto qstart = qpos + 9;
    auto qend = body.find('"', qstart);
    auto queue_name = body.substr(qstart, qend - qstart);
    
    auto payload_start = ppos + 11;
    auto payload_end = body.find('"', payload_start);
    auto payload = body.substr(payload_start, payload_end - payload_start);

    // 解析QoS级别
    // Разбор уровня QoS
    ProducerQos qos = ProducerQos::FireAndForget;
    if (body.find("require_ack") != std::string::npos) qos = ProducerQos::RequireAck;

    // 查找队列
    // Поиск очереди
    auto it = queues_.find(queue_name);
    if (it == queues_.end()) {
        // 队列不存在
        // Очередь не найдена
        const auto resp = build_json_response({{"status", "error"}, {"reason", "queue_not_found"}});
        zmq::message_t empty;
        producer_.send(identity, zmq::send_flags::sndmore);
        producer_.send(empty, zmq::send_flags::sndmore);
        producer_.send(zmq::buffer(resp), zmq::send_flags::none);
        return;
    }

    // 创建消息对象
    // Создание объекта сообщения
    Message msg;
    msg.id = make_message_id();
    msg.payload = payload;
    msg.created = std::chrono::system_clock::now();
    msg.ttl = it->second.config().ttl;  // 继承队列的TTL设置
    
    // 将消息加入队列
    // Добавление сообщения в очередь
    it->second.enqueue(std::move(msg));

    // 根据QoS返回响应
    // Возврат ответа в зависимости от QoS
    if (qos == ProducerQos::RequireAck) {
        // 需要确认：返回消息ID
        // Требуется подтверждение: возврат ID сообщения
        const auto resp = build_json_response({{"status", "ok"}, {"message_id", msg.id}});
        zmq::message_t empty;
        producer_.send(identity, zmq::send_flags::sndmore);
        producer_.send(empty, zmq::send_flags::sndmore);
        producer_.send(zmq::buffer(resp), zmq::send_flags::none);
    } else {
        // 无需确认：只返回接受状态
        // Подтверждение не требуется: только статус принятия
        const auto resp = build_json_response({{"status", "accepted"}});
        zmq::message_t empty;
        producer_.send(identity, zmq::send_flags::sndmore);
        producer_.send(empty, zmq::send_flags::sndmore);
        producer_.send(zmq::buffer(resp), zmq::send_flags::none);
    }
}

// 处理消费者请求
// Обработка запросов от потребителей
void BrokerApp::handle_consumer(zmq::message_t& identity, const std::string& body) {
    // 获取消费者身份标识（ZeroMQ identity）
    // Получение идентификатора потребителя (ZeroMQ identity)
    const std::string id_str(static_cast<char*>(identity.data()), identity.size());

    // 解析动作类型
    // Разбор типа действия
    auto action_pos = body.find("\"action\":\"");
    if (action_pos == std::string::npos) {
        // 请求格式错误
        // Неверный формат запроса
        const auto resp = build_json_response({{"status", "error"}, {"reason", "bad_request"}});
        zmq::message_t empty;
        consumer_.send(identity, zmq::send_flags::sndmore);
        consumer_.send(empty, zmq::send_flags::sndmore);
        consumer_.send(zmq::buffer(resp), zmq::send_flags::none);
        return;
    }
    
    auto act_start = action_pos + 10;
    auto act_end = body.find('"', act_start);
    auto action = body.substr(act_start, act_end - act_start);

    // 处理订阅请求
    // Обработка запроса на подписку
    if (action == "subscribe") {
        auto qpos = body.find("\"queue\":\"");
        auto qstart = qpos + 9;
        auto qend = body.find('"', qstart);
        auto queue_name = body.substr(qstart, qend - qstart);

        // 解析消费者QoS级别
        // Разбор уровня QoS потребителя
        ConsumerQos qos = ConsumerQos::FireAndForget;
        if (body.find("require_ack") != std::string::npos) qos = ConsumerQos::RequireAck;
        if (body.find("resume") != std::string::npos) qos = ConsumerQos::Resume;

        // 检查队列是否存在
        // Проверка существования очереди
        auto it = queues_.find(queue_name);
        if (it == queues_.end()) {
            const auto resp = build_json_response({{"status", "error"}, {"reason", "queue_not_found"}});
            zmq::message_t empty;
            consumer_.send(identity, zmq::send_flags::sndmore);
            consumer_.send(empty, zmq::send_flags::sndmore);
            consumer_.send(zmq::buffer(resp), zmq::send_flags::none);
            return;
        }

        // 创建消费者会话
        // Создание сессии потребителя
        ConsumerSession session;
        session.id = id_str;
        session.queue = queue_name;
        session.qos = qos;
        sessions_[id_str] = session;  // 存储会话
        
        // 返回订阅成功响应
        // Возврат ответа об успешной подписке
        const auto resp = build_json_response({{"status", "subscribed"}, {"queue", queue_name}});
        zmq::message_t empty;
        consumer_.send(identity, zmq::send_flags::sndmore);
        consumer_.send(empty, zmq::send_flags::sndmore);
        consumer_.send(zmq::buffer(resp), zmq::send_flags::none);
        return;
    }

    // 处理取消订阅请求
    // Обработка запроса на отмену подписки
    if (action == "unsubscribe") {
        sessions_.erase(id_str);  // 删除会话
        const auto resp = build_json_response({{"status", "unsubscribed"}});
        zmq::message_t empty;
        consumer_.send(identity, zmq::send_flags::sndmore);
        consumer_.send(empty, zmq::send_flags::sndmore);
        consumer_.send(zmq::buffer(resp), zmq::send_flags::none);
        return;
    }

    // 处理获取消息请求
    // Обработка запроса на получение сообщения
    if (action == "fetch") {
        // 检查消费者是否已订阅
        // Проверка, подписан ли потребитель
        auto sit = sessions_.find(id_str);
        if (sit == sessions_.end()) {
            const auto resp = build_json_response({{"status", "error"}, {"reason", "not_subscribed"}});
            zmq::message_t empty;
            consumer_.send(identity, zmq::send_flags::sndmore);
            consumer_.send(empty, zmq::send_flags::sndmore);
            consumer_.send(zmq::buffer(resp), zmq::send_flags::none);
            return;
        }
        
        // 查找对应的队列
        // Поиск соответствующей очереди
        auto qit = queues_.find(sit->second.queue);
        if (qit == queues_.end()) {
            const auto resp = build_json_response({{"status", "error"}, {"reason", "queue_not_found"}});
            zmq::message_t empty;
            consumer_.send(identity, zmq::send_flags::sndmore);
            consumer_.send(empty, zmq::send_flags::sndmore);
            consumer_.send(zmq::buffer(resp), zmq::send_flags::none);
            return;
        }

        // 根据QoS级别获取消息
        // Получение сообщения в зависимости от уровня QoS
        auto& session = sit->second;
        std::optional<Message> msg;
        
        if (session.qos == ConsumerQos::FireAndForget) {
            // 无需确认模式：只读取游标位置的消息
            // Режим без подтверждения: чтение сообщения по курсору
            msg = qit->second.peek_at(session.cursor);
            if (msg) session.cursor++;  // 移动游标
        } else {
            // 需要确认或恢复模式：取出消息并等待ACK
            // Режим с подтверждением или возобновлением: извлечение сообщения и ожидание ACK
            msg = qit->second.fetch_for_ack();
            if (msg) {
                session.pending = msg;  // 保存为待确认消息
                session.deadline = std::chrono::system_clock::now() + session.ack_timeout;
            }
        }

        // 检查是否获取到消息
        // Проверка, получено ли сообщение
        if (!msg) {
            const auto resp = build_json_response({{"status", "empty"}});
            zmq::message_t empty;
            consumer_.send(identity, zmq::send_flags::sndmore);
            consumer_.send(empty, zmq::send_flags::sndmore);
            consumer_.send(zmq::buffer(resp), zmq::send_flags::none);
            return;
        }

        // 返回消息给消费者
        // Возврат сообщения потребителю
        const auto resp = build_json_response(
            {{"status", "ok"}, {"message_id", msg->id}, {"payload", msg->payload}, {"timestamp", now_iso()}});
        zmq::message_t empty;
        consumer_.send(identity, zmq::send_flags::sndmore);
        consumer_.send(empty, zmq::send_flags::sndmore);
        consumer_.send(zmq::buffer(resp), zmq::send_flags::none);
        return;
    }

    // 处理消息确认请求
    // Обработка запроса на подтверждение сообщения
    if (action == "ack") {
        auto id_pos = body.find("\"message_id\":\"");
        if (id_pos == std::string::npos) return;  // 格式错误，忽略
        
        auto id_start = id_pos + 14;
        auto id_end = body.find('"', id_start);
        auto msg_id = body.substr(id_start, id_end - id_start);

        // 查找消费者会话并检查待确认消息是否匹配
        // Поиск сессии потребителя и проверка соответствия ожидающего подтверждения сообщения
        auto sit = sessions_.find(id_str);
        if (sit != sessions_.end() && sit->second.pending && sit->second.pending->id == msg_id) {
            // ACK成功：清除待确认状态
            // ACK успешен: очистка состояния ожидания подтверждения
            sit->second.pending.reset();
            const auto resp = build_json_response({{"status", "acknowledged"}, {"message_id", msg_id}});
            zmq::message_t empty;
            consumer_.send(identity, zmq::send_flags::sndmore);
            consumer_.send(empty, zmq::send_flags::sndmore);
            consumer_.send(zmq::buffer(resp), zmq::send_flags::none);
            return;
        }
        // 如果没有匹配的pending消息，不返回响应（客户端可能重复发送ACK）
        // Если нет соответствующего ожидающего сообщения, ответ не возвращается (клиент мог отправить повторный ACK)
    }
}

// 定期清理任务：TTL清理和ACK超时检查
// Регулярные задачи очистки: очистка TTL и проверка тайм-аутов ACK
void BrokerApp::sweep() {
    // 1. 清理所有队列的过期消息
    // 1. Очистка просроченных сообщений во всех очередях
    for (auto& [name, q] : queues_) {
        q.cleanup_expired();
    }
    
    // 2. 检查消费者ACK超时
    // 2. Проверка тайм-аутов ACK потребителей
    const auto now = std::chrono::system_clock::now();
    for (auto& [id, session] : sessions_) {
        // 只检查需要确认或恢复模式的会话
        // Проверяются только сессии с режимом подтверждения или возобновления
        if (session.qos == ConsumerQos::RequireAck || session.qos == ConsumerQos::Resume) {
            if (session.pending && now > session.deadline) {
                // ACK超时：消息重新入队
                // Тайм-аут ACK: повторная постановка сообщения в очередь
                auto qit = queues_.find(session.queue);
                if (qit != queues_.end()) {
                    qit->second.requeue(*session.pending);
                }
                session.pending.reset();  // 清除待确认状态
            }
        }
    }
}

}  // namespace broker