add_library(regex_engine
    regex_engine.cpp
    regex_engine.hpp
)

target_include_directories(regex_engine PUBLIC ${CMAKE_CURRENT_LIST_DIR})

add_executable(regex_cli main.cpp)
target_link_libraries(regex_cli PRIVATE regex_engine)


