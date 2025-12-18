#ifndef AST_H
#define AST_H

#include <string>
#include <vector>
#include <map>
#include <variant>
#include <memory>
#include <stdexcept>

// AST节点类型枚举
enum class ASTType {
    Null,
    Boolean,
    Number,
    String,
    Array,
    Object
};

// 前向声明
class ASTNode;

// AST节点值类型
using ASTValue = std::variant<
    std::nullptr_t,
    bool,
    double,
    std::string,
    std::vector<std::shared_ptr<ASTNode>>,
    std::map<std::string, std::shared_ptr<ASTNode>>
>;

// AST节点类
class ASTNode {
public:
    // Конструктор по умолчанию: создает узел Null
    ASTNode() : type_(ASTType::Null), value_(nullptr) {}
    
    explicit ASTNode(ASTType type) : type_(type) {
        switch (type) {
            case ASTType::Null:
                value_ = nullptr;
                break;
            case ASTType::Boolean:
                value_ = false;
                break;
            case ASTType::Number:
                value_ = 0.0;
                break;
            case ASTType::String:
                value_ = std::string("");
                break;
            case ASTType::Array:
                value_ = std::vector<std::shared_ptr<ASTNode>>();
                break;
            case ASTType::Object:
                value_ = std::map<std::string, std::shared_ptr<ASTNode>>();
                break;
        }
    }
    
    ASTType getType() const { return type_; }
    
    // Шаблонная функция, T может быть любым типом из variant
    template<typename T>
    T getValue() const {
        return std::get<T>(value_);
    }
    
    // 模板方法设置值
    template<typename T>
    void setValue(const T& val) {
        value_ = val;
    }
    
    bool isNull() const { return type_ == ASTType::Null; }
    bool isBoolean() const { return type_ == ASTType::Boolean; }
    bool isNumber() const { return type_ == ASTType::Number; }
    bool isString() const { return type_ == ASTType::String; }
    bool isArray() const { return type_ == ASTType::Array; }
    bool isObject() const { return type_ == ASTType::Object; }
    
    // 获取布尔值
    bool getBoolean() const {
        if (type_ != ASTType::Boolean) {
            throw std::runtime_error("Node is not a boolean");
        }
        return std::get<bool>(value_);
    }
    
    // 获取数字
    double getNumber() const {
        if (type_ != ASTType::Number) {
            throw std::runtime_error("Node is not a number");
        }
        return std::get<double>(value_);
    }
    
    // 获取字符串
    std::string getString() const {
        if (type_ != ASTType::String) {
            throw std::runtime_error("Node is not a string");
        }
        return std::get<std::string>(value_);
    }
    
    // 获取数组
    std::vector<std::shared_ptr<ASTNode>>& getArray() {
        if (type_ != ASTType::Array) {
            throw std::runtime_error("Node is not an array");
        }
        return std::get<std::vector<std::shared_ptr<ASTNode>>>(value_);
    }
    
    const std::vector<std::shared_ptr<ASTNode>>& getArray() const {
        if (type_ != ASTType::Array) {
            throw std::runtime_error("Node is not an array");
        }
        return std::get<std::vector<std::shared_ptr<ASTNode>>>(value_);
    }
    
    // 获取对象
    std::map<std::string, std::shared_ptr<ASTNode>>& getObject() {
        if (type_ != ASTType::Object) {
            throw std::runtime_error("Node is not an object");
        }
        return std::get<std::map<std::string, std::shared_ptr<ASTNode>>>(value_);
    }
    
    const std::map<std::string, std::shared_ptr<ASTNode>>& getObject() const {
        if (type_ != ASTType::Object) {
            throw std::runtime_error("Node is not an object");
        }
        return std::get<std::map<std::string, std::shared_ptr<ASTNode>>>(value_);
    }
    
    //Фабричный метод: предоставляет единый интерфейс создания, упрощает создание узлов
    static std::shared_ptr<ASTNode> createNull() {
        auto node = std::make_shared<ASTNode>(ASTType::Null);
        return node;
    }
    
    static std::shared_ptr<ASTNode> createBoolean(bool val) {
        auto node = std::make_shared<ASTNode>(ASTType::Boolean);
        node->setValue<bool>(val);
        return node;
    }
    
    static std::shared_ptr<ASTNode> createNumber(double val) {
        auto node = std::make_shared<ASTNode>(ASTType::Number);
        node->setValue<double>(val);
        return node;
    }
    // Фабричный метод для создания строкового узла
    static std::shared_ptr<ASTNode> createString(const std::string& val) {
        auto node = std::make_shared<ASTNode>(ASTType::String);
        node->setValue<std::string>(val);
        return node;
    }
    
    static std::shared_ptr<ASTNode> createArray() {
        return std::make_shared<ASTNode>(ASTType::Array);
    }
    
    static std::shared_ptr<ASTNode> createObject() {
        return std::make_shared<ASTNode>(ASTType::Object);
    }

private:
    ASTType type_;
    ASTValue value_;
};

#endif // AST_H

