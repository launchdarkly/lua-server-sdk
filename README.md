LaunchDarkly Server-Side SDK for Lua
===========================

[![Actions Status](https://github.com/launchdarkly/lua-server-sdk-private/actions/workflows/ci.yml/badge.svg)](https://github.com/launchdarkly/lua-server-sdk/actions/workflows/ci.yml)
[![Documentation](https://img.shields.io/static/v1?label=GitHub+Pages&message=API+reference&color=00add8)](https://launchdarkly.github.io/lua-server-sdk)

LaunchDarkly overview
-------------------------

[LaunchDarkly](https://www.launchdarkly.com) is a feature management platform that serves trillions of feature flags daily to help teams build better software, faster. [Get started](https://docs.launchdarkly.com/home/getting-started) using LaunchDarkly today!

[![Twitter Follow](https://img.shields.io/twitter/follow/launchdarkly.svg?style=social&label=Follow&maxAge=2592000)](https://twitter.com/intent/follow?screen_name=launchdarkly)

Supported Lua versions
-----------

This version of the LaunchDarkly SDK is known to be compatible with the Lua 5.1-5.3 interpreter, and LuaJIT 2.0.5.

Supported C++ server-side SDK versions
-----------

This version of the Lua server-side SDK depends on the LaunchDarkly C++ Server-side SDK.

If Redis support is desired, then it optionally depends on the C++ server-side SDK's Redis Source. 

| Dependency                     | Minimum Version                                                                                            | Notes                                      |
|--------------------------------|------------------------------------------------------------------------------------------------------------|--------------------------------------------|
| C++ Server-Side SDK            | [3.3.0](https://github.com/launchdarkly/cpp-sdks/releases/tag/launchdarkly-cpp-server-v3.3.0)              | Required dependency.                       |
| C++ Server-Side SDK with Redis | [2.1.0](https://github.com/launchdarkly/cpp-sdks/releases/tag/launchdarkly-cpp-server-redis-source-v2.1.0) | Optional, if using Redis as a data source. |


3rd Party Dependencies
------------
Depending on how the C++ server-side SDK was built, the Lua SDK may require additional runtime dependencies to work properly.


| Dependency | If C++ SDK compiled with..   | Notes                                                                  |
|------------|------------------------------|------------------------------------------------------------------------|
| OpenSSL    | `LD_DYNAMIC_LINK_OPENSSL=ON` | If linking OpenSSL dynamically, it must be present on target system.   |
| Boost      | `LD_DYNAMIC_LINK_BOOST=ON`   | If linking Boost dynamically, it must be present on the target system. |

_Note: The CI process builds against the C++ Server-side SDK's Linux shared libraries, which were compiled with `LD_DYNAMIC_LINK_BOOST=ON` so
Boost is fetched as part of the build process._


Getting started
-----------

Refer to the [SDK documentation](https://docs.launchdarkly.com/sdk/server-side/lua#getting-started) for instructions on getting started with using the SDK.

To compile the LuaRock modules:
1. Install [LuaRocks](https://github.com/luarocks/luarocks/wiki/Download)
2. Build the [C++ Server-side SDK](https://github.com/launchdarkly/cpp-sdks) from source using CMake, or obtain pre-built artifacts from the [releases page](https://github.com/launchdarkly/cpp-sdks/releases?q=%22launchdarkly-cpp-server%22)
3. Run `luarocks make` (replace the version number as necessary):
    ```bash
    # Base SDK
    luarocks make launchdarkly-server-sdk-2.0.2-0.rockspec \
    LD_DIR=./path-to-installed-cpp-sdk

    # SDK with Redis
    luarocks make launchdarkly-server-sdk-redis-2.0.2-0.rockspec \
    LDREDIS_DIR=./path-to-installed-cpp-sdk
    ```

Please note that the Lua SDK uses the C++ server-side SDK's C bindings, so if you're using prebuilt artifacts
then only a C99 compiler is necessary.

Learn more
-----------

Read our [documentation](https://docs.launchdarkly.com) for in-depth instructions on configuring and using LaunchDarkly. You can also head straight to the [complete reference guide for this SDK](https://docs.launchdarkly.com/sdk/server-side/lua).

Testing
-------

We run integration tests for all our SDKs using a centralized test harness. This approach gives us the ability to test for consistency across SDKs, as well as test networking behavior in a long-running application. These tests cover each method in the SDK, and verify that event sending, flag evaluation, stream reconnection, and other aspects of the SDK all behave correctly.

Contributing
------------

We encourage pull requests and other contributions from the community. Check out our [contributing guidelines](CONTRIBUTING.md) for instructions on how to contribute to this SDK.

About LaunchDarkly
-----------

* LaunchDarkly is a continuous delivery platform that provides feature flags as a service and allows developers to iterate quickly and safely. We allow you to easily flag your features and manage them from the LaunchDarkly dashboard.  With LaunchDarkly, you can:
    * Roll out a new feature to a subset of your users (like a group of users who opt-in to a beta tester group), gathering feedback and bug reports from real-world use cases.
    * Gradually roll out a feature to an increasing percentage of users, and track the effect that the feature has on key metrics (for instance, how likely is a user to complete a purchase if they have feature A versus feature B?).
    * Turn off a feature that you realize is causing performance problems in production, without needing to re-deploy, or even restart the application with a changed configuration file.
    * Grant access to certain features based on user attributes, like payment plan (eg: users on the ‘gold’ plan get access to more features than users in the ‘silver’ plan). Disable parts of your application to facilitate maintenance, without taking everything offline.
* LaunchDarkly provides feature flag SDKs for a wide variety of languages and technologies. Read [our documentation](https://docs.launchdarkly.com/sdk) for a complete list.
* Explore LaunchDarkly
    * [launchdarkly.com](https://www.launchdarkly.com/ "LaunchDarkly Main Website") for more information
    * [docs.launchdarkly.com](https://docs.launchdarkly.com/  "LaunchDarkly Documentation") for our documentation and SDK reference guides
    * [apidocs.launchdarkly.com](https://apidocs.launchdarkly.com/  "LaunchDarkly API Documentation") for our API documentation
    * [launchdarkly.com/blog](https://launchdarkly.com/blog/  "LaunchDarkly Blog Documentation") for the latest product updates
