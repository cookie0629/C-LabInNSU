add_library(libcompressor STATIC
  src/libcompressor.cpp
)
target_include_directories(libcompressor PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_link_libraries(libcompressor PUBLIC ZLIB::ZLIB BZip2::BZip2)

if (BUILD_TESTING)
  find_package(GTest REQUIRED)
  add_executable(libcompressor_tests tests/libcompressortest.cpp)
  target_link_libraries(libcompressor_tests PRIVATE libcompressor GTest::gtest_main spdlog::spdlog)
  add_test(NAME libcompressor_tests
           COMMAND libcompressor_tests --gtest_output=xml:${CMAKE_BINARY_DIR}/test_results.xml)
endif()

include(GNUInstallDirs)
install(TARGETS libcompressor
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
