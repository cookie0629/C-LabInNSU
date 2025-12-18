#ifndef TOML_PARSER_H
#define TOML_PARSER_H

#include "parser.h"
#include "ast.h"
#include <string>
#include <istream>
#include <ostream>
#include <memory>

// TOML解析器类
class TOMLParser : public Parser<void> {
public:
    std::shared_ptr<ASTNode> parse(const std::string& input) override;
    std::shared_ptr<ASTNode> parse(std::istream& input) override;

private:
    std::string input_;
    size_t pos_;
    std::vector<std::string> currentPath_;
    
    void skipWhitespace();
    void skipComment();
    void skipWhitespaceAndComments();
    char peek();
    char next();
    bool expect(char c);
    bool isEndOfLine();
    
    std::shared_ptr<ASTNode> parseValue();
    std::shared_ptr<ASTNode> parseString();
    std::shared_ptr<ASTNode> parseNumber();
    std::shared_ptr<ASTNode> parseBoolean();
    std::shared_ptr<ASTNode> parseDateTime();
    std::shared_ptr<ASTNode> parseArray();
    std::shared_ptr<ASTNode> parseInlineTable();
    
    std::string parseBasicString();
    std::string parseLiteralString();
    std::string parseKey();
    std::vector<std::string> parseKeyPath();
    
    void parseKeyValue(std::shared_ptr<ASTNode>& root);
    void parseTable(std::shared_ptr<ASTNode>& root);
    void parseArrayTable(std::shared_ptr<ASTNode>& root);
    
    void setNestedValue(std::shared_ptr<ASTNode>& root, const std::vector<std::string>& path, std::shared_ptr<ASTNode> value);
};

// TOML序列化器类
class TOMLSerializer : public Serializer<void> {
public:
    std::string serialize(const std::shared_ptr<ASTNode>& node) override;
    void serialize(const std::shared_ptr<ASTNode>& node, std::ostream& output) override;

private:
    void serializeValue(const std::shared_ptr<ASTNode>& node, std::ostream& output, int indent = 0);
    void serializeString(const std::string& str, std::ostream& output);
    void serializeObject(const std::shared_ptr<ASTNode>& node, std::ostream& output, const std::string& prefix = "", int indent = 0);
    std::string escapeString(const std::string& str);
};

#endif // TOML_PARSER_H

