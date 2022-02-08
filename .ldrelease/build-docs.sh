#!/bin/bash

set -e

# This only runs in the Linux build, since the docs are the same for all platforms.

# LD_RELEASE_TEMP_DIR is guaranteed to be empty, and will not be checked into version control.
DOCS_BUILD_DIR="$LD_RELEASE_TEMP_DIR/docs"
mkdir "$DOCS_BUILD_DIR"

cp launchdarkly-server-sdk.c "$DOCS_BUILD_DIR"
cp launchdarkly-server-sdk-redis.c "$DOCS_BUILD_DIR"

ldoc -d "$LD_RELEASE_DOCS_DIR" "$DOCS_BUILD_DIR"
