--- Server-side SDK for LaunchDarkly Redis store.
-- @module launchdarkly-server-sdk-redis

local ffi = require("ffi")

ffi.cdef[[
    struct LDRedisConfig;
    struct LDStoreInterface;

    struct LDRedisConfig *LDRedisConfigNew();

    bool LDRedisConfigSetHost(
        struct LDRedisConfig *const config,
        const char *const host);

    bool LDRedisConfigSetPort(
        struct LDRedisConfig *const config,
        const unsigned short port);

    bool LDRedisConfigSetPrefix(
        struct LDRedisConfig *const config,
        const char *const prefix);

    bool LDRedisConfigSetPoolSize(
        struct LDRedisConfig *const config,
        const unsigned int poolSize);

    bool LDRedisConfigFree(struct LDRedisConfig *const config);

    struct LDStoreInterface *LDStoreInterfaceRedisNew(
        struct LDRedisConfig *const config);
]]

local so = ffi.load("ldserverapi-redis")

local function applyWhenNotNil(subject, operation, value)
    if value ~= nil and value ~= cjson.null then
        operation(subject, value)
    end
end

--- Initialize a store backend
-- @tparam table fields list of configuration options
-- @tparam[opt] string fields.host Hostname for Redis.
-- @tparam[opt] int fields.port Port for Redis.
-- @tparam[opt] string fields.prefix Redis key prefix for SDK values.
-- @tparam[opt] int fields.poolSize Number of Redis connections to maintain.
-- @return A fresh Redis store backend.
local function makeStore(fields)
    local config = so.LDRedisConfigNew()

    applyWhenNotNil(config, so.LDRedisConfigSetHost, fields["host"])
    applyWhenNotNil(config, so.LDRedisConfigSetPort, fields["port"])
    applyWhenNotNil(config, so.LDRedisConfigSetPrefix, fields["prefix"])
    applyWhenNotNil(config, so.LDRedisConfigSetPoolSize, fields["poolSize"])

    return so.LDStoreInterfaceRedisNew(config)
end

-- @export
return {
    makeStore = makeStore
}
