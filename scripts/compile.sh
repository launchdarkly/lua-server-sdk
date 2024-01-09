#!/bin/sh

# Convenience to compile both the normal SDK and Redis luarocks at once, using
# the paths fetched by ./get-cpp-sdk-locally.sh.

set -e
luarocks make launchdarkly-server-sdk-2.0.1-0.rockspec LD_DIR=./cpp-sdks/build/INSTALL
luarocks make launchdarkly-server-sdk-redis-2.0.1-0.rockspec LDREDIS_DIR=./cpp-sdks/build/INSTALL
