FROM openresty/openresty:jammy

# {{ x-release-please-start-version }}
ARG VERSION=2.1.0
# {{ x-release-please-end }}

ARG CPP_SDK_VERSION=3.3.3

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

# The strategy for this Docker example is to download the C++ SDK release artifacts and use those instead of compiling
# from source. This is for example/CI purposes only; generally it's better to build from source to ensure all libraries
# are compatible.
#
# Since we require a newer version of boost than is available in Ubuntu 22.04, we grab it from a PPA (mhier/libboost-latest).
#
# The SDK dynamic libs expect the boost libs to follow a specific naming convention, which isn't what
# the libraries from the PPA follow ('-mt' suffix is added to indicate the libraries are built with multithreading support enabled.)
#
# It's not 100% clear if these libraries are multithread enabled (build logs in the PPA seem to indicate it),
# but even so, the C++ SDK is single-threaded.
#
# To workaround, add symlinks with the expected names.
RUN cd /usr/lib/x86_64-linux-gnu && \
    ln -s libboost_json.so.1.81.0 libboost_json-mt-x64.so.1.81.0 && \
    ln -s libboost_url.so.1.81.0 libboost_url-mt-x64.so.1.81.0
