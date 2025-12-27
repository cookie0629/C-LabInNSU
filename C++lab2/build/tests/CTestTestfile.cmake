# CMake generated Testfile for 
# Source directory: F:/C++lab2/tests
# Build directory: F:/C++lab2/build/tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test([=[money_rounding]=] "F:/C++lab2/build/tests/Debug/bank_tests.exe")
  set_tests_properties([=[money_rounding]=] PROPERTIES  _BACKTRACE_TRIPLES "F:/C++lab2/tests/CMakeLists.txt;9;add_test;F:/C++lab2/tests/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test([=[money_rounding]=] "F:/C++lab2/build/tests/Release/bank_tests.exe")
  set_tests_properties([=[money_rounding]=] PROPERTIES  _BACKTRACE_TRIPLES "F:/C++lab2/tests/CMakeLists.txt;9;add_test;F:/C++lab2/tests/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test([=[money_rounding]=] "F:/C++lab2/build/tests/MinSizeRel/bank_tests.exe")
  set_tests_properties([=[money_rounding]=] PROPERTIES  _BACKTRACE_TRIPLES "F:/C++lab2/tests/CMakeLists.txt;9;add_test;F:/C++lab2/tests/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test([=[money_rounding]=] "F:/C++lab2/build/tests/RelWithDebInfo/bank_tests.exe")
  set_tests_properties([=[money_rounding]=] PROPERTIES  _BACKTRACE_TRIPLES "F:/C++lab2/tests/CMakeLists.txt;9;add_test;F:/C++lab2/tests/CMakeLists.txt;0;")
else()
  add_test([=[money_rounding]=] NOT_AVAILABLE)
endif()
