- 依赖管理（Conan）
- 构建系统（CMake，MSVC/VS 或 WSL/Linux）
- 代码组织与库/可执行文件拆分
- 单元测试（GoogleTest）
- 日志（spdlog）
- 文档（Doxygen）

## 项目做什么
## Что делает проект

提供一个名为 `libcompressor` 的静态库，封装两种压缩算法（zlib、bzip2），并提供一个命令行程序 `compressor` 调用该库，对输入字符串进行压缩并以十六进制输出结果。
Проект содержит статическую библиотеку `libcompressor` (алгоритмы zlib и bzip2) и утилиту командной строки `compressor`, которая сжимает входную строку и печатает результат в шестнадцатеричном виде.

## 目录结构
## Структура проекта

```
libcompressor/            # 压缩功能库
  include/libcompressor/  # 头文件（对外 API）
  src/                    # 库的实现
  tests/                  # 单元测试
compressor/               # 命令行可执行程序（CLI）
CMakeLists.txt            # 顶层 CMake 配置
conanfile.txt            # Conan 依赖声明
README.md                # 快速上手与命令
GUIDE_新手说明.md         # 本文档（新手友好的详细说明）
```

## 关键文件与代码说明（逐段讲解）
## Пояснение ключевых файлов и кода (пошагово)

### 1) 头文件：`libcompressor/include/libcompressor/libcompressor.hpp`
### 1) Заголовочный файл: `libcompressor/include/libcompressor/libcompressor.hpp`

- 头部与基础类型
  - `#pragma once`：防止重复包含。
  - `#include <cstddef>`：引入 `std::size_t` 等基础大小类型。

- 算法枚举
  - `enum libcompressor_CompressionAlgorithm { libcompressor_Zlib, libcompressor_Bzip };`
  - 表示可选的压缩算法：zlib 与 bzip2。

- 缓冲区类型
  - `struct libcompressor_Buffer { char* data; int size; };`
  - `data` 指向一段连续内存；`size` 表示字节数。
  - 约定：如果 `data == nullptr` 或 `size <= 0`，表示一个空结果或错误。

- 函数声明与注释（中俄双语）
  - `libcompressor_Buffer libcompressor_compress(libcompressor_CompressionAlgorithm algo, libcompressor_Buffer input);`
  - 语义：根据 `algo` 压缩 `input`，返回新分配的压缩后缓冲区。调用者需要使用 `std::free` 释放返回的 `data`。

新手要点：头文件只暴露“用法”，不暴露实现；命名清晰、类型简单有助于理解内存所有权。
Подсказка: заголовок раскрывает только «интерфейс», а не реализацию; ясные имена и простые типы помогают понять владение памятью.

### 2) 实现文件：`libcompressor/src/libcompressor.cpp`
### 2) Реализация: `libcompressor/src/libcompressor.cpp`

- 头文件与第三方库
  - `#include "libcompressor/libcompressor.hpp"` 引入对外 API。
  - `#include <bzlib.h>`, `#include <zlib.h>` 分别使用 bzip2 与 zlib 的 C 接口。
  - `#include <cstdlib>`, `#include <cstring>` 提供 `malloc/free` 与 `std::mem*`。

- 内部辅助函数
  - 匿名命名空间中 `err()`：返回 `{nullptr, 0}` 的统一错误值，便于早退。

- 核心函数 `libcompressor_compress(...)`
  1) 入口校验：若输入为空（`!input.data || input.size <= 0`），直接返回 `err()`。
  2) 预估输出容量：`out_cap = input.size + 1024`，为最坏情况预留富余空间。
  3) 分配缓冲区：`char* out = (char*)std::malloc(out_cap)`；若分配失败则 `err()`。
  4) 根据算法分支：
     - zlib 分支：
       - 目标长度使用 `uLongf dest = out_cap`；调用 `compress2(...)`。
       - 若返回值非 `Z_OK` 则释放内存并 `err()`；否则 `out_size = (int)dest`。
     - bzip2 分支：
       - 目标长度使用 `unsigned int dest = out_cap`；调用 `BZ2_bzBuffToBuffCompress(...)`
       - 若返回值非 `BZ_OK` 则释放内存并 `err()`；否则 `out_size = (int)dest`。
     - 未知算法：释放内存并 `err()`。
  5) 成功路径：返回 `{out, out_size}`。调用者负责释放 `out`。

新手要点：
Подсказка:
- C 接口多用“入参+出参”的方式返回结果（这里通过 `dest` 回传实际长度）。
- 任意失败路径都应释放资源并统一返回错误值，避免泄漏。
- 统一的缓冲区结构让 API 简洁稳定。

### 3) 命令行程序：`compressor/src/compressor.cpp`
### 3) Утилита CLI: `compressor/src/compressor.cpp`

- 依赖与入口
  - `#include <spdlog/spdlog.h>`：用于错误日志。
  - `int main(int argc, char** argv)`：程序入口。

- 日志级别与参数校验
  - `spdlog::set_level(spdlog::level::err);` 仅输出错误级别，避免噪声。
  - 参数不足时打印用法：`compressor <zlib|bzip> <string>` 并返回失败。

- 解析算法参数
  - 将 `argv[1]` 写入 `std::string a`；若 `zlib` 选择 `libcompressor_Zlib`，若 `bzip` 选择 `libcompressor_Bzip`，否则报错。

- 构造输入缓冲区并压缩
  - `libcompressor_Buffer in{argv[2], (int)std::strlen(argv[2])};`
  - 调用 `libcompressor_compress(algo, in)`；若返回空指针或非正长度，记录错误并退出。

- 打印十六进制
  - `for (int i = 0; i < out.size; ++i) std::printf("%.2hhx", (unsigned char)out.data[i]);`
  - 末尾打印换行并 `std::free(out.data)` 释放内存。

新手要点：
Подсказка:
- CLI 层负责参数解析、日志与展示；库层负责算法与资源管理，分工清晰。
- 任何从库返回的动态内存都必须由调用者释放。

### 4) 单元测试：`libcompressor/tests/libcompressortest.cpp`
### 4) Модульные тесты: `libcompressor/tests/libcompressortest.cpp`

- 工具函数
  - `make_buf(const char* s)`：将 C 字符串包装成 `libcompressor_Buffer`，便于测试编写。

- 正向用例
  - `ZlibBasic` 与 `BzipBasic`：输入 `"abc"`，断言 `out.data != nullptr` 且 `out.size > 0`，最后释放输出。

- 边界用例
  - `EmptyInput`：输入空缓冲，期望返回 `{nullptr, 0}`。分别测试 zlib 与 bzip 分支。


### 5) 顶层构建：`CMakeLists.txt`
### 5) Верхнеуровневая сборка: `CMakeLists.txt`

- 语言与标准
  - `project(modern_cpp_task LANGUAGES CXX)`，`set(CMAKE_CXX_STANDARD 20)`。

- 警告与错误收敛
  - MSVC 下 `/W4 /WX`；其他编译器 `-Wall -Wextra -Wpedantic -Werror`。

- 依赖与子目录
  - `include(${CMAKE_BINARY_DIR}/conan_toolchain.cmake OPTIONAL)`：允许用 Conan 工具链。
  - `find_package(ZLIB REQUIRED)`、`find_package(BZip2 REQUIRED)`、`find_package(spdlog REQUIRED)`。
  - `add_subdirectory(libcompressor)` 与 `add_subdirectory(compressor)`。

- 文档与质量
  - `doxygen_add_docs(docs libcompressor/include ...)`：生成 API 文档到 `${CMAKE_BINARY_DIR}/docs`。
  - `format` 目标：批量 `clang-format -i`；`tidy` 目标：对源文件运行 `clang-tidy`。

新手要点：
Подсказка:
- 将第三方库以 `find_package` + target 的方式消费，能避免手工管理头文件/库路径。
- 自定义 `format`/`tidy` 目标把工程质量工具纳入一条命令。

### 6) 库构建：`libcompressor/CMakeLists.txt`
### 6) Сборка библиотеки: `libcompressor/CMakeLists.txt`

- 目标与包含路径
  - `add_library(libcompressor STATIC src/libcompressor.cpp)`。
  - `target_include_directories(libcompressor PUBLIC BUILD_INTERFACE:include INSTALL_INTERFACE:include)`。

- 链接第三方库
  - `target_link_libraries(libcompressor PUBLIC ZLIB::ZLIB BZip2::BZip2)`。

- 测试与安装
  - `BUILD_TESTING` 时引入 GTest，创建 `libcompressor_tests` 并注册 `add_test`。
  - 安装库与头文件到标准目录（`GNUInstallDirs`）。

### 7) 可执行文件：`compressor/CMakeLists.txt`
### 7) Исполняемый файл: `compressor/CMakeLists.txt`

- `add_executable(compressor src/compressor.cpp)`。
- `target_link_libraries(compressor PRIVATE libcompressor spdlog::spdlog)`。
- 安装可执行文件到二进制目录。

### 8) Conan 配置：`conanfile.txt`
### 8) Конфигурация Conan: `conanfile.txt`

- 依赖：`zlib`、`bzip2`、`spdlog`、`gtest`。
- 生成器：`CMakeDeps`（生成 find_package 文件）与 `CMakeToolchain`（生成工具链文件）。
- `cmake_layout`：标准化构建/生成目录布局。

新手要点：先 `conan install` 再 `cmake -B`，并确保 `-DCMAKE_TOOLCHAIN_FILE` 指向 Conan 生成的文件。
Подсказка: сначала выполните `conan install`, затем `cmake -B`, и убедитесь, что `-DCMAKE_TOOLCHAIN_FILE` указывает на сгенерированный Conan файл.

## 构建与运行（Windows / WSL / Linux）
## Сборка и запуск (Windows / WSL / Linux)

请参考 `README.md` 的对应小节，那里提供了可直接复制的命令：
- Windows（PowerShell + VS 2022 + x64）：使用 Conan 生成工具链至 `build/Debug/build/generators`，CMake 指向该工具链文件。
- WSL/Linux：与 Linux 命令一致，安装时使用 `sudo` 或自定义 `--prefix`。

## 学到什么

1. 如何用 Conan 声明与安装依赖（zlib/bzip2/spdlog/gtest）。
2. 如何在 CMake 中 `find_package` 并链接第三方库目标（`ZLIB::ZLIB`、`BZip2::BZip2`、`spdlog::spdlog`、`GTest::gtest_main`）。
3. 如何将项目拆分为库与可执行文件，并为库编写单元测试。
4. 如何生成 API 文档（`doxygen_add_docs`）和进行代码格式化/静态分析（`clang-format`、`clang-tidy`）。

## 常见问题（FAQ）
## Часто задаваемые вопросы (FAQ)

1) 切换生成器（MinGW ↔ Visual Studio）报错？
- 清理构建目录中的 `CMakeFiles` 与 `CMakeCache.txt`，或使用全新构建目录。

2) CMake 找不到 Conan 工具链？
- 确认 `conan install` 的输出目录，并将 `-DCMAKE_TOOLCHAIN_FILE` 指向正确路径（例如 `build/Debug/build/generators/conan_toolchain.cmake`）。

3) 安装失败提示需要管理员权限？
- Windows 使用 `--prefix` 指向用户可写目录，例如 `F:\C++First\install`。
- WSL/Linux 使用 `sudo cmake --install ...` 或 `--prefix "$HOME/.local"`。

## 进阶思考
## Идеи для развития

- 如何在库中加入解压函数（zlib 的 `uncompress`，bzip2 的 `BZ2_bzBuffToBuffDecompress`）。
- 如何让输出缓冲区按需自动扩容，减少内存浪费。
- 如何加入更多算法（比如 lz4、zstd）并抽象统一接口。
- 如何为 CLI 增加输入文件与输出文件参数，做成实用工具。
- 如何将 CLI 层与库层分离，做成一个可复用的库。

## 学习资源
## Ресурсы для обучения

- [CMake 官方文档](https://cmake.org/cmake/help/latest/)
- [Conan 官方文档](https://docs.conan.io/en/latest/)
- [spdlog 官方文档](https://spdlog.fmt.dev/)
- [GTest 官方文档](https://google.github.io/googletest/)
- [Doxygen 官方文档](https://www.doxygen.nl/index.html)


