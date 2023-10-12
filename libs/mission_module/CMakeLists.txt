CMAKE_MINIMUM_REQUIRED(VERSION 3.25 FATAL_ERROR)
PROJECT(ModuleGateway)

SET(CMAKE_CXX_STANDARD 20)


FILE(GLOB_RECURSE mission_module_cpp_files "./src/*")
ADD_LIBRARY(mission_module SHARED ${mission_module_cpp_files})
TARGET_LINK_LIBRARIES(mission_module PUBLIC fleet-protocol::common_headers module_gateway_maintainer memory_management)