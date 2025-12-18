add_executable(compressor src/compressor.cpp)
target_link_libraries(compressor PRIVATE libcompressor spdlog::spdlog)

include(GNUInstallDirs)
install(TARGETS compressor RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
