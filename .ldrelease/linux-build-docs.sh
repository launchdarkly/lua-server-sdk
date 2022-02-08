#!/bin/bash

set -e

# This only runs in the Linux build, since the docs are the same for all platforms.

PROJECT_DIR=$(pwd)

mkdir tmp
cp launchdarkly-server-sdk.c tmp/
cp launchdarkly-server-sdk-redis.c tmp/

ldoc tmp

cp -r ${PROJECT_DIR}/docs/* ${LD_RELEASE_DOCS_DIR}
