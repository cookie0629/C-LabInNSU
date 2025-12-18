#include "json_parser.h"
#include <sstream>
#include <cctype>
#include <iomanip>

std::shared_ptr<ASTNode> JSONParser::parse(const std::string& input) {
    input_ = input;
    pos_ = 0;
    skipWhitespace();

    // Распарсить JSON значение
    auto result = parseValue();
    skipWhitespace();
    if (pos_ < input_.length()) {
        throw ParseException("Unexpected characters after JSON value", pos_);
    }
    return result;
}

std::shared_ptr<ASTNode> JSONParser::parse(std::istream& input) {
    std::stringstream ss;
    ss << input.rdbuf();
    return parse(ss.str()); //调用字符串版本的 parse 方法
}

void JSONParser::skipWhitespace() {
    while (pos_ < input_.length() && std::isspace(static_cast<unsigned char>(input_[pos_]))) {
        pos_++;
    }
}

char JSONParser::peek() {
    if (pos_ >= input_.length()) {
        throw ParseException("Unexpected end of input", pos_);
    }
    return input_[pos_];
}

char JSONParser::next() {
    char c = peek();
    pos_++;
    return c;
}

bool JSONParser::expect(char c) {
    skipWhitespace();
    if (peek() != c) {
        throw ParseException("Expected '" + std::string(1, c) + "'", pos_);
    }
    pos_++;
    return true;
}

// Парсинг значения (рекурсивная точка входа)
std::shared_ptr<ASTNode> JSONParser::parseValue() {
    skipWhitespace();
    char c = peek();
    
    if (c == 'n') {
        return parseNull();
    } else if (c == 't' || c == 'f') {
        return parseBoolean();
    } else if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) {
        return parseNumber();
    } else if (c == '"') {
        return parseString();
    } else if (c == '[') {
        return parseArray();
    } else if (c == '{') {
        return parseObject();
    } else {
        throw ParseException("Unexpected character: " + std::string(1, c), pos_);
    }
}

std::shared_ptr<ASTNode> JSONParser::parseNull() {
    if (input_.substr(pos_, 4) == "null") {
        pos_ += 4;
        return ASTNode::createNull();
    }
    throw ParseException("Expected 'null'", pos_);
}

std::shared_ptr<ASTNode> JSONParser::parseBoolean() {
    if (input_.substr(pos_, 4) == "true") {
        pos_ += 4;
        return ASTNode::createBoolean(true);
    } else if (input_.substr(pos_, 5) == "false") {
        pos_ += 5;
        return ASTNode::createBoolean(false);
    }
    throw ParseException("Expected 'true' or 'false'", pos_);
}

std::shared_ptr<ASTNode> JSONParser::parseNumber() {
    size_t start = pos_;
    bool negative = false;
    
    if (peek() == '-') {
        negative = true;
        pos_++;
    }
    
    // Парсинг целой части
    if (!std::isdigit(static_cast<unsigned char>(peek()))) {
        throw ParseException("Expected digit", pos_);
    }
    while (pos_ < input_.length() && std::isdigit(static_cast<unsigned char>(input_[pos_]))) {
        pos_++;
    }
    
    // Парсинг дробной части (если есть)
    if (pos_ < input_.length() && input_[pos_] == '.') {
        pos_++;
        if (!std::isdigit(static_cast<unsigned char>(peek()))) {
            throw ParseException("Expected digit after decimal point", pos_);
        }
        while (pos_ < input_.length() && std::isdigit(static_cast<unsigned char>(input_[pos_]))) {
            pos_++;
        }
    }
    
    // Парсинг экспоненциальной части (если есть, напр. 1.23e-4)
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
    
    // Извлечь числовую строку и преобразовать в double
    std::string numStr = input_.substr(start, pos_ - start);
    double value = std::stod(numStr);
    
    return ASTNode::createNumber(value);
}


// Парсинг строки 
std::shared_ptr<ASTNode> JSONParser::parseString() {
    expect('"');
    std::string value = parseStringValue();
    expect('"');
    return ASTNode::createString(value);
}

std::string JSONParser::parseStringValue() {
    std::string result;
    while (pos_ < input_.length()) {
        char c = next();
        if (c == '"') {
            pos_--; // Отступить назад, чтобы вызывающий метод обработал кавычку
            break;
        } else if (c == '\\') {
            //Обработка escape-последовательности
            if (pos_ >= input_.length()) {
                throw ParseException("Incomplete escape sequence", pos_);
            }
            char esc = next();
            switch (esc) {
                case '"': result += '"'; break;
                case '\\': result += '\\'; break;
                case '/': result += '/'; break;
                case 'b': result += '\b'; break;
                case 'f': result += '\f'; break;
                case 'n': result += '\n'; break;
                case 'r': result += '\r'; break;
                case 't': result += '\t'; break;
                case 'u': result += parseUnicode(); break;
                default: throw ParseException("Invalid escape sequence", pos_ - 1);
            }
        } else {
            result += c;
        }
    }
    return result;
}

// Парсит Unicode escape-последовательности
std::string JSONParser::parseUnicode() {
    std::string hex;
    for (int i = 0; i < 4; i++) {
        if (pos_ >= input_.length()) {
            throw ParseException("Incomplete unicode escape", pos_);
        }
        char c = next();
        if (!std::isxdigit(static_cast<unsigned char>(c))) {
            throw ParseException("Invalid hex digit in unicode escape", pos_ - 1);
        }
        hex += c;
    }
    unsigned int code = std::stoul(hex, nullptr, 16);
    if (code <= 0x7F) {
        return std::string(1, static_cast<char>(code));
    } else if (code <= 0x7FF) {
        return std::string(1, static_cast<char>(0xC0 | (code >> 6))) +
               std::string(1, static_cast<char>(0x80 | (code & 0x3F)));
    } else {
        return std::string(1, static_cast<char>(0xE0 | (code >> 12))) +
               std::string(1, static_cast<char>(0x80 | ((code >> 6) & 0x3F))) +
               std::string(1, static_cast<char>(0x80 | (code & 0x3F)));
    }
}

// Парсинг массива
std::shared_ptr<ASTNode> JSONParser::parseArray() {
    expect('[');
    auto array = ASTNode::createArray();
    //Получает ссылку на массив, можно напрямую изменять
    auto& elements = array->getArray();
    
    skipWhitespace();
    if (peek() == ']') {
        pos_++;
        return array;
    }
    
    while (true) {
        elements.push_back(parseValue());
        skipWhitespace();
        if (peek() == ']') {
            pos_++;
            break;
        }
        expect(',');
    }
    
    return array;
}

// Парсинг объекта
std::shared_ptr<ASTNode> JSONParser::parseObject() {
    expect('{');
    auto object = ASTNode::createObject();
    auto& members = object->getObject();
    
    skipWhitespace();
    if (peek() == '}') {
        pos_++;
        return object;
    }
    
    while (true) {
        skipWhitespace();
        expect('"');
        std::string key = parseStringValue();
        expect('"');
        skipWhitespace();
        expect(':');
        // Сохраняет пару ключ-значение в map
        members[key] = parseValue();
        skipWhitespace();
        if (peek() == '}') {
            pos_++;
            break;
        }
        expect(',');
    }
    
    return object;
}

// JSONSerializer实现
std::string JSONSerializer::serialize(const std::shared_ptr<ASTNode>& node) {
    std::ostringstream oss;
    serialize(node, oss);
    return oss.str();
}

void JSONSerializer::serialize(const std::shared_ptr<ASTNode>& node, std::ostream& output) {
    serializeValue(node, output, 0);
}

// JSON сериализатор
void JSONSerializer::serializeValue(const std::shared_ptr<ASTNode>& node, 
                                   std::ostream& output, int indent) {
    if (node->isNull()) {
        output << "null";
    } else if (node->isBoolean()) {
        output << (node->getBoolean() ? "true" : "false");
    } else if (node->isNumber()) {
        double num = node->getNumber();
        // Если целое число, вывести в целочисленном формате; иначе вывести число с плавающей точкой
        if (num == static_cast<long long>(num)) {
            output << static_cast<long long>(num);
        } else {
            output << std::fixed << std::setprecision(15) << num;
        }
    } else if (node->isString()) {
        serializeString(node->getString(), output);  // Нужно экранировать специальные символы
    } else if (node->isArray()) {
        output << "[";
        const auto& arr = node->getArray();
        for (size_t i = 0; i < arr.size(); i++) {
            if (i > 0) output << ",";  // Разделять элементы запятой
            serializeValue(arr[i], output, indent);  // Рекурсивно сериализовать элемент
        }
        output << "]";
    } else if (node->isObject()) {
        output << "{";
        const auto& obj = node->getObject();
        bool first = true;
        for (const auto& [key, value] : obj) {
            if (!first) output << ",";
            first = false;
            serializeString(key, output);  // Сериализовать ключ
            output << ":";
            serializeValue(value, output, indent);  // Рекурсивно сериализовать значение
        }
        output << "}";
    }
}

void JSONSerializer::serializeString(const std::string& str, std::ostream& output) {
    output << "\"" << escapeString(str) << "\"";
}

std::string JSONSerializer::escapeString(const std::string& str) {
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
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    result += "\\u";
                    result += "0000";
                    char hex[3];
                    #ifdef _MSC_VER
                    sprintf_s(hex, sizeof(hex), "%02x", static_cast<unsigned char>(c));
                    #else
                    std::sprintf(hex, "%02x", static_cast<unsigned char>(c));
                    #endif
                    result += hex;
                } else {
                    result += c;
                }
                break;
        }
    }
    return result;
}

