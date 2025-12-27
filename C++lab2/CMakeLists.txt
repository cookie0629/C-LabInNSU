cmake_minimum_required(VERSION 3.20)

project(BankBranchSimulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
    add_compile_options(/W4 /permissive- /EHsc)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_subdirectory(external)

add_executable(bank_sim
    src/main.cpp
    src/Bank.cpp
)

target_include_directories(bank_sim PRIVATE include)

target_link_libraries(bank_sim PRIVATE project_options)

enable_testing()
add_subdirectory(tests)

