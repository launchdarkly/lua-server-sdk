#!/bin/bash

# Usage: ./scripts/build-cpp-server.sh <tag>

set -e

branch="main"

if [ -n "$1" ]
then
    branch="$1"
fi


git clone --depth 1 --branch "$branch" https://github.com/launchdarkly/cpp-sdks.git
cd cpp-sdks
mkdir build
cd build
cmake -D LD_BUILD_SHARED_LIBS=ON \
      -D BUILD_TESTING=OFF \
      -D LD_BUILD_EXAMPLES=OFF \
      -D LD_BUILD_REDIS_SUPPORT=ON \
      -D CMAKE_INSTALL_PREFIX=./INSTALL ..

cmake --build . --target launchdarkly-cpp-server-redis-source
cmake --install .
