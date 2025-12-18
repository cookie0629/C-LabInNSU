#include "toml_parser.h"
#include <sstream>
#include <cctype>
#include <iomanip>
#include <algorithm>

// Процесс парсинга TOML
std::shared_ptr<ASTNode> TOMLParser::parse(const std::string& input) {
    input_ = input;
    pos_ = 0;
    auto root = ASTNode::createObject();  // Корень TOML должен быть объектом
    
    while (pos_ < input_.length()) {
        skipWhitespaceAndComments();  // Пропустить пробелы и комментарии
        
        if (pos_ >= input_.length() || isEndOfLine()) {
            if (pos_ < input_.length()) pos_++;
            continue;  // Пропустить пустые строки
        }
        
        char c = peek();
        if (c == '[') {
            pos_++;
            if (peek() == '[') {
                parseArrayTable(root);  // [[array]] таблица массивов
            } else {
                parseTable(root);       // [table] обычная таблица
            }
        } else {
            parseKeyValue(root);         // key = value пара ключ-значение
        }
        
        skipWhitespaceAndComments();
        if (pos_ < input_.length() && !isEndOfLine()) {
            throw ParseException("Ожидался перевод строки после оператора", pos_);
        }
        if (pos_ < input_.length()) pos_++;
    }
    
    return root;
}

std::shared_ptr<ASTNode> TOMLParser::parse(std::istream& input) {
    std::stringstream ss;
    ss << input.rdbuf();
    return parse(ss.str());
}

void TOMLParser::skipWhitespace() {
    while (pos_ < input_.length() && (input_[pos_] == ' ' || input_[pos_] == '\t')) {
        pos_++;
    }
}

void TOMLParser::skipComment() {
    if (pos_ < input_.length() && input_[pos_] == '#') {
        while (pos_ < input_.length() && input_[pos_] != '\n' && input_[pos_] != '\r') {
            pos_++;
        }
    }
}

void TOMLParser::skipWhitespaceAndComments() {
    while (pos_ < input_.length()) {
        size_t oldPos = pos_;
        skipWhitespace();
        skipComment();
        if (pos_ == oldPos) break;
    }
}

char TOMLParser::peek() {
    if (pos_ >= input_.length()) {
        return '\0';
    }
    return input_[pos_];
}

char TOMLParser::next() {
    char c = peek();
    if (pos_ < input_.length()) pos_++;
    return c;
}

bool TOMLParser::expect(char c) {
    skipWhitespace();
    if (peek() != c) {
        throw ParseException("Expected '" + std::string(1, c) + "'", pos_);
    }
    pos_++;
    return true;
}

bool TOMLParser::isEndOfLine() {
    return peek() == '\n' || peek() == '\r';
}

std::shared_ptr<ASTNode> TOMLParser::parseValue() {
    skipWhitespace();
    char c = peek();
    
    if (c == '"' || c == '\'') {
        return parseString();
    } else if (c == '[') {
        return parseArray();
    } else if (c == '{') {
        return parseInlineTable();
    } else if (c == 't' || c == 'f') {
        return parseBoolean();
    } else if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) {
        return parseNumber();
    } else {
        throw ParseException("Unexpected character in value: " + std::string(1, c), pos_);
    }
}

std::shared_ptr<ASTNode> TOMLParser::parseString() {
    if (peek() == '"') {
        std::string value = parseBasicString();
        return ASTNode::createString(value);
    } else if (peek() == '\'') {
        std::string value = parseLiteralString();
        return ASTNode::createString(value);
    }
    throw ParseException("Expected string", pos_);
}

std::string TOMLParser::parseBasicString() {
    expect('"');
    std::string result;
    while (pos_ < input_.length()) {
        char c = next();
        if (c == '"') {
            return result;
        } else if (c == '\\') {
            if (pos_ >= input_.length()) {
                throw ParseException("Incomplete escape sequence", pos_);
            }
            char esc = next();
            switch (esc) {
                case '"': result += '"'; break;
                case '\\': result += '\\'; break;
                case 'b': result += '\b'; break;
                case 'f': result += '\f'; break;
                case 'n': result += '\n'; break;
                case 'r': result += '\r'; break;
                case 't': result += '\t'; break;
                default: result += esc; break;
            }
        } else {
            result += c;
        }
    }
    throw ParseException("Unterminated string", pos_);
}

std::string TOMLParser::parseLiteralString() {
    expect('\'');
    std::string result;
    while (pos_ < input_.length()) {
        char c = next();
        if (c == '\'') {
            return result;
        }
        result += c;
    }
    throw ParseException("Unterminated literal string", pos_);
}

std::shared_ptr<ASTNode> TOMLParser::parseNumber() {
    size_t start = pos_;
    bool negative = false;
    
    if (peek() == '-') {
        negative = true;
        pos_++;
    }
    
    if (!std::isdigit(static_cast<unsigned char>(peek()))) {
        throw ParseException("Expected digit", pos_);
    }
    
    while (pos_ < input_.length() && std::isdigit(static_cast<unsigned char>(input_[pos_]))) {
        pos_++;
    }
    
    if (pos_ < input_.length() && input_[pos_] == '.') {
        pos_++;
        if (!std::isdigit(static_cast<unsigned char>(peek()))) {
            throw ParseException("Expected digit after decimal point", pos_);
        }
        while (pos_ < input_.length() && std::isdigit(static_cast<unsigned char>(input_[pos_]))) {
            pos_++;
        }
    }
    
    if (pos_ < input_.length() && (input_[pos_] == 'e' || input_[pos_] == 'E')) {
        pos_++;
        if (pos_ < input_.length() && (input_[pos_] == '+' || input_[pos_] == '-')) {
            pos_++;
        }
        if (!std::isdigit(static_cast<unsigned char>(peek()))) {
            throw ParseException("Expected digit in exponent", pos_);
        }
        while (pos_ < input_.length() && std::isdigit(static_cast<unsigned char>(input_[pos_]))) {
            pos_++;
        }
    }
    
    std::string numStr = input_.substr(start, pos_ - start);
    double value = std::stod(numStr);
    
    return ASTNode::createNumber(value);
}

std::shared_ptr<ASTNode> TOMLParser::parseBoolean() {
    if (input_.substr(pos_, 4) == "true") {
        pos_ += 4;
        return ASTNode::createBoolean(true);
    } else if (input_.substr(pos_, 5) == "false") {
        pos_ += 5;
        return ASTNode::createBoolean(false);
    }
    throw ParseException("Expected 'true' or 'false'", pos_);
}

std::shared_ptr<ASTNode> TOMLParser::parseDateTime() {
    // 简化实现：作为字符串处理
    size_t start = pos_;
    while (pos_ < input_.length() && !std::isspace(static_cast<unsigned char>(input_[pos_])) 
           && input_[pos_] != ',' && input_[pos_] != ']' && input_[pos_] != '}') {
        pos_++;
    }
    std::string value = input_.substr(start, pos_ - start);
    return ASTNode::createString(value);
}

std::shared_ptr<ASTNode> TOMLParser::parseArray() {
    expect('[');
    auto array = ASTNode::createArray();
    auto& elements = array->getArray();
    
    skipWhitespaceAndComments();
    if (peek() == ']') {
        pos_++;
        return array;
    }
    
    while (true) {
        elements.push_back(parseValue());
        skipWhitespaceAndComments();
        if (peek() == ']') {
            pos_++;
            break;
        }
        expect(',');
        skipWhitespaceAndComments();
    }
    
    return array;
}

std::shared_ptr<ASTNode> TOMLParser::parseInlineTable() {
    expect('{');
    auto object = ASTNode::createObject();
    auto& members = object->getObject();
    
    skipWhitespace();
    if (peek() == '}') {
        pos_++;
        return object;
    }
    
    while (true) {
        std::string key = parseKey();
        skipWhitespace();
        expect('=');
        members[key] = parseValue();
        skipWhitespace();
        if (peek() == '}') {
            pos_++;
            break;
        }
        expect(',');
        skipWhitespace();
    }
    
    return object;
}

std::string TOMLParser::parseKey() {
    skipWhitespace();
    if (peek() == '"') {
        return parseBasicString();
    } else if (peek() == '\'') {
        return parseLiteralString();
    } else {
        std::string key;
        while (pos_ < input_.length() && 
               (std::isalnum(static_cast<unsigned char>(input_[pos_])) || 
                input_[pos_] == '_' || input_[pos_] == '-')) {
            key += next();
        }
        if (key.empty()) {
            throw ParseException("Expected key", pos_);
        }
        return key;
    }
}

std::vector<std::string> TOMLParser::parseKeyPath() {
    std::vector<std::string> path;
    while (true) {
        path.push_back(parseKey());
        skipWhitespace();
        if (peek() == '.' && pos_ + 1 < input_.length() && input_[pos_ + 1] != '.') {
            pos_++;
            skipWhitespace();
        } else {
            break;
        }
    }
    return path;
}

void TOMLParser::parseKeyValue(std::shared_ptr<ASTNode>& root) {
    std::vector<std::string> path = parseKeyPath();  // Распарсить путь ключа (может содержать точки)
    skipWhitespace();
    expect('=');
    auto value = parseValue();  // Распарсить значение
    setNestedValue(root, path, value);  // Установить вложенное значение
}
void TOMLParser::parseTable(std::shared_ptr<ASTNode>& root) {
    std::vector<std::string> path = parseKeyPath();
    skipWhitespace();
    expect(']');
    
    setNestedValue(root, path, ASTNode::createObject());
}

void TOMLParser::parseArrayTable(std::shared_ptr<ASTNode>& root) {
    expect('[');
    std::vector<std::string> path = parseKeyPath();
    skipWhitespace();
    expect(']');
    expect(']');
    
    // 找到或创建数组
    std::shared_ptr<ASTNode> current = root;
    for (size_t i = 0; i < path.size() - 1; i++) {
        auto& obj = current->getObject();
        if (obj.find(path[i]) == obj.end()) {
            obj[path[i]] = ASTNode::createObject();
        }
        current = obj[path[i]];
    }
    
    std::string lastKey = path.back();
    auto& obj = current->getObject();
    if (obj.find(lastKey) == obj.end()) {
        obj[lastKey] = ASTNode::createArray();
    }
    
    auto& arr = obj[lastKey]->getArray();
    arr.push_back(ASTNode::createObject());
}

void TOMLParser::setNestedValue(std::shared_ptr<ASTNode>& root, 
                                 const std::vector<std::string>& path, 
                                 std::shared_ptr<ASTNode> value) {
    std::shared_ptr<ASTNode> current = root;
    
    // Пройти по пути (кроме последней части)
    for (size_t i = 0; i < path.size() - 1; i++) {
        auto& obj = current->getObject();
        // Если путь не существует, создать новый объект
        if (obj.find(path[i]) == obj.end()) {
            obj[path[i]] = ASTNode::createObject();
        }
        current = obj[path[i]];  // Перейти на следующий уровень
    }
    
    // Установить значение для последнего ключа
    std::string lastKey = path.back();
    current->getObject()[lastKey] = value;
}

// TOMLSerializer实现
std::string TOMLSerializer::serialize(const std::shared_ptr<ASTNode>& node) {
    std::ostringstream oss;
    serialize(node, oss);
    return oss.str();
}

void TOMLSerializer::serialize(const std::shared_ptr<ASTNode>& node, std::ostream& output) {
    if (!node->isObject()) {
        throw std::runtime_error("TOML root must be an object");
    }
    serializeObject(node, output);
}

void TOMLSerializer::serializeValue(const std::shared_ptr<ASTNode>& node, std::ostream& output, int indent) {
    if (node->isNull()) {
        output << "null";
    } else if (node->isBoolean()) {
        output << (node->getBoolean() ? "true" : "false");
    } else if (node->isNumber()) {
        double num = node->getNumber();
        if (num == static_cast<long long>(num)) {
            output << static_cast<long long>(num);
        } else {
            output << std::fixed << std::setprecision(15) << num;
        }
    } else if (node->isString()) {
        serializeString(node->getString(), output);
    } else if (node->isArray()) {
        output << "[";
        const auto& arr = node->getArray();
        for (size_t i = 0; i < arr.size(); i++) {
            if (i > 0) output << ", ";
            serializeValue(arr[i], output, indent);
        }
        output << "]";
    } else if (node->isObject()) {
        output << "{";
        const auto& obj = node->getObject();
        bool first = true;
        for (const auto& [key, value] : obj) {
            if (!first) output << ", ";
            first = false;
            serializeString(key, output);
            output << " = ";
            serializeValue(value, output, indent);
        }
        output << "}";
    }
}

void TOMLSerializer::serializeString(const std::string& str, std::ostream& output) {
    // 检查是否需要引号
    bool needsQuote = false;
    for (char c : str) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '-') {
            needsQuote = true;
            break;
        }
    }
    
    if (needsQuote || str.empty()) {
        output << "\"" << escapeString(str) << "\"";
    } else {
        output << str;
    }
}

void TOMLSerializer::serializeObject(const std::shared_ptr<ASTNode>& node, std::ostream& output, const std::string& prefix, int indent) {
    const auto& obj = node->getObject();
    bool first = true;
    
    for (const auto& [key, value] : obj) {
        if (!first) output << "\n";
        first = false;
        
        std::string fullKey = prefix.empty() ? key : prefix + "." + key;
        
        if (value->isObject() && !value->getObject().empty()) {
            output << "[" << fullKey << "]\n";
            serializeObject(value, output, fullKey, indent + 1);
        } else if (value->isArray() && !value->getArray().empty() && value->getArray()[0]->isObject()) {
            for (const auto& item : value->getArray()) {
                output << "[[" << fullKey << "]]\n";
                serializeObject(item, output, fullKey, indent + 1);
            }
        } else {
            serializeString(key, output);
            output << " = ";
            serializeValue(value, output, indent);
        }
    }
}

std::string TOMLSerializer::escapeString(const std::string& str) {
    std::string result;
    for (char c : str) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default: result += c; break;
        }
    }
    return result;
}

