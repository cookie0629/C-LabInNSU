add_executable(bank_tests
    test_money.cpp
)

target_include_directories(bank_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(bank_tests PRIVATE project_options)

add_test(NAME money_rounding COMMAND bank_tests)

