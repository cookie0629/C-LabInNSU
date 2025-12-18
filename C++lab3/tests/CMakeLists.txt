add_executable(regex_tests
    test_main.cpp
)

target_link_libraries(regex_tests PRIVATE regex_engine)

add_test(NAME regex_tests COMMAND regex_tests)


