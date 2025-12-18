#include "test_converter.h"
#include "converter.h"
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

int testConverter() {
    std::cout << "Testing Format Converter..." << std::endl;
    int failures = 0;
    
    try {
        // 测试1: JSON到JSON
        {
            std::string json = "{\"name\": \"test\", \"value\": 42}";
            auto node = parseFromFormat(Format::JSON, json);
            std::string result = serializeToFormat(Format::JSON, node);
            ASSERT(!result.empty(), "Should convert JSON to JSON");
        }
        
        // 测试2: JSON到TOML
        {
            std::string json = "{\"name\": \"test\"}";
            auto node = parseFromFormat(Format::JSON, json);
            std::string result = serializeToFormat(Format::TOML, node);
            ASSERT(!result.empty(), "Should convert JSON to TOML");
        }
        
        // 测试3: JSON到XML
        {
            std::string json = "{\"root\": {\"name\": \"test\"}}";
            auto node = parseFromFormat(Format::JSON, json);
            std::string result = serializeToFormat(Format::XML, node);
            ASSERT(!result.empty(), "Should convert JSON to XML");
        }
        
        // 测试4: 使用模板格式转换器
        {
            std::string json = "{\"name\": \"templated\"}";
            FormatConverter<Format::JSON, Format::TOML> converter;
            std::string result = converter.convert(json);
            ASSERT(!result.empty(), "Template converter should produce output");
        }
        
        // 测试5: convertDocument调度
        {
            std::string xml = "<root><value>42</value></root>";
            std::string jsonResult = convertDocument(Format::XML, Format::JSON, xml);
            ASSERT(!jsonResult.empty(), "convertDocument should dispatch using template table");
        }
        
        // 测试6: 格式解析
        {
            ASSERT(parseFormat("json") == Format::JSON, "Should parse JSON format");
            ASSERT(parseFormat("toml") == Format::TOML, "Should parse TOML format");
            ASSERT(parseFormat("xml") == Format::XML, "Should parse XML format");
            ASSERT(parseFormat("unknown") == Format::UNKNOWN, "Should return UNKNOWN for invalid format");
        }
        
        std::cout << "  All Converter tests passed!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "  Converter test failed with exception: " << e.what() << std::endl;
        failures++;
    }
    
    return failures;
}

