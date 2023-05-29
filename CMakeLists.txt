CMAKE_MINIMUM_REQUIRED(VERSION 3.25 FATAL_ERROR)
PROJECT(ModuleGateway)

SET(CMAKE_CXX_STANDARD 20)



OPTION(BRINGAUTO_TESTS "Disable tests" OFF)
OPTION(BRINGAUTO_PACKAGE "Package creation" OFF)
OPTION(BRINGAUTO_INSTALL "Disable install" OFF)
OPTION(BRINGAUTO_SYSTEM_DEP "Enable system dependencies" OFF)
OPTION(BRINGAUTO_SAMPLES "Enable build of sample app, not used in project" OFF)

IF (BRINGAUTO_PACKAGE)
    IF (NOT BRINGAUTO_INSTALL)
        SET(BRINGAUTO_INSTALL ON CACHE BOOL "Forced install due to BRINGAUTO_PACKAGE=ON" FORCE)
        MESSAGE(WARNING "BRINGAUTO_INSTALL is switched to on because of BRINGAUTO_PACKAGE=ON")
    ENDIF ()
ENDIF ()


IF (BRINGAUTO_TESTS)
    ENABLE_TESTING()
    INCLUDE(${CMAKE_CURRENT_SOURCE_DIR}/test/CMakeLists.txt)
ENDIF (BRINGAUTO_TESTS)

FIND_PACKAGE(CMLIB
        COMPONENTS CMDEF CMUTIL STORAGE
        REQUIRED
        )

IF (NOT BRINGAUTO_SYSTEM_DEP)
    INCLUDE(cmake/Dependencies.cmake)
ENDIF ()

SET(Protobuf_USE_STATIC_LIBS ON)

FIND_PACKAGE(Boost 1.74 REQUIRED)
FIND_PACKAGE(Protobuf 3.21.12 REQUIRED)
FIND_PACKAGE(cxxopts 3.0.0 REQUIRED)
FIND_PACKAGE(nlohmann_json 3.2.0 REQUIRED)
FIND_PACKAGE(libbringauto_logger 1.2.0 REQUIRED)

ADD_SUBDIRECTORY(${CMAKE_CURRENT_LIST_DIR}/libs/fleet-protocol)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_LIST_DIR}/libs/button_module)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_LIST_DIR}/libs/mission_module)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_LIST_DIR}/libs/memory_management)

FILE(GLOB_RECURSE source_files "src/*")
ADD_LIBRARY(bringautolib STATIC "${source_files}")
TARGET_INCLUDE_DIRECTORIES(bringautolib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
TARGET_LINK_LIBRARIES(bringautolib PUBLIC memory_management fleet-protocol::protobuf_cpp fleet-protocol::common_headers fleet-protocol::module_gateway module_gateway_maintainer nlohmann_json::nlohmann_json cxxopts::cxxopts bringauto_logger::bringauto_logger ${CMAKE_DL_LIBS})

ADD_EXECUTABLE(${PROJECT_NAME} main.cpp)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC bringautolib)

IF (BRINGAUTO_INSTALL)
    CMDEF_INSTALL(TARGET virtual-vehicle-utility)
    BA_PACKAGE_DEPS_IMPORTED(virtual-vehicle-utility)
ENDIF ()


IF (BRINGAUTO_PACKAGE)
    CMDEF_PACKAGE(
            MAIN_TARGET virtual-vehicle-utility
    )
    SET(CPACK_GENERATOR ZIP)
    SET(CPACK_PACKAGE_CONTACT "BringAuto s.r.o. <maintainers@bringauto.com>")
    SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "BringAuto s.r.o <maintainers@bringauto.com>")
    SET(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS ON)
    SET(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
    INCLUDE(CPack)
ENDIF ()
