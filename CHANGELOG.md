# Change log

All notable changes to the LaunchDarkly Lua Server-side SDK will be documented in this file. This project adheres to [Semantic Versioning](http://semver.org).

## [2.1.3](https://github.com/launchdarkly/lua-server-sdk/compare/v2.1.2...v2.1.3) (2025-06-06)


### Bug Fixes

* Bump launchdarkly-cpp-server to v3.9.0 ([1d8888d](https://github.com/launchdarkly/lua-server-sdk/commit/1d8888d160140e096838ca415c4de3df8fae9cb8))
* Bump launchdarkly-cpp-server-redis-source to v2.1.19 ([1d8888d](https://github.com/launchdarkly/lua-server-sdk/commit/1d8888d160140e096838ca415c4de3df8fae9cb8))

## [2.1.2](https://github.com/launchdarkly/lua-server-sdk/compare/v2.1.1...v2.1.2) (2024-11-18)


### Bug Fixes

* **deps:** bump C++ server dep for holdouts fix ([#109](https://github.com/launchdarkly/lua-server-sdk/issues/109)) ([53a7ef1](https://github.com/launchdarkly/lua-server-sdk/commit/53a7ef19fea3fcea6209f18fdde42d699b464cdc))

## [2.1.1](https://github.com/launchdarkly/lua-server-sdk/compare/v2.1.0...v2.1.1) (2024-07-15)


### Bug Fixes

* **deps:** bump C++ Server to 3.5.2 ([#105](https://github.com/launchdarkly/lua-server-sdk/issues/105)) ([01fac9e](https://github.com/launchdarkly/lua-server-sdk/commit/01fac9e2f80948efbe10885ae053dae3f23d67e6))

## [2.1.0](https://github.com/launchdarkly/lua-server-sdk/compare/v2.0.6...v2.1.0) (2024-03-11)


### Features

* allow nil to be passed for clientInit config ([#94](https://github.com/launchdarkly/lua-server-sdk/issues/94)) ([e7c0dfe](https://github.com/launchdarkly/lua-server-sdk/commit/e7c0dfed95a0b10ca67e9ccfe8cab63db9fe2d53))

## [2.0.6](https://github.com/launchdarkly/lua-server-sdk/compare/v2.0.5...v2.0.6) (2024-02-07)


### Bug Fixes

* rockspec info JSON was generated incorrectly ([#88](https://github.com/launchdarkly/lua-server-sdk/issues/88)) ([8488c38](https://github.com/launchdarkly/lua-server-sdk/commit/8488c38e3680e4aad163c95928869c539d3bc854))

## [2.0.5](https://github.com/launchdarkly/lua-server-sdk/compare/v2.0.4...v2.0.5) (2024-02-07)


### Bug Fixes

* publish step should take rockspec inputs ([#85](https://github.com/launchdarkly/lua-server-sdk/issues/85)) ([ea11770](https://github.com/launchdarkly/lua-server-sdk/commit/ea1177043953d2c38c95b3b46f659aedf2b69ff3))

## [2.0.4](https://github.com/launchdarkly/lua-server-sdk/compare/v2.0.3...v2.0.4) (2024-02-07)


### Bug Fixes

* hello-haproxy Dockerfile is now updated by release-please ([d0522a5](https://github.com/launchdarkly/lua-server-sdk/commit/d0522a532b91f938caf1808f724077e7c199b6a3))

## [2.0.3](https://github.com/launchdarkly/lua-server-sdk/compare/v2.0.2...v2.0.3) (2024-02-07)


### Bug Fixes

* add hello-haproxy ([#69](https://github.com/launchdarkly/lua-server-sdk/issues/69)) ([72481e3](https://github.com/launchdarkly/lua-server-sdk/commit/72481e34d7bf0f52d4372f8d13d5e6fa8a034a30))

## [2.0.2](https://github.com/launchdarkly/lua-server-sdk/compare/v2.0.1...v2.0.2) (2024-01-09)


### Bug Fixes

* update version used in in compile.sh to match release version ([92be742](https://github.com/launchdarkly/lua-server-sdk/commit/92be74287031a4f53a7ea3f7e235e088f311423c))

## [2.0.1](https://github.com/launchdarkly/lua-server-sdk/compare/v2.0.0...v2.0.1) (2024-01-09)


### Bug Fixes

* add hello-lua-server example ([#41](https://github.com/launchdarkly/lua-server-sdk/issues/41)) ([462a3f3](https://github.com/launchdarkly/lua-server-sdk/commit/462a3f32f4e9d785fb1656cf6274513ca246740c))

## [2.0.0](https://github.com/launchdarkly/lua-server-sdk/compare/1.2.2...v2.0.0) (2023-12-28)


### ⚠ BREAKING CHANGES

* SDK configuration exposes new options and is organized hierarchically
* remove `inlineUsersInEvents` and `userKeysFlushInterval` config options
* remove global `registerLogger` function, replace with config option
* remove `alias` function, replace usage with multi-kind contexts
* Variation and VariationDetail now take contexts
* makeUser behavior modified to construct a user-kind context
* use C++ Server-side SDK 3.0 bindings ([#31](https://github.com/launchdarkly/lua-server-sdk/issues/31))

### Features

* added `makeContext` for constructing single or multi-kind contexts ([95e9718](https://github.com/launchdarkly/lua-server-sdk/commit/95e97188dd2258805734884592b601c7ebfa66c6))
* use C++ Server-side SDK 3.0 bindings ([#31](https://github.com/launchdarkly/lua-server-sdk/issues/31)) ([95e9718](https://github.com/launchdarkly/lua-server-sdk/commit/95e97188dd2258805734884592b601c7ebfa66c6))


### Code Refactoring

* makeUser behavior modified to construct a user-kind context ([95e9718](https://github.com/launchdarkly/lua-server-sdk/commit/95e97188dd2258805734884592b601c7ebfa66c6))
* remove `alias` function, replace usage with multi-kind contexts ([95e9718](https://github.com/launchdarkly/lua-server-sdk/commit/95e97188dd2258805734884592b601c7ebfa66c6))
* remove `inlineUsersInEvents` and `userKeysFlushInterval` config options ([95e9718](https://github.com/launchdarkly/lua-server-sdk/commit/95e97188dd2258805734884592b601c7ebfa66c6))
* remove global `registerLogger` function, replace with config option ([95e9718](https://github.com/launchdarkly/lua-server-sdk/commit/95e97188dd2258805734884592b601c7ebfa66c6))
* SDK configuration exposes new options and is organized hierarchically ([95e9718](https://github.com/launchdarkly/lua-server-sdk/commit/95e97188dd2258805734884592b601c7ebfa66c6))
* Variation and VariationDetail now take contexts ([95e9718](https://github.com/launchdarkly/lua-server-sdk/commit/95e97188dd2258805734884592b601c7ebfa66c6))

## [1.2.2] - 2022-02-08
### Fixed:
- Remove accidental check-in of temporary documentation build files.

## [1.2.1] - 2022-02-08
### Changed:
- Updated release configuration.

## [1.2.0] - 2022-02-07
### Added:
- Added `version()` function to retrieve SDK version

### Fixed:
- Fixed memory leak in `AllFlags` API.

### Removed:
- Removed references to unused `cjson` package.

## [1.1.0] - 2021-02-04
### Added:
- Added the `alias` method. This can be used to associate two user objects for analytics purposes by generating an alias event.

## [1.0.1] - 2020-08-28
### Fixed:
- Conflicting definition of `luaL_setfuncs` in certain versions of the Lua C API.


## [1.0.0] - 2020-07-27
First supported GA release.

### Fixed:
- Set `source.url` field to repository in rockspecs

## [1.0.0-beta.3] - 2020-07-20

### Added:
- Added support for Lua `5.2` and Lua `5.3`.
- Added `featureStoreBackend` option to configuration object.
- Added `launchdarkly_server_sdk_redis` module to support Redis as an external feature store.
- Added `registerLogger` which allows configuration of SDK logging capabilities.

### Changed:
- The SDK is now implemented as a compiled C module instead of directly in Lua.
- The SDK is now imported as `launchdarkly_server_sdk`, instead of `launchdarkly-server-sdk.lua`.
- The SDK no longer depends on the Lua `cjson`, or Lua `ffi` libraries.
- All methods are no longer closures. Use `client:aFunction`, instead of `client.aFunction`.

## [1.0.0-beta.2] - 2020-05-12

### Changed:
- Updates the configuration object to include wrapper name and version.

## [1.0.0-beta.1] - 2020-03-24

Initial beta release.
