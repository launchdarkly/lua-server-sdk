#!/bin/bash

set -e

sed -i "s/local SDKVersion =.*/local SDKVersion = \"${LD_RELEASE_VERSION}\"/" 'launchdarkly-server-sdk.lua'
