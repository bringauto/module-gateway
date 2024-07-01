FROM bringauto/cpp-build-environment:latest AS mission_module_builder

ARG MISSION_MODULE_VERSION=v1.2.7

RUN mkdir /home/bringauto/modules
ARG CMLIB_REQUIRED_ENV_TMP_PATH=/home/bringauto/modules/cmlib_cache

# Build mission module
ADD --chown=bringauto:bringauto https://github.com/bringauto/mission-module.git#$MISSION_MODULE_VERSION mission-module
RUN mkdir mission-module/_build && \
    cd mission-module/_build && \
    cmake -DCMAKE_BUILD_TYPE=Release -DBRINGAUTO_INSTALL=ON -DCMAKE_INSTALL_PREFIX=/home/bringauto/modules/mission_module/ -DFLEET_PROTOCOL_BUILD_EXTERNAL_SERVER=OFF .. && \
    make install
    
FROM bringauto/cpp-build-environment:latest AS io_module_builder

ARG IO_MODULE_VERSION=v1.2.7

RUN mkdir /home/bringauto/modules
ARG CMLIB_REQUIRED_ENV_TMP_PATH=/home/bringauto/modules/cmlib_cache

# Build io module
ADD --chown=bringauto:bringauto https://github.com/bringauto/io-module.git#$IO_MODULE_VERSION io-module
RUN mkdir io-module/_build && \
    cd io-module/_build && \
    cmake -DCMAKE_BUILD_TYPE=Release -DBRINGAUTO_INSTALL=ON -DCMAKE_INSTALL_PREFIX=/home/bringauto/modules/io_module/ -DFLEET_PROTOCOL_BUILD_EXTERNAL_SERVER=OFF .. && \
    make install

FROM bringauto/cpp-build-environment:latest

WORKDIR /home/bringauto/module-gateway
COPY --chown=bringauto:bringauto . /home/bringauto/module-gateway/tmp
COPY resources/config/for_docker.json /home/bringauto/config/for_docker.json

# Install module-gateway
RUN mkdir -p /home/bringauto/module-gateway/tmp/build && \
    mkdir -p /home/bringauto/module-gateway/tmp/cmlib_cache && \
    mkdir /home/bringauto/log && \
    export CMLIB_REQUIRED_ENV_TMP_PATH=/home/bringauto/module-gateway/tmp/cmlib_cache && \
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
