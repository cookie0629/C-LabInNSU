# Lab7 消息代理（C++17 + ZeroMQ + YAML）

本文件只说明“如何构建与运行”，详细设计与代码讲解请看 `新手指导.md`。

## 依赖
- `libzmq` + `cppzmq`
- `yaml-cpp`
- C++17 编译器与 CMake ≥ 3.15

Windows 推荐用 vcpkg：
```pwsh
vcpkg install zeromq cppzmq yaml-cpp
```
配置时添加：
```pwsh
-DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake
```

## 构建
```bash
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="D:/vcpkg/scripts/buildsystems/vcpkg.cmake"
cmake --build build
```

## 运行
启动代理（生产者端口/消费者端口可自定义）：
```bash
.\build\Debug\broker_app.exe example_config.yaml tcp://*:5555 tcp://*:5556
```

示例客户端（另一个终端）：
```bash
.\build\Debug\client_demo.exe tcp://localhost:5555 tcp://localhost:5556
```

## 基本协议（JSON over ZeroMQ）
- 生产者：`{"action":"produce","queue":"Q","payload":"<json>","qos":"require_ack|fire_and_forget"}`
- 消费者订阅：`{"action":"subscribe","queue":"Q","qos":"fire_and_forget|require_ack|resume"}`
- 消费者取消息：`{"action":"fetch"}`
- 消费者确认：`{"action":"ack","message_id":"<id>"}`
- 取消订阅：`{"action":"unsubscribe"}`

## 示例配置
`example_config.yaml` 展示了磁盘 FIFO 队列和内存排序队列，含可选 TTL。根据需要增删队列即可。
