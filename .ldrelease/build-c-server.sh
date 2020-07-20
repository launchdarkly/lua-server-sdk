#!/bin/bash

set -e

git clone https://github.com/launchdarkly/c-server-sdk.git
cd c-server-sdk
mkdir build
cd build
cmake -D BUILD_SHARED_LIBS=ON -D BUILD_TESTING=OFF -D REDIS_STORE=ON ..
make
make install
