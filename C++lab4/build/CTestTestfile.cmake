# CMake generated Testfile for 
# Source directory: F:/C++lab4
# Build directory: F:/C++lab4/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test(FormatConverterTests "F:/C++lab4/build/Debug/test_format_converter.exe")
  set_tests_properties(FormatConverterTests PROPERTIES  WORKING_DIRECTORY "F:/C++lab4/build" _BACKTRACE_TRIPLES "F:/C++lab4/CMakeLists.txt;47;add_test;F:/C++lab4/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test(FormatConverterTests "F:/C++lab4/build/Release/test_format_converter.exe")
  set_tests_properties(FormatConverterTests PROPERTIES  WORKING_DIRECTORY "F:/C++lab4/build" _BACKTRACE_TRIPLES "F:/C++lab4/CMakeLists.txt;47;add_test;F:/C++lab4/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test(FormatConverterTests "F:/C++lab4/build/MinSizeRel/test_format_converter.exe")
  set_tests_properties(FormatConverterTests PROPERTIES  WORKING_DIRECTORY "F:/C++lab4/build" _BACKTRACE_TRIPLES "F:/C++lab4/CMakeLists.txt;47;add_test;F:/C++lab4/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test(FormatConverterTests "F:/C++lab4/build/RelWithDebInfo/test_format_converter.exe")
  set_tests_properties(FormatConverterTests PROPERTIES  WORKING_DIRECTORY "F:/C++lab4/build" _BACKTRACE_TRIPLES "F:/C++lab4/CMakeLists.txt;47;add_test;F:/C++lab4/CMakeLists.txt;0;")
else()
  add_test(FormatConverterTests NOT_AVAILABLE)
endif()
