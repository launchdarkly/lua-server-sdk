/***
Server-side SDK for LaunchDarkly.
@module launchdarkly-server-sdk
*/

#include <lua.h>
#include <lauxlib.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <launchdarkly/api.h>

#define SDKVersion "1.1.0"

static struct LDJSON *
LuaValueToJSON(lua_State *const l, const int i);

static struct LDJSON *
LuaTableToJSON(lua_State *const l, const int i);

static struct LDJSON *
LuaArrayToJSON(lua_State *const l, const int i);

static void
LuaPushJSON(lua_State *const l, const struct LDJSON *const j);

static int globalLoggingCallback;
static lua_State *globalLuaState;

static void
logHandler(const LDLogLevel level, const char * const line)
{
    lua_rawgeti(globalLuaState, LUA_REGISTRYINDEX, globalLoggingCallback);

    lua_pushstring(globalLuaState, LDLogLevelToString(level));
    lua_pushstring(globalLuaState, line);

    lua_call(globalLuaState, 2, 0);
}

static LDLogLevel
LuaStringToLogLevel(const char *const text)
{
    if (strcmp(text, "FATAL") == 0) {
        return LD_LOG_FATAL;
    } else if (strcmp(text, "CRITICAL") == 0) {
        return LD_LOG_CRITICAL;
    } else if (strcmp(text, "ERROR") == 0) {
        return LD_LOG_ERROR;
    } else if (strcmp(text, "WARNING") == 0) {
        return LD_LOG_WARNING;
    } else if (strcmp(text, "INFO") == 0) {
        return LD_LOG_INFO;
    } else if (strcmp(text, "DEBUG") == 0) {
        return LD_LOG_DEBUG;
    } else if (strcmp(text, "TRACE") == 0) {
        return LD_LOG_TRACE;
    }

    return LD_LOG_INFO;
}

/***
Set the global logger for all SDK operations. This function is not thread
safe, and if used should be done so before other operations. The default
log level is "INFO".
@function registerLogger
@tparam string logLevel The level to at. Available options are:
"FATAL", "CRITICAL", "ERROR", "WARNING", "INFO", "DEBUG", "TRACE".
@tparam function cb The logging handler. Callback must be of the form
"function (logLevel, logLine) ... end".
*/
static int
LuaLDRegisterLogger(lua_State *const l)
{
    if (lua_gettop(l) != 2) {
        return luaL_error(l, "expecting exactly 2 arguments");
    }

    const char *const level = luaL_checkstring(l, 1);

    globalLuaState        = l;
    globalLoggingCallback = luaL_ref(l, LUA_REGISTRYINDEX);

    LDConfigureGlobalLogger(LuaStringToLogLevel(level), logHandler);

    return 0;
}

static bool
isArray(lua_State *const l, const int i)
{
    lua_pushvalue(l, i);

    lua_pushnil(l);

    bool array = true;

    while (lua_next(l, -2) != 0) {
        if (lua_type(l, -2) != LUA_TNUMBER) {
            array = false;
        }

        lua_pop(l, 1);
    }

    lua_pop(l, 1);

    return array;
}

static struct LDJSON *
LuaValueToJSON(lua_State *const l, const int i)
{
    struct LDJSON *result;

    switch (lua_type(l, i)) {
        case LUA_TBOOLEAN:
            result = LDNewBool(lua_toboolean(l, i));
            break;
        case LUA_TNUMBER:
            result = LDNewNumber(lua_tonumber(l, i));
            break;
        case LUA_TSTRING:
            result = LDNewText(lua_tostring(l, i));
            break;
        case LUA_TTABLE:
            if (isArray(l, i)) {
                result = LuaArrayToJSON(l, i);
            } else {
                result = LuaTableToJSON(l, i);
            }
            break;
        default:
            result = LDNewNull();
            break;
    }

    return result;
}

static struct LDJSON *
LuaArrayToJSON(lua_State *const l, const int i)
{
    struct LDJSON *const result = LDNewArray();

    lua_pushvalue(l, i);

    lua_pushnil(l);

    while (lua_next(l, -2) != 0) {
        struct LDJSON *value = LuaValueToJSON(l, -1);

        LDArrayPush(result, value);

        lua_pop(l, 1);
    }

    lua_pop(l, 1);

    return result;
}

static struct LDJSON *
LuaTableToJSON(lua_State *const l, const int i)
{
    struct LDJSON *const result = LDNewObject();

    lua_pushvalue(l, i);

    lua_pushnil(l);

    while (lua_next(l, -2) != 0) {
        const char *const key      = lua_tostring(l, -2);
        struct LDJSON *const value = LuaValueToJSON(l, -1);

        LDObjectSetKey(result, key, value);

        lua_pop(l, 1);
    }

    lua_pop(l, 1);

    return result;
}

static void
LuaPushJSONObject(lua_State *const l, const struct LDJSON *const j)
{
    struct LDJSON *iter;

    lua_newtable(l);

    for (iter = LDGetIter(j); iter; iter = LDIterNext(iter)) {
        LuaPushJSON(l, iter);
        lua_setfield(l, -2, LDIterKey(iter));
    }
}

static void
LuaPushJSONArray(lua_State *const l, const struct LDJSON *const j)
{
    struct LDJSON *iter;

    lua_newtable(l);

    int index = 1;

    for (iter = LDGetIter(j); iter; iter = LDIterNext(iter)) {
        LuaPushJSON(l, iter);
        lua_rawseti(l, -2, index);
        index++;
    }
}

static void
LuaPushJSON(lua_State *const l, const struct LDJSON *const j)
{
    switch (LDJSONGetType(j)) {
        case LDText:
            lua_pushstring(l, LDGetText(j));
            break;
        case LDBool:
            lua_pushboolean(l, LDGetBool(j));
            break;
        case LDNumber:
            lua_pushnumber(l, LDGetNumber(j));
            break;
        case LDObject:
            LuaPushJSONObject(l, j);
            break;
        case LDArray:
            LuaPushJSONArray(l, j);
            break;
        default:
            lua_pushnil(l);
            break;
    }

    return;
}

/***
Create a new opaque user object.
@function makeUser
@tparam table fields list of user fields.
@tparam string fields.key The user's key
@tparam[opt] boolean fields.anonymous Mark the user as anonymous
@tparam[opt] string fields.ip Set the user's IP
@tparam[opt] string fields.firstName Set the user's first name
@tparam[opt] string fields.lastName Set the user's last name
@tparam[opt] string fields.email Set the user's email
@tparam[opt] string fields.name Set the user's name
@tparam[opt] string fields.avatar Set the user's avatar
@tparam[opt] string fields.country Set the user's country
@tparam[opt] string fields.secondary Set the user's secondary key
@tparam[opt] table fields.privateAttributeNames A list of attributes to
redact
@tparam[opt] table fields.custom Set the user's custom JSON
@return an opaque user object
*/
static int
LuaLDUserNew(lua_State *const l)
{
    if (lua_gettop(l) != 1) {
        return luaL_error(l, "expecting exactly 1 argument");
    }

    luaL_checktype(l, 1, LUA_TTABLE);

    lua_getfield(l, 1, "key");

    const char *const key = luaL_checkstring(l, -1);

    struct LDUser *user = LDUserNew(key);

    lua_getfield(l, 1, "anonymous");

    if (lua_isboolean(l, -1)) {
        LDUserSetAnonymous(user, lua_toboolean(l, -1));
    }

    lua_getfield(l, 1, "ip");

    if (lua_isstring(l, -1)) {
        LDUserSetIP(user, luaL_checkstring(l,-1));
    };

    lua_getfield(l, 1, "firstName");

    if (lua_isstring(l, -1)) {
        LDUserSetFirstName(user, luaL_checkstring(l,-1));
    }

    lua_getfield(l, 1, "lastName");

    if (lua_isstring(l, -1)) {
        LDUserSetLastName(user, luaL_checkstring(l, -1));
    }

    lua_getfield(l, 1, "email");

    if (lua_isstring(l, -1)) {
        LDUserSetEmail(user, luaL_checkstring(l, -1));
    }

    lua_getfield(l, 1, "name");

    if (lua_isstring(l, -1)) {
        LDUserSetName(user, luaL_checkstring(l, -1));
    }

    lua_getfield(l, 1, "avatar");

    if (lua_isstring(l, -1)) {
        LDUserSetAvatar(user, luaL_checkstring(l, -1));
    }

    lua_getfield(l, 1, "country");

    if (lua_isstring(l, -1)) {
        LDUserSetCountry(user, luaL_checkstring(l, -1));
    }

    lua_getfield(l, 1, "secondary");

    if (lua_isstring(l, -1)) {
        LDUserSetSecondary(user, luaL_checkstring(l, -1));
    }

    lua_getfield(l, 1, "custom");

    if (lua_istable(l, -1)) {
        LDUserSetCustom(user, LuaValueToJSON(l, -1));
    }

    lua_getfield(l, 1, "privateAttributeNames");

    if (lua_istable(l, -1)) {
        struct LDJSON *attrs, *iter;
        attrs = LuaValueToJSON(l, -1);

        for (iter = LDGetIter(attrs); iter; iter = LDIterNext(iter)) {
            LDUserAddPrivateAttribute(user, LDGetText(iter));
        }

        LDJSONFree(attrs);
    }

    struct LDUser **u = (struct LDUser **)lua_newuserdata(l, sizeof(user));

    *u = user;

    luaL_getmetatable(l, "LaunchDarklyUser");
    lua_setmetatable(l, -2);

    return 1;
}

static int
LuaLDUserFree(lua_State *const l)
{
    struct LDUser **user;

    user = (struct LDUser **)luaL_checkudata(l, 1, "LaunchDarklyUser");

    if (*user) {
        LDUserFree(*user);
        *user = NULL;
    }

    return 0;
}

static struct LDConfig *
makeConfig(lua_State *const l, const int i)
{
    struct LDConfig *config;

    luaL_checktype(l, i, LUA_TTABLE);

    lua_getfield(l, i, "key");

    const char *const key = luaL_checkstring(l, -1);

    config = LDConfigNew(key);

    lua_getfield(l, i, "baseURI");

    if (lua_isstring(l, -1)) {
        LDConfigSetBaseURI(config, luaL_checkstring(l, -1));
    }

    lua_getfield(l, i, "streamURI");

    if (lua_isstring(l, -1)) {
        LDConfigSetStreamURI(config, luaL_checkstring(l, -1));
    }

    lua_getfield(l, i, "eventsURI");

    if (lua_isstring(l, -1)) {
        LDConfigSetEventsURI(config, luaL_checkstring(l, -1));
    }

    lua_getfield(l, i, "stream");

    if (lua_isboolean(l, -1)) {
        LDConfigSetStream(config, lua_toboolean(l, -1));
    }

    lua_getfield(l, i, "sendEvents");

    if (lua_isstring(l, -1)) {
        LDConfigSetSendEvents(config, lua_toboolean(l, -1));
    }

    lua_getfield(l, i, "eventsCapacity");

    if (lua_isnumber(l, -1)) {
        LDConfigSetEventsCapacity(config, luaL_checkinteger(l, -1));
    }

    lua_getfield(l, i, "timeout");

    if (lua_isnumber(l, -1)) {
        LDConfigSetTimeout(config, luaL_checkinteger(l, -1));
    }

    lua_getfield(l, i, "flushInterval");

    if (lua_isnumber(l, -1)) {
        LDConfigSetFlushInterval(config, luaL_checkinteger(l, -1));
    }

    lua_getfield(l, i, "pollInterval");

    if (lua_isnumber(l, -1)) {
        LDConfigSetPollInterval(config, luaL_checkinteger(l, -1));
    }

    lua_getfield(l, i, "offline");

    if (lua_isboolean(l, -1)) {
        LDConfigSetOffline(config, lua_toboolean(l, -1));
    }

    lua_getfield(l, i, "useLDD");

    if (lua_isboolean(l, -1)) {
        LDConfigSetUseLDD(config, lua_toboolean(l, -1));
    }

    lua_getfield(l, i, "inlineUsersInEvents");

    if (lua_isboolean(l, -1)) {
        LDConfigInlineUsersInEvents(config, lua_toboolean(l, -1));
    }

    lua_getfield(l, i, "allAttributesPrivate");

    if (lua_isboolean(l, -1)) {
        LDConfigSetAllAttributesPrivate(config, lua_toboolean(l, -1));
    }

    lua_getfield(l, i, "userKeysCapacity");

    if (lua_isnumber(l, -1)) {
        LDConfigSetUserKeysCapacity(config, luaL_checkinteger(l, -1));
    }

    lua_getfield(l, i, "featureStoreBackend");

    if (lua_isuserdata(l, -1)) {
        struct LDStoreInterface **storeInterface;

        storeInterface = (struct LDStoreInterface **)
            luaL_checkudata(l, -1, "LaunchDarklyStoreInterface");

        LDConfigSetFeatureStoreBackend(config, *storeInterface);
    }

    lua_getfield(l, 1, "privateAttributeNames");

    if (lua_istable(l, -1)) {
        struct LDJSON *attrs, *iter;
        attrs = LuaValueToJSON(l, -1);

        for (iter = LDGetIter(attrs); iter; iter = LDIterNext(iter)) {
            LDConfigAddPrivateAttribute(config, LDGetText(iter));
        }

        LDJSONFree(attrs);
    }

    LDConfigSetWrapperInfo(config, "lua-server-sdk", SDKVersion);

    return config;
}

/***
Initialize a new client, and connect to LaunchDarkly.
@function makeClient
@tparam table config list of configuration options
@tparam string config.key Environment SDK key
@tparam[opt] string config.baseURI Set the base URI for connecting to
LaunchDarkly. You probably don't need to set this unless instructed by
LaunchDarkly.
@tparam[opt] string config.streamURI Set the streaming URI for connecting to
LaunchDarkly. You probably don't need to set this unless instructed by
LaunchDarkly.
@tparam[opt] string config.eventsURI Set the events URI for connecting to
LaunchDarkly. You probably don't need to set this unless instructed by
LaunchDarkly.
@tparam[opt] boolean config.stream Enables or disables real-time streaming
flag updates. When set to false, an efficient caching polling mechanism is
used. We do not recommend disabling streaming unless you have been instructed
to do so by LaunchDarkly support. Defaults to true.
@tparam[opt] string config.sendEvents Sets whether to send analytics events
back to LaunchDarkly. By default, the client will send events. This differs
from Offline in that it only affects sending events, not streaming or
polling.
@tparam[opt] int config.eventsCapacity The capacity of the events buffer.
The client buffers up to this many events in memory before flushing. If the
capacity is exceeded before the buffer is flushed, events will be discarded.
@tparam[opt] int config.timeout The connection timeout to use when making
requests to LaunchDarkly.
@tparam[opt] int config.flushInterval he time between flushes of the event
buffer. Decreasing the flush interval means that the event buffer is less
likely to reach capacity.
@tparam[opt] int config.pollInterval The polling interval
(when streaming is disabled) in milliseconds.
@tparam[opt] boolean config.offline Sets whether this client is offline.
An offline client will not make any network connections to LaunchDarkly,
and will return default values for all feature flags.
@tparam[opt] boolean config.allAttributesPrivate Sets whether or not all user
attributes (other than the key) should be hidden from LaunchDarkly. If this
is true, all user attribute values will be private, not just the attributes
specified in PrivateAttributeNames.
@tparam[opt] boolean config.inlineUsersInEvents Set to true if you need to
see the full user details in every analytics event.
@tparam[opt] int config.userKeysCapacity The number of user keys that the
event processor can remember at an one time, so that duplicate user details
will not be sent in analytics.
@tparam[opt] int config.userKeysFlushInterval The interval at which the event
processor will reset its set of known user keys, in milliseconds.
@tparam[opt] table config.privateAttributeNames Marks a set of user attribute
names private. Any users sent to LaunchDarkly with this configuration active
will have attributes with these names removed.
@param[opt] backend config.featureStoreBackend Persistent feature store
backend.
@tparam int timeoutMilliseconds How long to wait for flags to
download. If the timeout is reached a non fully initialized client will be
returned.
@return A fresh client.
*/
static int
LuaLDClientInit(lua_State *const l)
{
    struct LDClient *client;
    struct LDConfig *config;
    unsigned int timeout;

    if (lua_gettop(l) != 2) {
        return luaL_error(l, "expecting exactly 2 arguments");
    }

    config = makeConfig(l, 1);

    timeout = luaL_checkinteger(l, 2);

    client = LDClientInit(config, timeout);

    struct LDClient **c =
        (struct LDClient **)lua_newuserdata(l, sizeof(client));

    *c = client;

    luaL_getmetatable(l, "LaunchDarklyClient");
    lua_setmetatable(l, -2);

    return 1;
}

static int
LuaLDClientClose(lua_State *const l)
{
    struct LDClient **client;

    client = (struct LDClient **)luaL_checkudata(l, 1, "LaunchDarklyClient");

    if (*client) {
        LDClientClose(*client);
        *client = NULL;
    }

    return 0;
}

static void
LuaPushDetails(lua_State *const l, struct LDDetails *const details,
    struct LDJSON *const value)
{
    struct LDJSON *reason;

    reason = LDReasonToJSON(details);

    lua_newtable(l);

    LuaPushJSON(l, reason);
    lua_setfield(l, -2, "reason");

    if (details->hasVariation) {
        lua_pushnumber(l, details->variationIndex);
        lua_setfield(l, -2, "variationIndex");
    }

    LuaPushJSON(l, value);
    lua_setfield(l, -2, "value");

    LDDetailsClear(details);
    LDJSONFree(value);
    LDJSONFree(reason);
}

/**
Evaluate a boolean flag
@class function
@name boolVariation
@tparam user user An opaque user object from @{makeUser}
@tparam string key The key of the flag to evaluate.
@tparam boolean fallback The value to return on error
@treturn boolean The evaluation result, or the fallback value
*/
static int
LuaLDClientBoolVariation(lua_State *const l)
{
    struct LDClient **client;
    struct LDUser **user;

    if (lua_gettop(l) != 4) {
        return luaL_error(l, "expecting exactly 4 arguments");
    }

    client = (struct LDClient **)luaL_checkudata(l, 1, "LaunchDarklyClient");

    user = (struct LDUser **)luaL_checkudata(l, 2, "LaunchDarklyUser");

    const char *const key = luaL_checkstring(l, 3);

    const int fallback = lua_toboolean(l, 4);

    const LDBoolean result =
        LDBoolVariation(*client, *user, key, fallback, NULL);

    lua_pushboolean(l, result);

    return 1;
}

/***
Evaluate a boolean flag and return an explanation
@class function
@name boolVariationDetail
@tparam user user An opaque user object from @{makeUser}
@tparam string key The key of the flag to evaluate.
@tparam boolean fallback The value to return on error
@treturn table The evaluation explanation
*/
static int
LuaLDClientBoolVariationDetail(lua_State *const l)
{
    struct LDClient **client;
    struct LDUser **user;
    struct LDDetails details;

    if (lua_gettop(l) != 4) {
        return luaL_error(l, "expecting exactly 4 arguments");
    }

    LDDetailsInit(&details);

    client = (struct LDClient **)luaL_checkudata(l, 1, "LaunchDarklyClient");

    user = (struct LDUser **)luaL_checkudata(l, 2, "LaunchDarklyUser");

    const char *const key = luaL_checkstring(l, 3);

    const int fallback = lua_toboolean(l, 4);

    const LDBoolean result =
        LDBoolVariation(*client, *user, key, fallback, &details);

    LuaPushDetails(l, &details, LDNewBool(result));

    return 1;
}

/***
Evaluate an integer flag
@class function
@name intVariation
@tparam user user An opaque user object from @{makeUser}
@tparam string key The key of the flag to evaluate.
@tparam int fallback The value to return on error
@treturn int The evaluation result, or the fallback value
*/
static int
LuaLDClientIntVariation(lua_State *const l)
{
    struct LDClient **client;
    struct LDUser **user;

    if (lua_gettop(l) != 4) {
        return luaL_error(l, "expecting exactly 4 arguments");
    }

    client = (struct LDClient **)luaL_checkudata(l, 1, "LaunchDarklyClient");

    user = (struct LDUser **)luaL_checkudata(l, 2, "LaunchDarklyUser");

    const char *const key = luaL_checkstring(l, 3);

    const int fallback = luaL_checkinteger(l, 4);

    const int result = LDIntVariation(*client, *user, key, fallback, NULL);

    lua_pushnumber(l, result);

    return 1;
}

/***
Evaluate an integer flag and return an explanation
@class function
@name intVariationDetail
@tparam user user An opaque user object from @{makeUser}
@tparam string key The key of the flag to evaluate.
@tparam int fallback The value to return on error
@treturn table The evaluation explanation
*/
static int
LuaLDClientIntVariationDetail(lua_State *const l)
{
    struct LDClient **client;
    struct LDUser **user;
    struct LDDetails details;

    if (lua_gettop(l) != 4) {
        return luaL_error(l, "expecting exactly 4 arguments");
    }

    LDDetailsInit(&details);

    client = (struct LDClient **)luaL_checkudata(l, 1, "LaunchDarklyClient");

    user = (struct LDUser **)luaL_checkudata(l, 2, "LaunchDarklyUser");

    const char *const key = luaL_checkstring(l, 3);

    const int fallback = luaL_checkinteger(l, 4);

    const int result = LDIntVariation(*client, *user, key, fallback, &details);

    LuaPushDetails(l, &details, LDNewNumber(result));

    return 1;
}

/***
Evaluate a double flag
@class function
@name doubleVariation
@tparam user user An opaque user object from @{makeUser}
@tparam string key The key of the flag to evaluate.
@tparam number fallback The value to return on error
@treturn double The evaluation result, or the fallback value
*/
static int
LuaLDClientDoubleVariation(lua_State *const l)
{
    struct LDClient **client;
    struct LDUser **user;

    if (lua_gettop(l) != 4) {
        return luaL_error(l, "expecting exactly 4 arguments");
    }

    client = (struct LDClient **)luaL_checkudata(l, 1, "LaunchDarklyClient");

    user = (struct LDUser **)luaL_checkudata(l, 2, "LaunchDarklyUser");

    const char *const key = luaL_checkstring(l, 3);

    const double fallback = lua_tonumber(l, 4);

    const double result =
        LDDoubleVariation(*client, *user, key, fallback, NULL);

    lua_pushnumber(l, result);

    return 1;
}

/***
Evaluate a double flag and return an explanation
@class function
@name doubleVariationDetail
@tparam user user An opaque user object from @{makeUser}
@tparam string key The key of the flag to evaluate.
@tparam number fallback The value to return on error
@treturn table The evaluation explanation
*/
static int
LuaLDClientDoubleVariationDetail(lua_State *const l)
{
    struct LDClient **client;
    struct LDUser **user;
    struct LDDetails details;

    if (lua_gettop(l) != 4) {
        return luaL_error(l, "expecting exactly 4 arguments");
    }

    LDDetailsInit(&details);

    client = (struct LDClient **)luaL_checkudata(l, 1, "LaunchDarklyClient");

    user = (struct LDUser **)luaL_checkudata(l, 2, "LaunchDarklyUser");

    const char *const key = luaL_checkstring(l, 3);

    const double fallback = lua_tonumber(l, 4);

    const double result =
        LDDoubleVariation(*client, *user, key, fallback, &details);

    LuaPushDetails(l, &details, LDNewNumber(result));

    return 1;
}

/***
Evaluate a string flag
@class function
@name stringVariation
@tparam user user An opaque user object from @{makeUser}
@tparam string key The key of the flag to evaluate.
@tparam string fallback The value to return on error
@treturn string The evaluation result, or the fallback value
*/
static int
LuaLDClientStringVariation(lua_State *const l)
{
    struct LDClient **client;
    struct LDUser **user;

    if (lua_gettop(l) != 4) {
        return luaL_error(l, "expecting exactly 4 arguments");
    }

    client = (struct LDClient **)luaL_checkudata(l, 1, "LaunchDarklyClient");

    user = (struct LDUser **)luaL_checkudata(l, 2, "LaunchDarklyUser");

    const char *const key = luaL_checkstring(l, 3);

    const char *const fallback = luaL_checkstring(l, 4);

    char *const result =
        LDStringVariation(*client, *user, key, fallback, NULL);

    lua_pushstring(l, result);

    LDFree(result);

    return 1;
}

/***
Evaluate a string flag and return an explanation
@class function
@name stringVariationDetail
@tparam user user An opaque user object from @{makeUser}
@tparam string key The key of the flag to evaluate.
@tparam string fallback The value to return on error
@treturn table The evaluation explanation
*/
static int
LuaLDClientStringVariationDetail(lua_State *const l)
{
    struct LDClient **client;
    struct LDUser **user;
    struct LDDetails details;

    if (lua_gettop(l) != 4) {
        return luaL_error(l, "expecting exactly 4 arguments");
    }

    LDDetailsInit(&details);

    client = (struct LDClient **)luaL_checkudata(l, 1, "LaunchDarklyClient");

    user = (struct LDUser **)luaL_checkudata(l, 2, "LaunchDarklyUser");

    const char *const key = luaL_checkstring(l, 3);

    const char *const fallback = luaL_checkstring(l, 4);

    char *const result =
        LDStringVariation(*client, *user, key, fallback, &details);

    LuaPushDetails(l, &details, LDNewText(result));

    LDFree(result);

    return 1;
}

/***
Evaluate a json flag
@class function
@name jsonVariation
@tparam user user An opaque user object from @{makeUser}
@tparam string key The key of the flag to evaluate.
@tparam table fallback The value to return on error
@treturn table The evaluation result, or the fallback value
*/
static int
LuaLDClientJSONVariation(lua_State *const l)
{
    struct LDClient **client;
    struct LDUser **user;
    struct LDJSON *fallback, *result;

    if (lua_gettop(l) != 4) {
        return luaL_error(l, "expecting exactly 4 arguments");
    }

    client = (struct LDClient **)luaL_checkudata(l, 1, "LaunchDarklyClient");

    user = (struct LDUser **)luaL_checkudata(l, 2, "LaunchDarklyUser");

    const char *const key = luaL_checkstring(l, 3);

    fallback = LuaValueToJSON(l, 4);

    result = LDJSONVariation(*client, *user, key, fallback, NULL);

    LuaPushJSON(l, result);

    LDJSONFree(fallback);
    LDJSONFree(result);

    return 1;
}

/***
Evaluate a json flag and return an explanation
@class function
@name jsonVariationDetail
@tparam user user An opaque user object from @{makeUser}
@tparam string key The key of the flag to evaluate.
@tparam table fallback The value to return on error
@treturn table The evaluation explanation
*/
static int
LuaLDClientJSONVariationDetail(lua_State *const l)
{
    struct LDClient **client;
    struct LDUser **user;
    struct LDJSON *fallback, *result;
    struct LDDetails details;

    if (lua_gettop(l) != 4) {
        return luaL_error(l, "expecting exactly 4 arguments");
    }

    LDDetailsInit(&details);

    client = (struct LDClient **)luaL_checkudata(l, 1, "LaunchDarklyClient");

    user = (struct LDUser **)luaL_checkudata(l, 2, "LaunchDarklyUser");

    const char *const key = luaL_checkstring(l, 3);

    fallback = LuaValueToJSON(l, 4);

    result = LDJSONVariation(*client, *user, key, fallback, &details);

    LuaPushDetails(l, &details, result);

    LDJSONFree(fallback);

    return 1;
}

/***
Immediately flushes queued events.
@function flush
@treturn nil
*/
static int
LuaLDClientFlush(lua_State *const l)
{
    struct LDClient **client;

    if (lua_gettop(l) != 1) {
        return luaL_error(l, "expecting exactly 1 argument");
    }

    client = (struct LDClient **)luaL_checkudata(l, 1, "LaunchDarklyClient");

    LDClientFlush(*client);

    return 0;
}

/***
Reports that a user has performed an event. Custom data, and a metric
can be attached to the event as JSON.
@function track
@tparam string key The name of the event
@tparam user user An opaque user object from @{makeUser}
@tparam[opt] table data A value to be associated with an event
@tparam[optchain] number metric A value to be associated with an event
@treturn nil
*/
static int
LuaLDClientTrack(lua_State *const l)
{
    struct LDClient **client;
    struct LDUser **user;
    struct LDJSON *value;

    if (lua_gettop(l) < 3 || lua_gettop(l) > 5) {
        return luaL_error(l, "expecting 3-5 arguments");
    }

    client = (struct LDClient **)luaL_checkudata(l, 1, "LaunchDarklyClient");

    const char *const key = luaL_checkstring(l, 2);

    user = (struct LDUser **)luaL_checkudata(l, 3, "LaunchDarklyUser");

    if (lua_isnil(l, 4)) {
        value = NULL;
    } else {
        value = LuaValueToJSON(l, 4);
    }

    if (lua_gettop(l) == 5 && lua_isnumber(l, 5)) {
        const double metric = luaL_checknumber(l, 5);

        LDClientTrackMetric(*client, key, *user, value, metric);
    } else {
        LDClientTrack(*client, key, *user, value);
    }

    return 0;
}

/***
Associates two users for analytics purposes by generating an alias event.
@function alias
@tparam user currentUser An opaque user object from @{makeUser}
@tparam user previousUser An opaque user object from @{makeUser}
@treturn nil
*/
static int
LuaLDClientAlias(lua_State *const l)
{
    struct LDClient **client;
    struct LDUser **currentUser, **previousUser;
    struct LDJSON *value;

    if (lua_gettop(l) != 3) {
        return luaL_error(l, "expected exactly three arguments");
    }

    client = (struct LDClient **)luaL_checkudata(l, 1, "LaunchDarklyClient");
    currentUser = (struct LDUser **)luaL_checkudata(l, 2, "LaunchDarklyUser");
    previousUser = (struct LDUser **)luaL_checkudata(l, 3, "LaunchDarklyUser");

    if (!LDClientAlias(*client, *currentUser, *previousUser)) {
        return luaL_error(l, "LDClientAlias failed");
    }

    return 0;
}

/***
Check if a client has been fully initialized. This may be useful if the
initialization timeout was reached.
@function isInitialized
@treturn boolean true if fully initialized
*/
static int
LuaLDClientIsInitialized(lua_State *const l)
{
    struct LDClient **client;

    if (lua_gettop(l) != 1) {
        return luaL_error(l, "expecting exactly 1 argument");
    }

    client = (struct LDClient **)luaL_checkudata(l, 1, "LaunchDarklyClient");

    lua_pushboolean(l, LDClientIsInitialized(*client));

    return 1;
}

/***
Generates an identify event for a user.
@function identify
@tparam user user An opaque user object from @{makeUser}
@treturn nil
*/
static int
LuaLDClientIdentify(lua_State *const l)
{
    struct LDClient **client;
    struct LDUser **user;

    if (lua_gettop(l) != 2) {
        return luaL_error(l, "expecting exactly 2 arguments");
    }

    client = (struct LDClient **)luaL_checkudata(l, 1, "LaunchDarklyClient");
    user = (struct LDUser **)luaL_checkudata(l, 2, "LaunchDarklyUser");

    LDClientIdentify(*client, *user);

    return 0;
}

/***
Returns a map from feature flag keys to values for a given user.
This does not send analytics events back to LaunchDarkly.
@function allFlags
@tparam user user An opaque user object from @{makeUser}
@treturn table
*/
static int
LuaLDClientAllFlags(lua_State *const l)
{
    struct LDClient **client;
    struct LDUser **user;
    struct LDJSON *result;

    if (lua_gettop(l) != 2) {
        return luaL_error(l, "expecting exactly 2 arguments");
    }

    client = (struct LDClient **)luaL_checkudata(l, 1, "LaunchDarklyClient");

    user = (struct LDUser **)luaL_checkudata(l, 2, "LaunchDarklyUser");

    result = LDAllFlags(*client, *user);

    LuaPushJSON(l, result);

    return 1;
}

static const struct luaL_Reg launchdarkly_functions[] = {
    { "clientInit",     LuaLDClientInit     },
    { "makeUser",       LuaLDUserNew        },
    { "registerLogger", LuaLDRegisterLogger },
    { NULL,             NULL                }
};

static const struct luaL_Reg launchdarkly_client_methods[] = {
    { "boolVariation",         LuaLDClientBoolVariation         },
    { "boolVariationDetail",   LuaLDClientBoolVariationDetail   },
    { "intVariation",          LuaLDClientIntVariation          },
    { "intVariationDetail",    LuaLDClientIntVariationDetail    },
    { "doubleVariation",       LuaLDClientDoubleVariation       },
    { "doubleVariationDetail", LuaLDClientDoubleVariationDetail },
    { "stringVariation",       LuaLDClientStringVariation       },
    { "stringVariationDetail", LuaLDClientStringVariationDetail },
    { "jsonVariation",         LuaLDClientJSONVariation         },
    { "jsonVariationDetail",   LuaLDClientJSONVariationDetail   },
    { "flush",                 LuaLDClientFlush                 },
    { "track",                 LuaLDClientTrack                 },
    { "alias",                 LuaLDClientAlias                 },
    { "allFlags",              LuaLDClientAllFlags              },
    { "isInitialized",         LuaLDClientIsInitialized         },
    { "identify",              LuaLDClientIdentify              },
    { "__gc",                  LuaLDClientClose                 },
    { NULL,                    NULL                             }
};

static const struct luaL_Reg launchdarkly_user_methods[] = {
    { "__gc", LuaLDUserFree },
    { NULL,   NULL          }
};

static const struct luaL_Reg launchdarkly_store_methods[] = {
    { NULL, NULL }
};

/*
** Adapted from Lua 5.2.0
*/
static void
ld_luaL_setfuncs(lua_State *L, const luaL_Reg *l, int nup)
{
    luaL_checkstack(L, nup+1, "too many upvalues");
    for (; l->name != NULL; l++) {  /* fill the table with given functions */
        int i;
        lua_pushstring(L, l->name);
         /* copy upvalues to the top */
        for (i = 0; i < nup; i++) {
            lua_pushvalue(L, -(nup+1));
        }
        lua_pushcclosure(L, l->func, nup);  /* closure with those upvalues */
        lua_settable(L, -(nup + 3));
    }
    lua_pop(L, nup);  /* remove upvalues */
}

int
luaopen_launchdarkly_server_sdk(lua_State *const l)
{
    luaL_newmetatable(l, "LaunchDarklyClient");
    lua_pushvalue(l, -1);
    lua_setfield(l, -2, "__index");
    ld_luaL_setfuncs(l, launchdarkly_client_methods, 0);

    luaL_newmetatable(l, "LaunchDarklyUser");
    lua_pushvalue(l, -1);
    lua_setfield(l, -2, "__index");
    ld_luaL_setfuncs(l, launchdarkly_user_methods, 0);

    luaL_newmetatable(l, "LaunchDarklyStoreInterface");
    lua_pushvalue(l, -1);
    lua_setfield(l, -2, "__index");
    ld_luaL_setfuncs(l, launchdarkly_store_methods, 0);

    #if LUA_VERSION_NUM == 503 || LUA_VERSION_NUM == 502
        luaL_newlib(l, launchdarkly_functions);
    #else
        luaL_register(l, "launchdarkly-server-sdk", launchdarkly_functions);
    #endif

    return 1;
}
