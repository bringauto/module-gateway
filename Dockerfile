FROM bringauto/cpp-build-environment:latest AS cmlib_cache_builder

ARG MISSION_MODULE_VERSION=v1.2.8
ARG IO_MODULE_VERSION=v1.2.8

WORKDIR /home/bringauto
ARG CMLIB_REQUIRED_ENV_TMP_PATH=/home/bringauto/cmlib_cache

COPY CMakeLists.txt /home/bringauto/module-gateway/CMakeLists.txt
COPY CMLibStorage.cmake /home/bringauto/module-gateway/CMLibStorage.cmake
COPY cmake/Dependencies.cmake /home/bringauto/module-gateway/cmake/Dependencies.cmake

RUN mkdir /home/bringauto/mission-module && \
    mkdir /home/bringauto/mission-module/cmake && \
    mkdir /home/bringauto/io-module && \
    mkdir /home/bringauto/io-module/cmake

WORKDIR /home/bringauto/mission-module
RUN wget -O CMakeLists.txt https://github.com/bringauto/mission-module/raw/"$MISSION_MODULE_VERSION"/CMakeLists.txt && \
    wget -O CMLibStorage.cmake https://github.com/bringauto/mission-module/raw/"$MISSION_MODULE_VERSION"/CMLibStorage.cmake && \
    wget -O cmake/Dependencies.cmake https://github.com/bringauto/mission-module/raw/"$MISSION_MODULE_VERSION"/cmake/Dependencies.cmake

WORKDIR /home/bringauto/io-module
RUN wget -O CMakeLists.txt https://github.com/bringauto/io-module/raw/"$IO_MODULE_VERSION"/CMakeLists.txt && \
    wget -O CMLibStorage.cmake https://github.com/bringauto/io-module/raw/"$IO_MODULE_VERSION"/CMLibStorage.cmake && \
    wget -O cmake/Dependencies.cmake https://github.com/bringauto/io-module/raw/"$IO_MODULE_VERSION"/cmake/Dependencies.cmake

WORKDIR /home/bringauto/module-gateway/build
RUN cmake .. -DCMAKE_BUILD_TYPE=Release -DBRINGAUTO_GET_PACKAGES_ONLY=ON
WORKDIR /home/bringauto/mission-module/build
RUN cmake .. -DCMAKE_BUILD_TYPE=Release -DBRINGAUTO_GET_PACKAGES_ONLY=ON
WORKDIR /home/bringauto/io-module/build
RUN cmake .. -DCMAKE_BUILD_TYPE=Release -DBRINGAUTO_GET_PACKAGES_ONLY=ON



FROM bringauto/cpp-build-environment:latest AS mission_module_builder

ARG MISSION_MODULE_VERSION=v1.2.8

RUN mkdir /home/bringauto/modules
ARG CMLIB_REQUIRED_ENV_TMP_PATH=/home/bringauto/modules/cmlib_cache

# Build mission module
ADD --chown=bringauto:bringauto https://github.com/bringauto/mission-module.git#$MISSION_MODULE_VERSION mission-module
COPY --from=cmlib_cache_builder /home/bringauto/cmlib_cache /home/bringauto/modules/cmlib_cache
WORKDIR /home/bringauto/mission-module/_build
RUN cmake -DCMAKE_BUILD_TYPE=Release -DBRINGAUTO_INSTALL=ON -DCMAKE_INSTALL_PREFIX=/home/bringauto/modules/mission_module/ -DFLEET_PROTOCOL_BUILD_EXTERNAL_SERVER=OFF .. && \
    make install



FROM bringauto/cpp-build-environment:latest AS io_module_builder

ARG IO_MODULE_VERSION=v1.2.8

RUN mkdir /home/bringauto/modules
ARG CMLIB_REQUIRED_ENV_TMP_PATH=/home/bringauto/modules/cmlib_cache

# Build io module
ADD --chown=bringauto:bringauto https://github.com/bringauto/io-module.git#$IO_MODULE_VERSION io-module
COPY --from=cmlib_cache_builder /home/bringauto/cmlib_cache /home/bringauto/modules/cmlib_cache
WORKDIR /home/bringauto/io-module/_build
RUN cmake -DCMAKE_BUILD_TYPE=Release -DBRINGAUTO_INSTALL=ON -DCMAKE_INSTALL_PREFIX=/home/bringauto/modules/io_module/ -DFLEET_PROTOCOL_BUILD_EXTERNAL_SERVER=OFF .. && \
    make install



FROM bringauto/cpp-build-environment:latest

WORKDIR /home/bringauto/module-gateway
RUN mkdir -p /home/bringauto/module-gateway/cmlib_cache
ARG CMLIB_REQUIRED_ENV_TMP_PATH=/home/bringauto/module-gateway/cmlib_cache

COPY --chown=bringauto:bringauto . /home/bringauto/module-gateway/tmp
COPY resources/config/for_docker.json /home/bringauto/config/for_docker.json
COPY --from=cmlib_cache_builder /home/bringauto/cmlib_cache /home/bringauto/module-gateway/cmlib_cache

# Install module-gateway
RUN mkdir -p /home/bringauto/module-gateway/tmp/build && \
    mkdir /home/bringauto/log && \
    cd /home/bringauto/module-gateway/tmp/build && \
    cmake /home/bringauto/module-gateway/tmp -DCMAKE_BUILD_TYPE=Release -DBRINGAUTO_INSTALL=ON -DCMAKE_INSTALL_PREFIX=/home/bringauto/module-gateway && \
    make -j 8 && \
    make install

# Remove tmp library with source files
RUN rm -rf /home/bringauto/module-gateway/tmp

# Copy module libraries
COPY --from=mission_module_builder /home/bringauto/modules /home/bringauto/modules
COPY --from=io_module_builder /home/bringauto/modules /home/bringauto/modules

EXPOSE 1636
