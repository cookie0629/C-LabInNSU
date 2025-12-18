# 格式转换器 (Format Converter)
一个用C++实现的格式转换工具，支持在JSON、TOML和XML三种格式之间进行相互转换。

## 项目简介

本项目是一个控制台应用程序，实现了JSON、TOML和XML三种数据格式的解析、序列化和相互转换功能。项目完全使用C++标准库实现，不依赖任何第三方库，展示了现代C++的模板编程、面向对象设计、错误处理等核心特性。

## 项目结构

```
.
├── CMakeLists.txt          # CMake构建配置文件
├── README.md               # 项目说明文档
├── include/                # 头文件目录
│   ├── ast.h              # 抽象语法树(AST)定义
│   ├── parser.h           # 解析器和序列化器基类
│   ├── json_parser.h      # JSON解析器和序列化器
│   ├── toml_parser.h       # TOML解析器和序列化器
│   ├── xml_parser.h        # XML解析器和序列化器
│   └── converter.h         # 格式转换器
├── src/                    # 源文件目录
│   ├── main.cpp           # 主程序入口
│   ├── converter.cpp      # 转换器实现
│   ├── json_parser.cpp    # JSON解析器实现
│   ├── toml_parser.cpp    # TOML解析器实现
│   └── xml_parser.cpp     # XML解析器实现
└── tests/                  # 测试文件目录
    ├── test_main.cpp      # 测试主程序
    ├── test_json.h/cpp    # JSON测试
    ├── test_toml.h/cpp    # TOML测试
    ├── test_xml.h/cpp     # XML测试
    └── test_converter.h/cpp # 转换器测试
```

## 编译和运行

### 前置要求

- C++17或更高版本的编译器（GCC 7+, Clang 5+, MSVC 2017+）
- CMake 3.10或更高版本

### 编译步骤

1. **创建构建目录**
   ```bash
   mkdir build
   cd build
   ```

2. **配置CMake**
   ```bash
   cmake ..
   ```

3. **编译项目**
   ```bash
   cmake --build .
   ```
   在Windows上需要指定配置：
   ```bash
   cmake --build . --config Release    # Release版本（推荐）
   cmake --build . --config Debug      # Debug版本（用于调试）
   ```

4. **运行测试**
   
   使用ctest（推荐）：
   ```bash
   ctest -C Release    # Windows上需要指定配置

   ```
   
   或者直接运行测试程序：
   ```bash
   # Linux/Mac
   ./test_format_converter
   
   # Windows (Release配置)
   .\Release\test_format_converter.exe
   
   # Windows (Debug配置)
   .\Debug\test_format_converter.exe
   ```

### 使用方法

程序通过命令行参数指定输入和输出格式，从标准输入读取数据，将结果输出到标准输出。

**基本语法：**
```bash
./FormatConverter <input_format> <output_format>
```

**支持的格式：**
- `json` - JSON格式
- `toml` - TOML格式
- `xml` - XML格式

**示例：**

1. **JSON转TOML** (Linux/Mac)
   ```bash
   echo '{"name": "test", "value": 42}' | ./FormatConverter json toml
   ```
   (Windows PowerShell)
   ```powershell
   '{"name": "test", "value": 42}' | .\FormatConverter.exe json toml
   ```

2. **TOML转XML** (Linux/Mac)
   ```bash
   echo 'name = "test"' | ./FormatConverter toml xml
   ```
   (Windows PowerShell)
   ```powershell
   'name = "test"' | .\FormatConverter.exe toml xml
   ```

3. **XML转JSON** (Linux/Mac)
   ```bash
   echo '<root><name>test</name></root>' | ./FormatConverter xml json
   ```
   (Windows PowerShell)
   ```powershell
   '<root><name>test</name></root>' | .\FormatConverter.exe xml json
   ```

4. **从文件读取** (Linux/Mac)
   ```bash
   cat input.json | ./FormatConverter json xml > output.xml
   ```
   (Windows PowerShell)
   ```powershell
   Get-Content input.json -Raw | .\FormatConverter.exe json xml | Out-File output.xml -Encoding utf8
   ```

## 技术实现

### 核心设计

1. **抽象语法树(AST)**
   - 使用`std::variant`实现类型安全的节点值存储
   - 支持基本数据类型：null、boolean、number、string、array、object
   - 使用智能指针管理内存

2. **解析器设计**
   - 使用模板基类`Parser<T>`和`Serializer<T>`定义接口
   - 每种格式实现独立的解析器和序列化器
   - 使用递归下降解析算法

3. **错误处理**
   - 自定义`ParseException`异常类
   - 提供详细的错误信息和位置信息
   - 错误输出到stderr

4. **模板编程**
   - `FormatTraits`模板为每种格式绑定解析器与序列化器
   - `FormatConverter<InputFormat, OutputFormat>`模板类实现编译期组合
   - `convertDocument`通过模板函数表实现运行期调度

### 支持的数据类型

所有格式都支持以下基本数据类型：
- **Null**: 空值
- **Boolean**: 布尔值（true/false）
- **Number**: 数字（整数和浮点数）
- **String**: 字符串
- **Array**: 数组/列表
- **Object**: 对象/映射/表

### 格式特性

#### JSON
- 完全符合JSON标准
- 支持转义字符和Unicode
- 支持嵌套对象和数组

#### TOML
- 支持基本键值对
- 支持嵌套表（`[table]`）
- 支持数组表（`[[array]]`）
- 支持内联表
- 支持注释

#### XML
- 支持元素、属性和文本内容
- 支持嵌套结构
- 支持CDATA
- 自动处理XML声明

## 开发指南

### 添加新格式支持

要添加新的格式支持，需要：

1. 创建新的解析器类，继承`Parser<void>`
2. 创建新的序列化器类，继承`Serializer<void>`
3. 在`converter.cpp`中添加格式枚举和转换逻辑
4. 更新`main.cpp`中的格式解析

### 运行测试

项目包含完整的单元测试，覆盖：
- JSON解析和序列化
- TOML解析和序列化
- XML解析和序列化
- 格式转换功能

运行测试：
```bash
cd build
./test_format_converter
```

## 注意事项

1. **输入格式**：确保输入数据格式正确，否则会抛出解析异常
2. **数据类型转换**：某些格式特定的特性（如TOML的日期时间）可能转换为字符串
3. **XML结构**：XML转换为其他格式时，属性会存储在`@attributes`字段中
4. **内存管理**：使用智能指针自动管理内存，无需手动释放

## 常见问题

**Q: 为什么某些JSON格式转换后格式略有不同？**
A: 序列化器会重新格式化输出，空格和缩进可能不同，但数据内容保持一致。

**Q: 支持哪些编码？**
A: 程序支持UTF-8编码的输入和输出。

**Q: 如何处理大文件？**
A: 程序使用流式处理，可以处理较大的输入文件，但整个文档会加载到内存中。

## 许可证

本项目为课程作业项目，仅供学习使用。

## 作者

C++ Lab 4 - 第四题实现

---

**提示**：如果遇到编译错误，请确保：
1. 使用C++17或更高版本的编译器
2. CMake版本至少为3.10
3. 所有源文件都在正确的位置

