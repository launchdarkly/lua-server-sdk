FROM ubuntu:22.04

# {{ x-release-please-start-version }}
ARG VERSION=2.1.3
# {{ x-release-please-end }}

ARG CPP_SDK_VERSION=3.9.0

RUN apt-get update && apt-get install -y \
    curl luarocks lua5.3 lua5.3-dev \
    haproxy apt-transport-https ca-certificates \
    software-properties-common

RUN add-apt-repository ppa:mhier/libboost-latest && \
     apt-get update && \
     apt-get install -y boost1.81

RUN curl https://github.com/launchdarkly/cpp-sdks/releases/download/launchdarkly-cpp-server-v${CPP_SDK_VERSION}/linux-gcc-x64-dynamic.zip -L -o /tmp/sdk.zip && \
    mkdir ./cpp-sdk && \
    unzip /tmp/sdk.zip -d ./cpp-sdk && \
    rm /tmp/sdk.zip

COPY . .
COPY ./examples/hello-haproxy/haproxy.cfg /etc/haproxy/haproxy.cfg
COPY ./examples/hello-haproxy/service.lua /service.lua
COPY ./examples/env-helper/get_from_env_or_default.lua /get_from_env_or_default.lua

RUN luarocks make launchdarkly-server-sdk-"${VERSION}"-0.rockspec LD_DIR=./cpp-sdk/build-dynamic/release



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

CMD  haproxy -d -f /etc/haproxy/haproxy.cfg
