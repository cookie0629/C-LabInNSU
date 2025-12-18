#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include <string>
#include <memory>
#include <stdexcept>

/*
На этапе парсинга ошибки синтаксиса сообщаются через 
пользовательский класс исключений ParseException 
(унаследованный от std::runtime_error) с записью позиции возникновения ошибки.
*/
class ParseException : public std::runtime_error {
public:
    ParseException(const std::string& message, size_t position = 0)
        : std::runtime_error(message), position_(position) {}
    
    size_t getPosition() const { return position_; }

private:
    size_t position_;
};

template<typename T>
class Parser {
public:
    virtual ~Parser() = default;
    
    // Парсинг из строки
    virtual std::shared_ptr<ASTNode> parse(const std::string& input) = 0;//dолжна быть реализована в производном классе
    
    // Парсинг из входного потока
    virtual std::shared_ptr<ASTNode> parse(std::istream& input) = 0;
};

// Базовый класс сериализатора (шаблонный интерфейс)
// Сериализатор выполняет обратную парсеру функцию: преобразует AST в строковый формат.
template<typename T>
class Serializer {
public:
    virtual ~Serializer() = default;
    
    // Сериализация в строку
    virtual std::string serialize(const std::shared_ptr<ASTNode>& node) = 0;
    
    // Сериализация в выходной поток
    virtual void serialize(const std::shared_ptr<ASTNode>& node, std::ostream& output) = 0;
};

#endif

