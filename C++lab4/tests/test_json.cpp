#include "test_json.h"
#include "json_parser.h"
#include <iostream>
#include <sstream>
#include <cassert>

// Гарантирует, что макрос можно использовать как функцию
#define ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            std::cerr << "FAIL: " << message << std::endl; \
            return 1; \
        } \
    } while(0)

int testJSON() {
    std::cout << "Testing JSON Parser and Serializer..." << std::endl;
    int failures = 0;
    
    try {
        //Парсинг простой строки
        {
            JSONParser parser;
            auto node = parser.parse("\"hello\"");
            ASSERT(node->isString(), "Should parse string");
            ASSERT(node->getString() == "hello", "String value should match");
        }
        
        //Парсинг числа
        {
            JSONParser parser;
            auto node = parser.parse("42");
            ASSERT(node->isNumber(), "Should parse number");
            ASSERT(node->getNumber() == 42.0, "Number value should match");
        }
        
        //Разбор логических значений
        {
            JSONParser parser;
            auto trueNode = parser.parse("true");
            ASSERT(trueNode->isBoolean(), "Should parse boolean");
            ASSERT(trueNode->getBoolean() == true, "Boolean value should match");
            
            auto falseNode = parser.parse("false");
            ASSERT(falseNode->getBoolean() == false, "Boolean value should match");
        }
        
        //Парсинг null
        {
            JSONParser parser;
            auto node = parser.parse("null");
            ASSERT(node->isNull(), "Should parse null");
        }
        
        //Парсинг массивов
        {
            JSONParser parser;
            auto node = parser.parse("[1, 2, 3]");
            ASSERT(node->isArray(), "Should parse array");
            auto& arr = node->getArray();
            ASSERT(arr.size() == 3, "Array should have 3 elements");
            ASSERT(arr[0]->getNumber() == 1.0, "First element should be 1");
        }
        
        // Парсинг объектов
        {
            JSONParser parser;
            auto node = parser.parse("{\"name\": \"test\", \"value\": 42}");
            ASSERT(node->isObject(), "Should parse object");
            auto& obj = node->getObject();
            ASSERT(obj.find("name") != obj.end(), "Should have 'name' key");
            ASSERT(obj["name"]->getString() == "test", "Name value should match");
        }
        
        // Парсинг сложных структур
        {
            JSONSerializer serializer;
            auto node = ASTNode::createString("test");
            std::string result = serializer.serialize(node);
            ASSERT(result == "\"test\"", "Should serialize string correctly");
        }
        
        // Парсинг и сериализация сложных структур
        {
            std::string original = "{\"a\": 1, \"b\": [2, 3]}";
            JSONParser parser;
            JSONSerializer serializer;
            auto node = parser.parse(original);
            std::string serialized = serializer.serialize(node);
            auto node2 = parser.parse(serialized);
            ASSERT(node2->isObject(), "Round-trip should work");
        }
        
        std::cout << "  All JSON tests passed!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "  JSON test failed with exception: " << e.what() << std::endl;
        failures++;
    }
    
    return failures;
}

