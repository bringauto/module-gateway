CMAKE_MINIMUM_REQUIRED(VERSION 3.25 FATAL_ERROR)
PROJECT(ModuleGateway)

FIND_PACKAGE(CMLIB
        COMPONENTS CMDEF CMUTIL STORAGE
        REQUIRED
)

SET(BRINGAUTO_MODULE_GATEWAY_VERSION 1.3.5)

SET(MODULE_GATEWAY_MINIMUM_LOGGER_VERBOSITY "DEBUG" CACHE STRING "Minimum logger verbosity level for module-gateway")

CMDEF_COMPILE_DEFINITIONS(
        ALL
        "MODULE_GATEWAY_VERSION=\"${BRINGAUTO_MODULE_GATEWAY_VERSION}\""
        "MODULE_GATEWAY_MINIMUM_LOGGER_VERBOSITY=\"${MODULE_GATEWAY_MINIMUM_LOGGER_VERBOSITY}\""
)
SET(CMAKE_INSTALL_RPATH "$ORIGIN/../${CMDEF_LIBRARY_INSTALL_DIR}")
SET(CMAKE_CXX_STANDARD 20)

INCLUDE(CheckPIESupported)
CHECK_PIE_SUPPORTED()
SET(CMAKE_POSITION_INDEPENDENT_CODE ON)

OPTION(BRINGAUTO_TESTS             "Disable tests" OFF)
OPTION(BRINGAUTO_PACKAGE           "Package creation" OFF)
OPTION(BRINGAUTO_INSTALL           "Disable install" OFF)
OPTION(BRINGAUTO_SYSTEM_DEP        "Enable system dependencies" OFF)
OPTION(BRINGAUTO_SAMPLES           "Enable build of sample app, not used in project" OFF)
OPTION(BRINGAUTO_GET_PACKAGES_ONLY "Only download packages for this project" OFF)

IF(BRINGAUTO_PACKAGE)
    IF(NOT BRINGAUTO_INSTALL)
        SET(BRINGAUTO_INSTALL ON CACHE BOOL "Forced install due to BRINGAUTO_PACKAGE=ON" FORCE)
        MESSAGE(WARNING "BRINGAUTO_INSTALL is switched to on because of BRINGAUTO_PACKAGE=ON")
    ENDIF()
ENDIF()

IF(NOT BRINGAUTO_SYSTEM_DEP)
    INCLUDE(cmake/Dependencies.cmake)
    IF(BRINGAUTO_GET_PACKAGES_ONLY)
        RETURN()
    ENDIF()
ENDIF()

SET(Protobuf_USE_STATIC_LIBS ON)

FIND_PACKAGE(Boost 1.74 REQUIRED CONFIG)
FIND_PACKAGE(Protobuf 3.21.12 REQUIRED)
FIND_PACKAGE(cxxopts 3.1.1 REQUIRED)
FIND_PACKAGE(nlohmann_json 3.2.0 REQUIRED)
FIND_PACKAGE(PahoMqttCpp REQUIRED)
FIND_PACKAGE(eclipse-paho-mqtt-c REQUIRED)
FIND_PACKAGE(libbringauto_logger 2.0.0 REQUIRED)
FIND_PACKAGE(fleet-protocol-interface 2.0.0 REQUIRED)
FIND_PACKAGE(ZLIB 1.2.11 REQUIRED)
FIND_PACKAGE(fleet-protocol-cxx-helpers-static 1.1.1 REQUIRED)
FIND_PACKAGE(aeron 1.48.6 REQUIRED)
FIND_PACKAGE(async-function-execution-shared 0.1.0 REQUIRED)

FILE(GLOB_RECURSE source_files "source/*")
ADD_LIBRARY(module-gateway-lib STATIC "${source_files}")
TARGET_INCLUDE_DIRECTORIES(module-gateway-lib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
ADD_SUBDIRECTORY(source/bringauto/modules/memory_management)
TARGET_LINK_LIBRARIES(module-gateway-lib PUBLIC
                        Boost::headers
                        protobuf::libprotobuf
                        fleet-protocol-interface::protobuf-cpp-interface
                        fleet-protocol-interface::common-headers-interface
                        fleet-protocol-interface::module-gateway-interface
                        fleet-protocol-interface::module-maintainer-module-gateway-interface
                        nlohmann_json::nlohmann_json
                        cxxopts::cxxopts
                        bringauto_logger::bringauto_logger
                        eclipse-paho-mqtt-c::paho-mqtt3as
                        PahoMqttCpp::paho-mqttpp3
                        ZLIB::ZLIB
                        fleet-protocol-cxx-helpers-static::fleet-protocol-cxx-helpers-static
                        async-function-execution-shared::async-function-execution-shared
                        memory_management
                        ${CMAKE_DL_LIBS}
)

CMDEF_ADD_EXECUTABLE(
        TARGET module-gateway-app
        SOURCES main.cpp
        VERSION ${BRINGAUTO_MODULE_GATEWAY_VERSION}
)

IF(CMAKE_BUILD_TYPE STREQUAL "Debug" OR BRINGAUTO_TESTS)
    TARGET_COMPILE_OPTIONS(module-gateway-app PRIVATE -Wall -Wextra -Wpedantic
            -fsanitize=address,undefined,leak -fno-sanitize-recover )
    TARGET_LINK_OPTIONS(module-gateway-app PUBLIC -fsanitize=undefined,leak,address)
    TARGET_COMPILE_OPTIONS(module-gateway-lib PUBLIC -Wall -Wextra -Wpedantic
            -fsanitize=address,undefined,leak -fno-sanitize-recover)
    TARGET_LINK_OPTIONS(module-gateway-lib PUBLIC -Wall -Wextra -Wpedantic
            -fsanitize=address,undefined,leak -fno-sanitize-recover)
ENDIF()

TARGET_LINK_LIBRARIES(module-gateway-app PUBLIC module-gateway-lib)

IF(BRINGAUTO_TESTS)
    ENABLE_TESTING()
    #INCLUDE(${CMAKE_CURRENT_SOURCE_DIR}/test/CMakeLists.txt)
    INCLUDE(CTest)
ENDIF(BRINGAUTO_TESTS)


IF(BRINGAUTO_INSTALL)
    INCLUDE(GNUInstallDirs)
    CONFIGURE_FILE(resources/systemd/module-gateway.service.in module-gateway.service @ONLY)
    CMDEF_INSTALL(
            TARGET module-gateway-app
            NAMESPACE module-gateway-app::
    )
    BA_PACKAGE_DEPS_IMPORTED(module-gateway-app)
    INSTALL(FILES "${CMAKE_CURRENT_LIST_DIR}/resources/config/default.json"
            DESTINATION
            "${CMAKE_INSTALL_SYSCONFDIR}/bringauto/module-gateway/"
            RENAME "config.json")
    INSTALL(FILES "${CMAKE_CURRENT_BINARY_DIR}/module-gateway.service"
            DESTINATION
            "${CMAKE_INSTALL_SYSCONFDIR}/systemd/system/")
ENDIF()


IF (BRINGAUTO_PACKAGE)
    CMDEF_PACKAGE(
            MAIN_TARGET module-gateway-app
            VERSION ${BRINGAUTO_MODULE_GATEWAY_VERSION}
    )
    SET(CPACK_GENERATOR ZIP)
    INCLUDE(CPack)
ENDIF ()
