# Change log

All notable changes to the LaunchDarkly Lua Server-side SDK will be documented in this file. This project adheres to [Semantic Versioning](http://semver.org).

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
