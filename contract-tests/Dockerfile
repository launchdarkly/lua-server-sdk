# Builds the contract test service: the Lua SDK from this checkout, served by OpenResty.
FROM openresty/openresty:1.31.8.1-0-jammy

ARG CPP_SDK_VERSION=3.15.0

RUN apt-get update && apt-get install -y \
    git netbase curl libssl-dev apt-transport-https ca-certificates \
    software-properties-common \
    cmake ninja-build locales-all

# OpenResty bundles its own OpenSSL, and the SDK uses that OpenSSL at runtime.
# The bundled OpenSSL looks for CA certificates in a directory that the image does not have.
# This variable points it at the system CA bundle. nginx.conf passes it to the worker process.
ENV SSL_CERT_FILE=/etc/ssl/certs/ca-certificates.crt

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

COPY . .
COPY ./contract-tests/nginx.conf /usr/local/openresty/nginx/conf/nginx.conf
COPY ./contract-tests/service.lua /usr/local/openresty/nginx/contract-tests/

RUN luarocks make launchdarkly-server-sdk-[0-9]*-0.rockspec LD_DIR=./cpp-sdk-libs && \
    cp launchdarkly_server_sdk.so /usr/local/openresty/lualib/
