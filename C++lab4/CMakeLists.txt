cmake_minimum_required(VERSION 3.10)
project(FormatConverter VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 包含目录
include_directories(${CMAKE_SOURCE_DIR}/include)

# 源文件
set(SOURCES
    src/main.cpp
    src/converter.cpp
    src/json_parser.cpp
    src/toml_parser.cpp
    src/xml_parser.cpp
)

# 可执行文件
add_executable(${PROJECT_NAME} ${SOURCES})

# 编译选项
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()

# 测试
enable_testing()

# 测试可执行文件
add_executable(test_format_converter
    tests/test_main.cpp
    tests/test_json.cpp
    tests/test_toml.cpp
    tests/test_xml.cpp
    tests/test_converter.cpp
    src/converter.cpp
    src/json_parser.cpp
    src/toml_parser.cpp
    src/xml_parser.cpp
)

# 添加测试
add_test(NAME FormatConverterTests COMMAND test_format_converter)

# 设置测试工作目录
set_tests_properties(FormatConverterTests PROPERTIES
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

