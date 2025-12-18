# CMake generated Testfile for 
# Source directory: F:/C++lab3/tests
# Build directory: F:/C++lab3/build/tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test(regex_tests "F:/C++lab3/build/tests/Debug/regex_tests.exe")
  set_tests_properties(regex_tests PROPERTIES  _BACKTRACE_TRIPLES "F:/C++lab3/tests/CMakeLists.txt;7;add_test;F:/C++lab3/tests/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test(regex_tests "F:/C++lab3/build/tests/Release/regex_tests.exe")
  set_tests_properties(regex_tests PROPERTIES  _BACKTRACE_TRIPLES "F:/C++lab3/tests/CMakeLists.txt;7;add_test;F:/C++lab3/tests/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test(regex_tests "F:/C++lab3/build/tests/MinSizeRel/regex_tests.exe")
  set_tests_properties(regex_tests PROPERTIES  _BACKTRACE_TRIPLES "F:/C++lab3/tests/CMakeLists.txt;7;add_test;F:/C++lab3/tests/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test(regex_tests "F:/C++lab3/build/tests/RelWithDebInfo/regex_tests.exe")
  set_tests_properties(regex_tests PROPERTIES  _BACKTRACE_TRIPLES "F:/C++lab3/tests/CMakeLists.txt;7;add_test;F:/C++lab3/tests/CMakeLists.txt;0;")
else()
  add_test(regex_tests NOT_AVAILABLE)
endif()
