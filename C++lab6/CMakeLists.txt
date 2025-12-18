cmake_minimum_required(VERSION 3.16)

project(WarehouseSim LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

add_library(warehouse_lib
    src/RandomGenerator.cpp
    src/Warehouse.cpp
    src/Loader.cpp
    src/Manager.cpp
    src/TaskDispatcher.cpp
    src/ZoneResources.cpp
)

target_include_directories(warehouse_lib PUBLIC include)

add_executable(warehouse_app src/main.cpp)
target_link_libraries(warehouse_app PRIVATE warehouse_lib)

target_compile_definitions(warehouse_lib PRIVATE $<$<CONFIG:Debug>:WAREHOUSE_DEBUG>)

