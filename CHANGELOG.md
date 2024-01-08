# Change log

All notable changes to the LaunchDarkly Lua Server-side SDK will be documented in this file. This project adheres to [Semantic Versioning](http://semver.org).

## [2.0.1](https://github.com/launchdarkly/lua-server-sdk/compare/v2.0.0...v2.0.1) (2024-01-08)


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
