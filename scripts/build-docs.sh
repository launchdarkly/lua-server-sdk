#!/bin/bash

set -e

# The first argument is the directory where the docs should be built.

if [ -z "$1" ]; then
    echo "Usage: $0 <docs-build-dir>"
    exit 1
fi

DOCS_BUILD_DIR=$1
mkdir "$DOCS_BUILD_DIR"

cp launchdarkly-server-sdk.c "$DOCS_BUILD_DIR"
cp launchdarkly-server-sdk-redis.c "$DOCS_BUILD_DIR"

ldoc -d "$LD_RELEASE_DOCS_DIR" "$DOCS_BUILD_DIR"
