FROM debian:bookworm

# {{ x-release-please-start-version }}
ARG VERSION=2.1.0
# {{ x-release-please-end }}

ARG CPP_SDK_VERSION=3.5.2

# For unknown reasons, it appears that boost.json and boost.url aren't included in the
# libboost-all package.

RUN apt-get update && apt-get install -y \
    git netbase curl libssl-dev libssl3 apt-transport-https ca-certificates \
    software-properties-common \
    cmake ninja-build build-essential \
    libboost1.81-all-dev libboost-json1.81-dev libboost-url1.81-dev \
    luarocks


RUN mkdir cpp-sdk-libs
RUN git clone --branch launchdarkly-cpp-server-v${CPP_SDK_VERSION} https://github.com/launchdarkly/cpp-sdks.git && \
    cd cpp-sdks && \
    mkdir build-dynamic && \
    cd build-dynamic && \
    cmake -GNinja \
        -DLD_BUILD_EXAMPLES=OFF \
        -DBUILD_TESTING=OFF \
        -DLD_BUILD_SHARED_LIBS=ON \
        -DLD_DYNAMIC_LINK_OPENSSL=ON .. && \
    cmake --build . --target launchdarkly-cpp-server && \
    cmake --install . --prefix=../../cpp-sdk-libs



RUN mkdir hello-debian

# The source .c file and the rockspec are needed to compile the LuaRock.
COPY launchdarkly-server-sdk-${VERSION}-0.rockspec .
COPY launchdarkly-server-sdk.c .

# The example expects to find the env-helper script up one directory, so preserve that structure
# when copying. The purpose of env-helper is to allow the flag key & SDK keys to be injected via
# environment variable (LAUNCHDARKLY_SDK_KEY and LAUNCHDARKLY_FLAG_KEY).
COPY ./examples/hello-debian/hello.lua hello-debian/
COPY ./examples/env-helper env-helper


RUN luarocks make launchdarkly-server-sdk-${VERSION}-0.rockspec LD_DIR=./cpp-sdk-libs

ENV LD_LIBRARY_PATH=../cpp-sdk-libs/lib
ENTRYPOINT ["lua", "hello-debian/hello.lua"]
