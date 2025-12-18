#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include "parser.h"
#include "ast.h"
#include <string>
#include <istream>
#include <ostream>
#include <memory>

// Структура класса JSONParser
class JSONParser : public Parser<void> {
public:
    std::shared_ptr<ASTNode> parse(const std::string& input) override;
    std::shared_ptr<ASTNode> parse(std::istream& input) override;

private:
    std::string input_;
    size_t pos_;
    
    // Вспомогательные методы
    void skipWhitespace();
    char peek();
    char next();
    bool expect(char c);
    
    // Методы парсинга
    std::shared_ptr<ASTNode> parseValue();
    std::shared_ptr<ASTNode> parseNull();
    std::shared_ptr<ASTNode> parseBoolean();
    std::shared_ptr<ASTNode> parseNumber();
    std::shared_ptr<ASTNode> parseString();
    std::shared_ptr<ASTNode> parseArray();
    std::shared_ptr<ASTNode> parseObject();
    
    std::string parseStringValue();
    std::string parseUnicode();
};

// JSON序列化器类
class JSONSerializer : public Serializer<void> {
public:
    std::string serialize(const std::shared_ptr<ASTNode>& node) override;
    void serialize(const std::shared_ptr<ASTNode>& node, std::ostream& output) override;

private:
    void serializeValue(const std::shared_ptr<ASTNode>& node, std::ostream& output, int indent = 0);
    void serializeString(const std::string& str, std::ostream& output);
    std::string escapeString(const std::string& str);
};

#endif // JSON_PARSER_H

