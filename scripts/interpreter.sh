#!/bin/bash

# This invokes a lua script with a lua interpreter specified in an
# environment variable. This is useful for testing in CI, where we want to test with lua and luajit.

if [ -z "$1" ]; then
    echo "Usage: $0 <lua file>"
    exit 1
fi

lua_interpreter=${LUA_INTERPRETER:-lua}

"$lua_interpreter" "$1"
