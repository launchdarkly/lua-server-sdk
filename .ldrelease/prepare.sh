#!/bin/bash

set -e

apt-get update -y && apt-get install -y luajit lua-ldoc zip ca-certificates \
    curl libpcre3 libcurl4-openssl-dev cmake libhiredis-dev git \
    build-essential libpcre3-dev
