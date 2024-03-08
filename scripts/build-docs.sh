#!/bin/bash

set -e

# The first argument is the directory where the docs should be built.
# The second is where the docs should be copied to when built.

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <docs build dir> <docs release dir>"
    exit 1
fi

DOCS_BUILD_DIR=$1
DOCS_RELEASE_DIR=$2

mkdir "$DOCS_BUILD_DIR"

cp launchdarkly-server-sdk.c "$DOCS_BUILD_DIR"
cp launchdarkly-server-sdk-redis.c "$DOCS_BUILD_DIR"

ldoc -d "$DOCS_RELEASE_DIR" "$DOCS_BUILD_DIR" -p "LaunchDarkly Lua Server SDK"
