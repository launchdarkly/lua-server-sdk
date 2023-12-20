#!/bin/sh

set -e
luarocks make launchdarkly-server-sdk-1.0-0.rockspec LD_DIR=./cpp-sdks/build/INSTALL
luarocks make launchdarkly-server-sdk-redis-1.0-0.rockspec LDREDIS_DIR=./cpp-sdks/build/INSTALL
