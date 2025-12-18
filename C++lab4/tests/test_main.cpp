#include <iostream>
#include "test_json.h"
#include "test_toml.h"
#include "test_xml.h"
#include "test_converter.h"

int main() {
    int failures = 0;
    
    std::cout << "Running Format Converter Tests...\n" << std::endl;
    
    failures += testJSON();
    failures += testTOML();
    failures += testXML();
    failures += testConverter();
    
    if (failures == 0) {
        std::cout << "\nAll tests passed!" << std::endl;
        return 0;
    } else {
        std::cout << "\n" << failures << " test(s) failed!" << std::endl;
        return 1;
    }
}

