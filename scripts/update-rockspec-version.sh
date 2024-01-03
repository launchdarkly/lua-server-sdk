#!/bin/bash

set -e

# This script has two responsibilities:
# 1. Update the package version in the rockspec file.
# 2. Update the actual filename of the rockspec file.
# These two should match.
# This script doesn't update the rockspec version - the part after the package version, but before the .rockspec
# suffix. That should be done manually if the rockspec itself has changed but not the package contents.
#
# For example, if the current version is 1.0.0, then the rockspec filename is
# launchdarkly-server-sdk-1.0.0-0.rockspec.
#
# If the new version is 1.2.0, then we want the new filename to be
# launchdarkly-server-sdk-1.2.0-0.rockspec.


input_rockspec=$1
input_version=$2
git_username=$3
git_email=$4

autocommit=''
if [ -n "$git_username" ] || [ -n "$git_email" ]; then
    autocommit=1
fi

# Ensure autocommit if provided is
if [ -z "$input_rockspec" ] || [ -z "$input_version" ]; then
    echo "Usage: $0 <rockspec package> <new version> [git username] [git email]"
    echo "Example usage locally: $0 launchdarkly-server-sdk 1.2.0"
    echo "Example usage in CI: $0 launchdarkly-server-sdk 1.2.0 LaunchDarklyReleaseBot LaunchDarklyReleaseBot@launchdarkly.com"
    echo "Providing a git username and email will automatically commit & push any changes."
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

    # Split the suffix by '.' so we can get the rockspec version number.
    IFS='.' read -ra rockspec_parts <<< "$rockspec"

    rockspec_revision="${rockspec_parts[0]}"
    echo "Rockspec found: $file"
    echo "  package version: $semver"
    echo "  rockspec revision: $rockspec_revision"


    new_file_name="$input_rockspec-$input_version-$rockspec_revision.rockspec"
    git mv "$file" "$new_file_name"
    echo "Renamed $file to $new_file_name"

    sed -i .bak "s/version = \".*\"/version = \"$input_version-$rockspec_revision\"/" "$new_file_name"
    echo "Bumped version from $semver to $input_version"

    rm -f "$new_file_name.bak"

    if [ "$(git status --porcelain | wc -l)" -gt 0 ]; then
      if [ -n "$git_username" ]; then
        git config user.name "$git_username"
      fi
      if [ -n "$git_email" ]; then
        git config user.email "$git_email"
      fi
      git add "$new_file_name"
      if [ $autocommit ]; then
        git commit -m "chore: bump $input_rockspec version from $semver to $input_version"
        git push
      else
        echo "Changes staged, but not committed. Please commit manually."
      fi
      exit 0
    fi
done
