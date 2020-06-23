#!/bin/bash

set -e

apt-get update -y && apt-get install -y luajit lua-ldoc zip ca-certificates \
    curl zip lua-cjson libpcre3 libcurl4-openssl-dev
