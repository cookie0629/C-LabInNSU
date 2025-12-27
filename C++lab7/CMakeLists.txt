cmake_minimum_required(VERSION 3.15)
project(MessageBrokerLab7 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Third-party dependencies:
# - libzmq + cppzmq headers
# - yaml-cpp
# - nlohmann-json (header-only)
# If they are not installed system-wide, point CMAKE_PREFIX_PATH accordingly.
find_package(Threads REQUIRED)
find_package(ZeroMQ REQUIRED)
find_package(cppzmq REQUIRED)
find_package(yaml-cpp REQUIRED)

add_library(broker_core
    src/broker.cpp
)
target_include_directories(broker_core PUBLIC include)
target_link_libraries(broker_core PUBLIC cppzmq yaml-cpp Threads::Threads)

add_executable(broker_app src/main.cpp)
target_link_libraries(broker_app PRIVATE broker_core)

add_executable(client_demo src/client_demo.cpp)
target_link_libraries(client_demo PRIVATE broker_core)

install(TARGETS broker_app client_demo)


