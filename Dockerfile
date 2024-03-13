FROM europe-west1-docker.pkg.dev/bringauto-infrastructure/teamcity-build-images/ubuntu22.04:latest
WORKDIR /home/bringauto/module-gateway
COPY --chown=bringauto:bringauto . /home/bringauto/module-gateway/tmp
COPY resources/config/for_docker.json /home/bringauto/config/for_docker.json

# Install Module gateway
RUN mkdir -p /home/bringauto/module-gateway/tmp/build && \
    mkdir /home/bringauto/module-gateway/tmp/cmlib_cache && \
    mkdir /home/bringauto/log && \
    export CMLIB_REQUIRED_ENV_TMP_PATH=/home/bringauto/module-gateway/tmp/cmlib_cache && \
    cd /home/bringauto/module-gateway/tmp/build && \
    cmake /home/bringauto/module-gateway/tmp -DCMLIB_DIR=/cmakelib -DCMAKE_BUILD_TYPE=Release -DBRINGAUTO_INSTALL=ON -DCMAKE_INSTALL_PREFIX=/home/bringauto/module-gateway && \
    make -j 8 && \
    make install

# Remove tmp library with source files
RUN rm -rf /home/bringauto/module-gateway/tmp
