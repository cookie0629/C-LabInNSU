#include "test_toml.h"
#include "toml_parser.h"
#include <iostream>
#include <sstream>
#include <cassert>

#define ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            std::cerr << "FAIL: " << message << std::endl; \
            return 1; \
        } \
    } while(0)

int testTOML() {
    std::cout << "Testing TOML Parser and Serializer..." << std::endl;
    int failures = 0;
    
    try {
        // 测试1: 解析简单键值对
        {
            TOMLParser parser;
            auto node = parser.parse("name = \"test\"");
            ASSERT(node->isObject(), "Should parse object");
            auto& obj = node->getObject();
            ASSERT(obj.find("name") != obj.end(), "Should have 'name' key");
            ASSERT(obj["name"]->getString() == "test", "Value should match");
        }
        
        // 测试2: 解析数字
        {
            TOMLParser parser;
            auto node = parser.parse("value = 42");
            auto& obj = node->getObject();
            ASSERT(obj["value"]->getNumber() == 42.0, "Number should match");
        }
        
        // 测试3: 解析布尔值
        {
            TOMLParser parser;
            auto node = parser.parse("flag = true");
            auto& obj = node->getObject();
            ASSERT(obj["flag"]->getBoolean() == true, "Boolean should match");
        }
        
        // 测试4: 解析数组
        {
            TOMLParser parser;
            auto node = parser.parse("arr = [1, 2, 3]");
            auto& obj = node->getObject();
            ASSERT(obj["arr"]->isArray(), "Should parse array");
            auto& arr = obj["arr"]->getArray();
            ASSERT(arr.size() == 3, "Array should have 3 elements");
        }
        
        // 测试5: 解析嵌套表
        {
            TOMLParser parser;
            auto node = parser.parse("[table]\nkey = \"value\"");
            auto& obj = node->getObject();
            ASSERT(obj.find("table") != obj.end(), "Should have 'table' key");
            ASSERT(obj["table"]->isObject(), "Table should be object");
        }
        
        std::cout << "  All TOML tests passed!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "  TOML test failed with exception: " << e.what() << std::endl;
        failures++;
    }
    
    return failures;
}

