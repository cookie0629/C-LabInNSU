#include "converter.h"
#include "parser.h"
#include <iostream>
#include <string>
#include <sstream>

int main(int argc, char* argv[]) {
    // 1. Проверить аргументы командной строки
    if (argc != 3) {
        std::cerr << "Usage: " << (argc > 0 ? argv[0] : "converter") 
                  << " <input_format> <output_format>" << std::endl;
        std::cerr << "Supported formats: json, toml, xml" << std::endl;
        return 1;
    }
    
    // 2. Распарсить входной и выходной форматы
    Format inputFormat = parseFormat(argv[1]);
    Format outputFormat = parseFormat(argv[2]);
    
    // 3. Проверить форматы
    if (inputFormat == Format::UNKNOWN) {
        std::cerr << "Error: Unknown input format: " << argv[1] << std::endl;
        std::cerr << "Supported formats: json, toml, xml" << std::endl;
        return 1;
    }
    
    if (outputFormat == Format::UNKNOWN) {
        std::cerr << "Error: Unknown output format: " << argv[2] << std::endl;
        std::cerr << "Supported formats: json, toml, xml" << std::endl;
        return 1;
    }
    /*
    На этапе выполнения в main используется единый блок try-catch: 
    перехватываются отдельно ParseException и общие исключения std::exception, 
    после чего информация об ошибке выводится в std::cerr
    */
    try {
        // 4. Прочитать ввод из stdin
        std::string input;
        std::string line;
        while (std::getline(std::cin, line)) {
            if (!input.empty()) {
                input += "\n";
            }
            input += line;
        }
        
        // 5. Удалить возможную метку BOM (UTF-8 BOM)
        if (input.length() >= 3 && 
            static_cast<unsigned char>(input[0]) == 0xEF &&
            static_cast<unsigned char>(input[1]) == 0xBB &&
            static_cast<unsigned char>(input[2]) == 0xBF) {
            input = input.substr(3);
        }
        
        // 6. 使用模板化转换器完成解析与序列化
        std::string output = convertDocument(inputFormat, outputFormat, input);
        
        // 7. Вывести в stdout
        std::cout << output;
        
        return 0;
    } catch (const ParseException& e) {
        std::cerr << "Parse error: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}

