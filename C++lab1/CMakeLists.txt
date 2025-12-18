cmake_minimum_required(VERSION 3.21)
project(modern_cpp_task LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS YES)

include(CTest)

if (MSVC)
  add_compile_options(/W4 /WX)
else()
  add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

include(${CMAKE_BINARY_DIR}/conan_toolchain.cmake OPTIONAL) 
find_package(ZLIB REQUIRED)
find_package(BZip2 REQUIRED)
find_package(spdlog REQUIRED)

add_subdirectory(libcompressor)
add_subdirectory(compressor)

find_package(Doxygen REQUIRED)
set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/docs)
set(DOXYGEN_EXTRACT_ALL YES)
doxygen_add_docs(
  docs
  libcompressor/include
  COMMENT "Generate API docs"
)

file(GLOB_RECURSE ALL_CXX
  libcompressor/include/*.hpp
  libcompressor/src/*.cpp
  compressor/src/*.cpp)
add_custom_target(format
  COMMAND clang-format -i ${ALL_CXX}
  COMMENT "Run clang-format")
add_custom_target(tidy
  COMMAND clang-tidy -p ${CMAKE_BINARY_DIR}
          --header-filter='^(libcompressor|compressor)/.*'
          --extra-arg-before=--gcc-toolchain=/usr
          --extra-arg=-stdlib=libstdc++
          --extra-arg=-I/usr/include/c++/11
          --extra-arg=-I/usr/include/x86_64-linux-gnu/c++/11
          --extra-arg=-I/usr/include/x86_64-linux-gnu
          ${ALL_CXX}
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  COMMENT "Run clang-tidy on project sources")
