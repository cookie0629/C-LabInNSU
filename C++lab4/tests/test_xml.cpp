#include "test_xml.h"
#include "xml_parser.h"
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

int testXML() {
    std::cout << "Testing XML Parser and Serializer..." << std::endl;
    int failures = 0;
    
    try {
        // 测试1: 解析简单元素
        {
            XMLParser parser;
            auto node = parser.parse("<root>test</root>");
            ASSERT(node->isObject(), "Should parse object");
            auto& obj = node->getObject();
            ASSERT(obj.find("@tag") != obj.end(), "Should have tag");
            ASSERT(obj["@tag"]->getString() == "root", "Tag name should match");
        }
        
        // 测试2: 解析带属性的元素
        {
            XMLParser parser;
            auto node = parser.parse("<root id=\"1\">content</root>");
            auto& obj = node->getObject();
            ASSERT(obj.find("@attributes") != obj.end(), "Should have attributes");
            auto& attrs = obj["@attributes"]->getObject();
            ASSERT(attrs.find("id") != attrs.end(), "Should have 'id' attribute");
        }
        
        // 测试3: 解析嵌套元素
        {
            XMLParser parser;
            auto node = parser.parse("<root><child>value</child></root>");
            auto& obj = node->getObject();
            ASSERT(obj.find("@children") != obj.end(), "Should have children");
        }
        
        // 测试4: 序列化
        {
            XMLSerializer serializer;
            auto node = ASTNode::createObject();
            node->getObject()["@tag"] = ASTNode::createString("test");
            node->getObject()["@text"] = ASTNode::createString("content");
            std::string result = serializer.serialize(node);
            ASSERT(result.find("<test>") != std::string::npos, "Should serialize tag");
        }
        
        std::cout << "  All XML tests passed!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "  XML test failed with exception: " << e.what() << std::endl;
        failures++;
    }
    
    return failures;
}

