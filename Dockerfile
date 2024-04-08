FROM bringauto/cpp-build-environment:latest AS modules_builder

ARG MISSION_MODULE_VERSION=v1.2.2
ARG IO_MODULE_VERSION=v1.2.2

RUN mkdir /home/bringauto/modules
ARG CMLIB_REQUIRED_ENV_TMP_PATH=/home/bringauto/modules/cmlib_cache

RUN git clone https://github.com/bringauto/mission-module.git && \
    mkdir mission-module/_build && \
    cd mission-module/_build && \
    git checkout $MISSION_MODULE_VERSION && \
    cmake -DCMAKE_BUILD_TYPE=Release -DBRINGAUTO_INSTALL=ON -DCMAKE_INSTALL_PREFIX=/home/bringauto/modules/mission_module/ -DFLEET_PROTOCOL_BUILD_EXTERNAL_SERVER=OFF .. && \
    make install

RUN git clone https://github.com/bringauto/io-module.git && \
    mkdir io-module/_build && \
    cd io-module/_build && \
    git checkout $IO_MODULE_VERSION && \
    cmake -DCMAKE_BUILD_TYPE=Release -DBRINGAUTO_INSTALL=ON -DCMAKE_INSTALL_PREFIX=/home/bringauto/modules/io_module/ -DFLEET_PROTOCOL_BUILD_EXTERNAL_SERVER=OFF .. && \
    make install

FROM bringauto/cpp-build-environment:latest
COPY --from=modules_builder /home/bringauto/modules /home/bringauto/modules

WORKDIR /home/bringauto/module-gateway
COPY --chown=bringauto:bringauto . /home/bringauto/module-gateway/tmp
COPY resources/config/for_docker.json /home/bringauto/config/for_docker.json

# Install module-gateway
RUN mkdir -p /home/bringauto/module-gateway/tmp/build && \
    mkdir /home/bringauto/module-gateway/tmp/cmlib_cache && \
    mkdir /home/bringauto/log && \
    export CMLIB_REQUIRED_ENV_TMP_PATH=/home/bringauto/module-gateway/tmp/cmlib_cache && \
    cd /home/bringauto/module-gateway/tmp/build && \
    cmake /home/bringauto/module-gateway/tmp -DCMAKE_BUILD_TYPE=Release -DBRINGAUTO_INSTALL=ON -DCMAKE_INSTALL_PREFIX=/home/bringauto/module-gateway && \
    make -j 8 && \
    make install

# Remove tmp library with source files
RUN rm -rf /home/bringauto/module-gateway/tmp
