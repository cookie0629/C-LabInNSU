#ifndef XML_PARSER_H
#define XML_PARSER_H

#include "parser.h"
#include "ast.h"
#include <string>
#include <istream>
#include <ostream>
#include <memory>

// XML解析器类
class XMLParser : public Parser<void> {
public:
    std::shared_ptr<ASTNode> parse(const std::string& input) override;
    std::shared_ptr<ASTNode> parse(std::istream& input) override;

private:
    std::string input_;
    size_t pos_;
    
    void skipWhitespace();
    char peek();
    char next();
    bool expect(const std::string& str);
    
    std::shared_ptr<ASTNode> parseDocument();
    std::shared_ptr<ASTNode> parseElement();
    std::string parseTagName();
    std::map<std::string, std::string> parseAttributes();
    std::string parseAttributeValue();
    std::string parseText();
    std::string parseCDATA();
    void parseComment();
    void parsePI();
    
    bool isNameChar(char c);
    bool isNameStartChar(char c);
};

// XML序列化器类
class XMLSerializer : public Serializer<void> {
public:
    std::string serialize(const std::shared_ptr<ASTNode>& node) override;
    void serialize(const std::shared_ptr<ASTNode>& node, std::ostream& output) override;

private:
    void serializeElement(const std::shared_ptr<ASTNode>& node, std::ostream& output, int indent = 0);
    void serializeValue(const std::shared_ptr<ASTNode>& node, std::ostream& output, int indent = 0);
    std::string escapeXML(const std::string& str);
    std::string indentString(int indent);
};

#endif // XML_PARSER_H

