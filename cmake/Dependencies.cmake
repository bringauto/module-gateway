SET(CMAKE_FIND_USE_CMAKE_SYSTEM_PATH FALSE)

BA_PACKAGE_LIBRARY(protobuf                 v4.21.12)
BA_PACKAGE_LIBRARY(fleet-protocol-interface v2.0.0 NO_DEBUG ON)
BA_PACKAGE_LIBRARY(nlohmann-json            v3.10.5 NO_DEBUG ON)
BA_PACKAGE_LIBRARY(cxxopts                  v3.1.1 NO_DEBUG ON)
BA_PACKAGE_LIBRARY(boost                    v1.86.0)
BA_PACKAGE_LIBRARY(ba-logger                v2.0.0
)
BA_PACKAGE_LIBRARY(pahomqttc                v1.3.9)
BA_PACKAGE_LIBRARY(pahomqttcpp              v1.3.2)
BA_PACKAGE_LIBRARY(zlib                     v1.2.11 OUTPUT_PATH_VAR ZLIB_DIR)

IF (BRINGAUTO_TESTS)
    BA_PACKAGE_LIBRARY(gtest v1.12.1)
ENDIF ()
