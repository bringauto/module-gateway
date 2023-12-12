FROM eu.gcr.io/bringauto-infrastructure/teamcity-build-images/ubuntu22.04:latest
WORKDIR /home/bringauto/module-gateway
COPY --chown=bringauto:bringauto . /home/bringauto/module-gateway/tmp
COPY configs/for_docker.json /home/bringauto/config/for_docker.json

# Install Module gateway
RUN mkdir -p /home/bringauto/module-gateway/build && \
    mkdir /home/bringauto/module-gateway/cmlib_cache && \
    mkdir /home/bringauto/log && \
    mkdir /home/bringauto/modules && \
    export CMLIB_REQUIRED_ENV_TMP_PATH=/home/bringauto/module-gateway/cmlib_cache && \
    cd /home/bringauto/module-gateway/build && \
    cmake /home/bringauto/module-gateway/tmp -DCMLIB_DIR=/cmakelib -DCMAKE_BUILD_TYPE=Release -DBRINGAUTO_INSTALL=ON -DCMAKE_INSTALL_PREFIX=/home/bringauto/module-gateway && \
    make -j 8 && \
    make install

# Install IO module
WORKDIR /home/bringauto/module-gateway/tmp//libs/io-module
RUN mkdir _build && cd _build && \
    cmake -DCMLIB_DIR=/cmakelib .. && \
    make -j 8 && \
    mv ./libio_module_module_manager.so /home/bringauto/modules/libio_module_module_manager.so

# Remove tmp library with source files
RUN rm -rf /home/bringauto/module-gateway/tmp