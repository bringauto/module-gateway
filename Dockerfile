FROM eu.gcr.io/bringauto-infrastructure/teamcity-build-images/ubuntu22.04:latest
WORKDIR /module-gateway
COPY . /module-gateway/tmp

RUN mkdir -p /module-gateway/build && \
    mkdir /module-gateway/cmlib_cache && \
    export CMLIB_REQUIRED_ENV_TMP_PATH=/module-gateway/cmlib_cache && \
    cd /module-gateway/build && \
    cmake /module-gateway/tmp -DCMLIB_DIR=/cmakelib -DCMAKE_BUILD_TYPE=Release -DBRINGAUTO_INSTALL=ON && \
    make -j 8