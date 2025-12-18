#ifndef CONVERTER_H
#define CONVERTER_H

#include "ast.h"
#include "json_parser.h"
#include "toml_parser.h"
#include "xml_parser.h"
#include <memory>
#include <stdexcept>
#include <string>

// 格式枚举
enum class Format {
    JSON,
    TOML,
    XML,
    UNKNOWN
};

// 每种格式的特征：绑定解析器与序列化器类型
template<Format fmt>
struct FormatTraits;
/*
FormatTraits<Format::JSON> / FormatTraits<Format::TOML> / FormatTraits<Format::XML>, 
которые связывают значения перечисления с соответствующими типами парсеров и сериализаторов.
*/
template<>
struct FormatTraits<Format::JSON> {
    using Parser = JSONParser;
    using Serializer = JSONSerializer;
    static constexpr const char* name = "json";
};

template<>
struct FormatTraits<Format::TOML> {
    using Parser = TOMLParser;
    using Serializer = TOMLSerializer;
    static constexpr const char* name = "toml";
};

template<>
struct FormatTraits<Format::XML> {
    using Parser = XMLParser;
    using Serializer = XMLSerializer;
    static constexpr const char* name = "xml";
};

// 模板函数：根据格式解析输入
template<Format fmt>
std::shared_ptr<ASTNode> parseWithFormat(const std::string& input) {
    typename FormatTraits<fmt>::Parser parser;
    return parser.parse(input);
}

// 模板函数：根据格式序列化输出
template<Format fmt>
std::string serializeWithFormat(const std::shared_ptr<ASTNode>& node) {
    typename FormatTraits<fmt>::Serializer serializer;
    return serializer.serialize(node);
}

// 模板格式转换器
/*
Использование шаблонных классов: 
например, FormatConverter<Format InputFormat, Format OutputFormat> 
, где входной и выходной форматы передаются как параметры шаблона 
в виде перечислений на этапе компиляции.
*/
template<Format InputFormat, Format OutputFormat>
class FormatConverter {
public:
    /*
    Использование шаблонных функций: 
    например, parseWithFormat<fmt> и serializeWithFormat<fmt>, 
    где параметр шаблона fmt позволяет выбрать корректный парсер 
    или сериализатор на этапе компиляции
    */
    std::shared_ptr<ASTNode> parseInput(const std::string& input) const {
        return parseWithFormat<InputFormat>(input);
    }
    
    std::string serializeOutput(const std::shared_ptr<ASTNode>& node) const {
        return serializeWithFormat<OutputFormat>(node);
    }
    
    std::string convert(const std::string& input) const {
        auto ast = parseInput(input);
        return serializeOutput(ast);
    }
};

// 格式工具函数
Format parseFormat(const std::string& formatStr);
std::shared_ptr<ASTNode> parseFromFormat(Format format, const std::string& input);
std::string serializeToFormat(Format format, const std::shared_ptr<ASTNode>& node);
std::string convertDocument(Format inputFormat, Format outputFormat, const std::string& input);

#endif // CONVERTER_H

