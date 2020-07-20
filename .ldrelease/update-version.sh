#!/bin/bash

set -e

sed -i "s/#define SDKVersion .*/#define SDKVersion \"${LD_RELEASE_VERSION}\"/" 'launchdarkly-server-sdk.c'
