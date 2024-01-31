FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    curl luarocks lua5.3 lua5.3-dev \
    haproxy apt-transport-https ca-certificates \
    software-properties-common

RUN add-apt-repository ppa:mhier/libboost-latest && \
     apt-get update && \
     apt-get install -y boost1.81


RUN curl https://github.com/launchdarkly/cpp-sdks/releases/download/launchdarkly-cpp-server-v3.3.1/linux-gcc-x64-dynamic.zip -L -o /tmp/sdk.zip && \
    mkdir ./cpp-sdk && \
    unzip /tmp/sdk.zip -d ./cpp-sdk && \
    rm /tmp/sdk.zip

COPY . .

RUN luarocks make launchdarkly-server-sdk-2.0.2-0.rockspec LD_DIR=./cpp-sdk/build-dynamic/release

COPY ./examples/hello-haproxy/haproxy.cfg /etc/haproxy/haproxy.cfg
COPY ./examples/hello-haproxy/service.lua /service.lua

RUN cd /usr/lib/x86_64-linux-gnu && \
    ln -s libboost_json.so.1.81.0 libboost_json-mt-x64.so.1.81.0 && \
    ln -s libboost_url.so.1.81.0 libboost_url-mt-x64.so.1.81.0

CMD  haproxy -d -f /etc/haproxy/haproxy.cfg
