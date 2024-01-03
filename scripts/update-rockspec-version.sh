#!/bin/bash

set -e

# This script has two responsibilities:
# 1. Update the version in the rockspec file.
# 2. Update the actual filename of the rockspec file.
#
# For example, if the current version is 1.0.0, then the rockspec filename is
# launchdarkly-server-sdk-1.0.0-0.rockspec.
#
# If the new version is 1.2.0, then we want the new filename to be
# launchdarkly-server-sdk-1.2.0-0.rockspec.
#
# The '-0' represents the rockspec revision itself (no source changes, just build changes.) This script doesn't
# handle updating that - it should be done manually.

input_rockspec=$1
input_version=$2

if [ -z "$input_rockspec" ] || [ -z "$input_version" ]; then
    echo "Usage: $0 <rockspec package name without version> <target version>"
    exit 1
fi

for file in "$input_rockspec"-*.rockspec; do
    [[ -e "$file" ]] || (echo "No rockspec file found for $input_rockspec" && exit 1)

    # Since the glob might match rockspecs with further suffixes (like -redis), make sure there's actually
    # a version number following the $input_rockspec prefix.
    if [[ ! "$file" =~ "$input_rockspec-"[0-9] ]]; then
        continue
    fi

    # Strip off the prefix + '-' so only the semver, rockspec revision, and file extension remain.
    version_suffix="${file#$input_rockspec-}"




    IFS='-' read -ra parts <<< "$version_suffix"
    semver="${parts[0]}"
    rockspec="${parts[1]}"

    # Split the suffix by '.' and take the first part, which is the rockspec revision.
    IFS='.' read -ra rockspec_parts <<< "$rockspec"

    rockspec_version="${rockspec_parts[0]}"
    echo "Rockspec found: $file"
    echo "  package version: $semver"
    echo "  rockspec version: $rockspec_version"



    new_file_name="$input_rockspec-$input_version-$rockspec_version.rockspec"
    git mv "$file" "$new_file_name"
    echo "Renamed $file to $new_file_name"

    sed -i .bak "s/version = \".*\"/version = \"$input_version-$rockspec_version\"/" "$new_file_name"
    echo "Bumped version from $semver to $input_version"

    rm "$new_file_name.bak"
done
