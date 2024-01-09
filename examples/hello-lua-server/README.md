# LaunchDarkly sample Lua server-side application
We've built a simple console application that demonstrates how LaunchDarkly's SDK works. 
Below, you'll find the build procedure. 

For more comprehensive instructions, you can visit the [Lua reference guide](https://docs.launchdarkly.com/sdk/server-side/lua).

## Dependencies
You will need the shared library(s) for the LaunchDarkly [C++ Server-side SDK](https://github.com/launchdarkly/cpp-sdks). 

You can obtain prebuilt artifacts [here](https://github.com/launchdarkly/cpp-sdks/releases?q=%22launchdarkly-cpp-server%22), or build your own from source.

To build from source, you may use the helper script as a starting point:

```bash
# Obtains the C++ Server-side SDK on the main branch
./scripts/get-cpp-sdk-locally.sh

# Obtain a particular release (update to your desired release as needed)
./scripts/get-cpp-sdk-locally.sh launchdarkly-cpp-server-redis-source-v2.1.0
```
**Note (1):** The helper script assumes `git`, `cmake`, `ninja`, a C++17 compiler, OpenSSL, and `boost > 1.81` are present.

**Note (2):** The helper script builds the C++ Server-side SDK with Redis support for maximum flexibility during development. 
You can build only the base SDK if Redis isn't necessary. 

## Instructions
1. Install the dependencies described above.
2. Edit hello.lua and set the value of `YOUR_SDK_KEY` to your LaunchDarkly SDK key. Alternatively, set the environment variable
`LD_SDK_KEY` before invoking the Lua interpreter.
3. If there is an existing boolean feature flag in your LaunchDarkly project that you want to evaluate, set `YOUR_FEATURE_KEY` to the flag key.

```
local YOUR_SDK_KEY = "sdk-key-123abc"

local YOUR_FEATURE_KEY = "my-boolean-flag"
```

4. On the command line, build the SDK with `./scripts/compile.sh` (this assumes you've obtained the C++ SDK using `./scripts/get-cpp-sdk-locally.sh`).
5. On the command line, run `lua hello.lua`

If the test is successful, you should see:

```bash
$ lua hello.lua
Feature flag 'my-boolean-flag' is [true/false] for this user
```
