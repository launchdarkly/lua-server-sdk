#!/bin/bash

name=$1

docker run -dit --rm --name $name -p 8123:80 \
          --env LAUNCHDARKLY_SDK_KEY="$LAUNCHDARKLY_SDK_KEY" \
          --env LAUNCHDARKLY_FLAG_KEY="$LAUNCHDARKLY_FLAG_KEY" \
          launchdarkly:$name
curl --retry 5 --retry-all-errors --retry-delay 1 -s -v http://localhost:8123
