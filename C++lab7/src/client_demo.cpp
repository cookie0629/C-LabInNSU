#include "broker.hpp"

#include <chrono>
#include <iostream>
#include <thread>

int main(int argc, char* argv[]) {
    // 设置默认的生产者和消费者端点
    // Установка конечных точек по умолчанию для производителя и потребителя
    std::string producer_ep = "tcp://localhost:5555";
    std::string consumer_ep = "tcp://localhost:5556";
    
    // 从命令行参数获取自定义端点
    // Получение пользовательских конечных точек из аргументов командной строки
    if (argc >= 2) producer_ep = argv[1];
    if (argc >= 3) consumer_ep = argv[2];

    // 创建ZeroMQ上下文和套接字
    // Создание контекста и сокетов ZeroMQ
    zmq::context_t ctx{1};  // 单线程上下文
    zmq::socket_t prod{ctx, zmq::socket_type::req};  // REQ套接字用于生产者
    zmq::socket_t cons{ctx, zmq::socket_type::req};  // REQ套接字用于消费者
    
    // 连接到代理服务器
    // Подключение к серверу брокера
    prod.connect(producer_ep);
    cons.connect(consumer_ep);

    // Lambda函数：发送JSON请求并接收响应
    // Лямбда-функция: отправка JSON запроса и получение ответа
    auto send_recv = [](zmq::socket_t& sock, const std::string& json) {
        sock.send(zmq::buffer(json), zmq::send_flags::none);  // 发送请求
        zmq::message_t reply;  // 接收响应的消息对象
        sock.recv(reply);      // 接收响应
        return reply.to_string();  // 转换为字符串返回
    };

    // 步骤1：订阅队列
    // Шаг 1: Подписка на очередь
    std::cout << "Subscribing to queue 'HighPriorityQueue'...\n";
    std::string subscribe = R"({"action":"subscribe","queue":"HighPriorityQueue","qos":"require_ack"})";
    std::cout << send_recv(cons, subscribe) << "\n";

    // 步骤2：发送示例消息
    // Шаг 2: Отправка примерного сообщения
    std::cout << "Sending sample message...\n";
    std::string produce =
        R"({"action":"produce","queue":"HighPriorityQueue","payload":"{\"msg\":\"hello\"}","qos":"require_ack"})";
    std::cout << send_recv(prod, produce) << "\n";

    // 等待一小段时间，确保消息已处理
    // Ожидание небольшого промежутка времени, чтобы убедиться, что сообщение обработано
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // 步骤3：获取消息
    // Шаг 3: Получение сообщения
    std::cout << "Fetching...\n";
    std::string fetch = R"({"action":"fetch"})";
    auto resp = send_recv(cons, fetch);
    std::cout << resp << "\n";

    // 简化的消息ID解析（查找JSON中的message_id字段）
    // Упрощённый разбор ID сообщения (поиск поля message_id в JSON)
    auto id_pos = resp.find("\"message_id\":\"");
    if (id_pos != std::string::npos) {
        // 提取消息ID
        // Извлечение ID сообщения
        auto id_start = id_pos + 14;
        auto id_end = resp.find('"', id_start);
        auto message_id = resp.substr(id_start, id_end - id_start);
        
        // 步骤4：发送确认消息
        // Шаг 4: Отправка подтверждающего сообщения
        std::string ack = std::string(R"({"action":"ack","message_id":")") + message_id + "\"}";
        std::cout << "Acking...\n";
        std::cout << send_recv(cons, ack) << "\n";
    }

    std::cout << "Done.\n";
    return 0;
}