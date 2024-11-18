#!/bin/sh

# Convenience to compile both the normal SDK and Redis luarocks at once, using
# the paths fetched by ./get-cpp-sdk-locally.sh.

set -e

version="2.1.2" # {x-release-please-version }

luarocks make launchdarkly-server-sdk-$version-0.rockspec LD_DIR=./cpp-sdks/build/INSTALL
luarocks make launchdarkly-server-sdk-redis-$version-0.rockspec LDREDIS_DIR=./cpp-sdks/build/INSTALL
