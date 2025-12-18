#include "xml_parser.h"
#include <sstream>
#include <cctype>
#include <algorithm>

// XMLParser实现
std::shared_ptr<ASTNode> XMLParser::parse(const std::string& input) {
    input_ = input;
    pos_ = 0;
    skipWhitespace();
    return parseDocument();
}

std::shared_ptr<ASTNode> XMLParser::parse(std::istream& input) {
    std::stringstream ss;
    ss << input.rdbuf();
    return parse(ss.str());
}

void XMLParser::skipWhitespace() {
    while (pos_ < input_.length() && std::isspace(static_cast<unsigned char>(input_[pos_]))) {
        pos_++;
    }
}

char XMLParser::peek() {
    if (pos_ >= input_.length()) {
        return '\0';
    }
    return input_[pos_];
}

char XMLParser::next() {
    char c = peek();
    if (pos_ < input_.length()) pos_++;
    return c;
}

bool XMLParser::expect(const std::string& str) {
    for (char c : str) {
        if (peek() != c) {
            throw ParseException("Expected '" + str + "'", pos_);
        }
        pos_++;
    }
    return true;
}

bool XMLParser::isNameStartChar(char c) {
    return std::isalpha(static_cast<unsigned char>(c)) || c == '_' || c == ':';
}

bool XMLParser::isNameChar(char c) {
    return isNameStartChar(c) || std::isdigit(static_cast<unsigned char>(c)) || 
           c == '-' || c == '.' || c == 0xB7;
}

std::shared_ptr<ASTNode> XMLParser::parseDocument() {
    skipWhitespace();
    
    // 跳过XML声明和注释
    while (pos_ < input_.length()) {
        if (input_.substr(pos_, 5) == "<?xml") {
            parsePI();
        } else if (input_.substr(pos_, 4) == "<!--") {
            parseComment();
        } else if (input_[pos_] == '<') {
            break;
        } else {
            pos_++;
        }
        skipWhitespace();
    }
    
    if (peek() != '<') {
        throw ParseException("Expected XML element", pos_);
    }
    
    return parseElement();
}

std::shared_ptr<ASTNode> XMLParser::parseElement() {
    expect("<");                    // Ожидать открывающий тег
    std::string tagName = parseTagName();  // Распарсить имя тега
    auto element = ASTNode::createObject();  // Каждый элемент - это объект
    auto& obj = element->getObject();
    
    // Парсить атрибуты
    auto attrs = parseAttributes();
    if (!attrs.empty()) {
        auto attrsNode = ASTNode::createObject();
        for (const auto& [key, value] : attrs) {
            attrsNode->getObject()[key] = ASTNode::createString(value);
        }
        obj["@attributes"] = attrsNode;  // Атрибуты хранятся в ключе @attributes
    }
    
    skipWhitespace();
    
    // Проверить самозакрывающийся тег <tag/>
    if (peek() == '/' && pos_ + 1 < input_.length() && input_[pos_ + 1] == '>') {
        pos_ += 2;
        obj["@tag"] = ASTNode::createString(tagName);
        return element;
    }
    
    expect(">");
    
    // Парсить содержимое (текст и дочерние элементы)
    std::vector<std::shared_ptr<ASTNode>> children;
    std::string textContent;
    
    while (pos_ < input_.length()) {
        skipWhitespace();
        
        if (peek() == '<') {
            if (input_.substr(pos_, 4) == "<!--") {
                parseComment();  // Пропустить комментарий
                continue;
            } else if (pos_ + 1 < input_.length() && input_[pos_ + 1] == '/') {
                // Закрывающий тег </tag>
                pos_++;  // Пропустить '<'
                pos_++;  // Пропустить '/'
                std::string endTag = parseTagName();
                if (endTag != tagName) {
                    throw ParseException("Несоответствующий закрывающий тег", pos_);
                }
                expect(">");
                break;  // Выйти из цикла
            } else {
                // Дочерний элемент
                if (!textContent.empty()) {
                    // Сохранить текстовое содержимое
                    textContent.erase(0, textContent.find_first_not_of(" \t\n\r"));
                    textContent.erase(textContent.find_last_not_of(" \t\n\r") + 1);
                    if (!textContent.empty()) {
                        children.push_back(ASTNode::createString(textContent));
                    }
                    textContent.clear();
                }
                children.push_back(parseElement());  // Рекурсивно парсить дочерний элемент
            }
        } else {
            textContent += next();  // Собирать текстовое содержимое
        }
    }
    
    // Сохранить имя тега
    obj["@tag"] = ASTNode::createString(tagName);
    
    // Сохранить содержимое
    if (children.size() == 1 && children[0]->isString()) {
        obj["@text"] = children[0];  // Только текст, сохранить напрямую
    } else if (!children.empty()) {
        auto childrenArray = ASTNode::createArray();
        childrenArray->getArray() = children;
        obj["@children"] = childrenArray;  // Несколько дочерних узлов, сохранить как массив
    }
    
    return element;
}

std::string XMLParser::parseTagName() {
    skipWhitespace();
    std::string name;
    if (!isNameStartChar(peek())) {
        throw ParseException("Invalid tag name start character", pos_);
    }
    name += next();
    while (isNameChar(peek())) {
        name += next();
    }
    return name;
}

std::map<std::string, std::string> XMLParser::parseAttributes() {
    std::map<std::string, std::string> attrs;
    skipWhitespace();
    
    while (pos_ < input_.length() && peek() != '>' && peek() != '/' && 
           !(peek() == '/' && pos_ + 1 < input_.length() && input_[pos_ + 1] == '>')) {
        skipWhitespace();
        if (peek() == '>' || peek() == '/') break;
        
        std::string name = parseTagName();
        skipWhitespace();
        expect("=");
        skipWhitespace();
        std::string value = parseAttributeValue();
        attrs[name] = value;
        skipWhitespace();
    }
    
    return attrs;
}

std::string XMLParser::parseAttributeValue() {
    char quote = peek();
    if (quote != '"' && quote != '\'') {
        throw ParseException("Expected quote in attribute value", pos_);
    }
    pos_++;
    
    std::string value;
    while (pos_ < input_.length() && peek() != quote) {
        char c = next();
        if (c == '&') {
            // 实体引用
            std::string entity;
            while (pos_ < input_.length() && peek() != ';') {
                entity += next();
            }
            if (peek() == ';') {
                pos_++;
                if (entity == "lt") value += '<';
                else if (entity == "gt") value += '>';
                else if (entity == "amp") value += '&';
                else if (entity == "quot") value += '"';
                else if (entity == "apos") value += '\'';
                else value += '&' + entity + ';';
            } else {
                value += '&' + entity;
            }
        } else {
            value += c;
        }
    }
    
    if (peek() != quote) {
        throw ParseException("Unterminated attribute value", pos_);
    }
    pos_++;
    
    return value;
}

std::string XMLParser::parseText() {
    std::string text;
    while (pos_ < input_.length() && peek() != '<') {
        char c = next();
        if (c == '&') {
            // 实体引用
            std::string entity;
            while (pos_ < input_.length() && peek() != ';') {
                entity += next();
            }
            if (peek() == ';') {
                pos_++;
                if (entity == "lt") text += '<';
                else if (entity == "gt") text += '>';
                else if (entity == "amp") text += '&';
                else if (entity == "quot") text += '"';
                else if (entity == "apos") text += '\'';
                else text += '&' + entity + ';';
            } else {
                text += '&' + entity;
            }
        } else {
            text += c;
        }
    }
    return text;
}

std::string XMLParser::parseCDATA() {
    expect("<![CDATA[");
    std::string cdata;
    while (pos_ < input_.length()) {
        if (input_.substr(pos_, 3) == "]]>") {
            pos_ += 3;
            break;
        }
        cdata += next();
    }
    return cdata;
}

void XMLParser::parseComment() {
    expect("<!--");
    while (pos_ < input_.length()) {
        if (input_.substr(pos_, 3) == "-->") {
            pos_ += 3;
            return;
        }
        pos_++;
    }
    throw ParseException("Unterminated comment", pos_);
}

void XMLParser::parsePI() {
    expect("<?");
    while (pos_ < input_.length()) {
        if (input_.substr(pos_, 2) == "?>") {
            pos_ += 2;
            return;
        }
        pos_++;
    }
    throw ParseException("Unterminated processing instruction", pos_);
}

// XMLSerializer实现
std::string XMLSerializer::serialize(const std::shared_ptr<ASTNode>& node) {
    std::ostringstream oss;
    serialize(node, oss);
    return oss.str();
}

void XMLSerializer::serialize(const std::shared_ptr<ASTNode>& node, std::ostream& output) {
    if (!node->isObject()) {
        throw std::runtime_error("XML root must be an object");
    }
    output << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    serializeElement(node, output, 0);
}

void XMLSerializer::serializeElement(const std::shared_ptr<ASTNode>& node, std::ostream& output, int indent) {
    if (!node->isObject()) {
        serializeValue(node, output, indent);
        return;
    }
    
    const auto& obj = node->getObject();
    std::string tagName = "root";
    
    if (obj.find("@tag") != obj.end() && obj.at("@tag")->isString()) {
        tagName = obj.at("@tag")->getString();
    }
    
    output << indentString(indent) << "<" << tagName;
    
    // 序列化属性
    if (obj.find("@attributes") != obj.end()) {
        const auto& attrs = obj.at("@attributes")->getObject();
        for (const auto& [key, value] : attrs) {
            if (value->isString()) {
                output << " " << key << "=\"" << escapeXML(value->getString()) << "\"";
            }
        }
    }
    
    // 检查是否有内容
    bool hasText = obj.find("@text") != obj.end();
    bool hasChildren = obj.find("@children") != obj.end();
    
    if (!hasText && !hasChildren) {
        output << "/>\n";
        return;
    }
    
    output << ">";
    
    if (hasText && !hasChildren) {
        // 只有文本内容
        if (obj.at("@text")->isString()) {
            output << escapeXML(obj.at("@text")->getString());
        } else {
            serializeValue(obj.at("@text"), output, indent);
        }
    } else if (hasChildren) {
        // 有子元素
        output << "\n";
        const auto& children = obj.at("@children")->getArray();
        for (const auto& child : children) {
            serializeElement(child, output, indent + 1);
        }
        if (hasText && obj.at("@text")->isString()) {
            output << indentString(indent + 1) << escapeXML(obj.at("@text")->getString()) << "\n";
        }
        output << indentString(indent);
    }
    
    output << "</" << tagName << ">\n";
}

void XMLSerializer::serializeValue(const std::shared_ptr<ASTNode>& node, std::ostream& output, int indent) {
    if (node->isNull()) {
        output << "null";
    } else if (node->isBoolean()) {
        output << (node->getBoolean() ? "true" : "false");
    } else if (node->isNumber()) {
        output << node->getNumber();
    } else if (node->isString()) {
        output << escapeXML(node->getString());
    } else if (node->isArray()) {
        for (const auto& item : node->getArray()) {
            serializeValue(item, output, indent);
        }
    } else if (node->isObject()) {
        serializeElement(node, output, indent);
    }
}

std::string XMLSerializer::escapeXML(const std::string& str) {
    std::string result;
    for (char c : str) {
        switch (c) {
            case '<': result += "&lt;"; break;
            case '>': result += "&gt;"; break;
            case '&': result += "&amp;"; break;
            case '"': result += "&quot;"; break;
            case '\'': result += "&apos;"; break;
            default: result += c; break;
        }
    }
    return result;
}

std::string XMLSerializer::indentString(int indent) {
    return std::string(indent * 2, ' ');
}

