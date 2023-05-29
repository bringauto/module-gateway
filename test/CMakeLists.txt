CMAKE_MINIMUM_REQUIRED(VERSION 3.25)
PROJECT(ModuleGateway)

SET(CMAKE_CXX_STANDARD 20)

FIND_PACKAGE(GTest REQUIRED)
INCLUDE_DIRECTORIES(${GTEST_INCLUDE_DIRS})
FILE(GLOB_RECURSE source_test_files "${CMAKE_CURRENT_LIST_DIR}/src/*" "${CMAKE_CURRENT_LIST_DIR}/src/testing_utils/*")
ADD_EXECUTABLE(modulegateway_tests ${source_test_files} ${CMAKE_CURRENT_LIST_DIR}/mainTests.cpp)
TARGET_INCLUDE_DIRECTORIES(modulegateway_tests PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include/")
TARGET_LINK_LIBRARIES(modulegateway_tests PUBLIC ${GTEST_LIBRARIES} pthread bringautolib button_module)
INCLUDE(GoogleTest)
GTEST_DISCOVER_TESTS(modulegateway_tests)
