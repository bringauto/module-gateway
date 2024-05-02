CMAKE_MINIMUM_REQUIRED(VERSION 3.25)
PROJECT(ModuleGateway)

SET(CMAKE_CXX_STANDARD 20)

ADD_SUBDIRECTORY("${CMAKE_CURRENT_LIST_DIR}/lib/example-module")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DDEBUG)
endif()

FIND_PACKAGE(GTest REQUIRED)
FILE(GLOB_RECURSE source_test_files "${CMAKE_CURRENT_LIST_DIR}/source/*" "${CMAKE_CURRENT_LIST_DIR}/source/testing_utils/*")
ADD_EXECUTABLE(modulegateway_tests ${source_test_files} ${CMAKE_CURRENT_LIST_DIR}/mainTests.cpp)
TARGET_INCLUDE_DIRECTORIES(modulegateway_tests PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include/")
TARGET_LINK_LIBRARIES(modulegateway_tests PUBLIC ${GTEST_LIBRARIES} pthread module-gateway-lib)
INCLUDE(GoogleTest)
GTEST_DISCOVER_TESTS(modulegateway_tests)
