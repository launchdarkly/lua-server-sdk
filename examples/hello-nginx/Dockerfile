# We are pinning to this version of OpenResty as it is compatible with the C++ SDK's usage of OpenSSL.
# It is unclear why, but the latest version of OpenResty doesn't seem to be compatible: the SDK
# emits errors of the form 'failed to do XYZ (STORE routines) [asio.ssl:369098857]'
FROM openresty/openresty:1.21.4.1-0-jammy

# {{ x-release-please-start-version }}
ARG VERSION=2.1.2
# {{ x-release-please-end }}

ARG CPP_SDK_VERSION=3.8.0

RUN apt-get update && apt-get install -y \
    git netbase curl libssl-dev apt-transport-https ca-certificates \
    software-properties-common \
    cmake ninja-build locales-all

RUN add-apt-repository ppa:mhier/libboost-latest && \
     apt-get update && \
     apt-get install -y boost1.81


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

RUN mkdir -p /usr/local/openresty/nginx/scripts

COPY . .
COPY ./examples/hello-nginx/nginx.conf /usr/local/openresty/nginx/conf/nginx.conf
COPY ./examples/hello-nginx/shared.lua /usr/local/openresty/nginx/scripts/
COPY ./examples/env-helper/get_from_env_or_default.lua /usr/local/openresty/nginx/scripts/


RUN luarocks make launchdarkly-server-sdk-"${VERSION}"-0.rockspec LD_DIR=./cpp-sdk-libs && \
    cp launchdarkly_server_sdk.so /usr/local/openresty/lualib/
