/***
Server-side SDK for LaunchDarkly.
@module launchdarkly-server-sdk
*/

#include <lua.h>
#include <lauxlib.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <launchdarkly/server_side/bindings/c/sdk.h>
#include <launchdarkly/server_side/bindings/c/config/builder.h>

#include <launchdarkly/bindings/c/logging/log_level.h>
#include <launchdarkly/bindings/c/context_builder.h>
#include <launchdarkly/bindings/c/array_builder.h>
#include <launchdarkly/bindings/c/object_builder.h>



#define SDKVersion "1.2.2" /* {x-release-please-version} */

static LDValue
LuaValueToJSON(lua_State *const l, const int i);

static LDValue
LuaTableToJSON(lua_State *const l, const int i);

static LDValue
LuaArrayToJSON(lua_State *const l, const int i);

static void
LuaPushJSON(lua_State *const l, LDValue j);

static int globalLogEnabledCallback;
static int globalLogWriteCallback;

static lua_State *globalLuaState;

static int lua_tablelen(lua_State *L, int index)
{
    #if LUA_VERSION_NUM >= 502
    return lua_rawlen(L, index);
    #else
    return lua_objlen(L, index);
    #endif
}


bool logEnabled(enum LDLogLevel level, void *user_data /* ignored */) {
	lua_rawgeti(globalLuaState, LUA_REGISTRYINDEX, globalLogEnabledCallback);

    lua_pushstring(globalLuaState, LDLogLevel_Name(level, "unknown"));

    lua_call(globalLuaState, 1, 1); // one argument (level, string), one result (enable, boolean).

	return lua_toboolean(globalLuaState, -1);
}

void logWrite(enum LDLogLevel level, const char* msg, void *user_data /* ignored */) {
	lua_rawgeti(globalLuaState, LUA_REGISTRYINDEX, globalLogWriteCallback);

    lua_pushstring(globalLuaState, LDLogLevel_Name(level, "unknown"));
    lua_pushstring(globalLuaState, msg);

    lua_call(globalLuaState, 2, 0); // two args (level, string + msg, string), no results.
}


/***
TODO: Document that the log levels have changed and there is an additional callback, and removed the log level arg.
Set the global logger for all SDK operations. This function is not thread
safe, and if used should be done so before other operations.
@function registerLogger
@tparam function writeCb The logging write handler. Callback must be of the form
"function (logLevel, logLine) ... end".
@tparam function enabledCb The log level enabled handler. Callback must be of the form
"function (logLevel) -> bool ... end". Return true if the given log level is enabled.
*/
static int
LuaLDRegisterLogger(lua_State *const l)
{
    if (lua_gettop(l) != 2) {
        return luaL_error(l, "expecting exactly 2 arguments");
    }

    globalLuaState        = l;
	globalLogEnabledCallback = luaL_ref(l, LUA_REGISTRYINDEX);
    globalLogWriteCallback = luaL_ref(l, LUA_REGISTRYINDEX);

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

static LDValue
LuaValueToJSON(lua_State *const l, const int i)
{
    LDValue result = NULL;

    switch (lua_type(l, i)) {
        case LUA_TBOOLEAN:
            result = LDValue_NewBool(lua_toboolean(l, i));
            break;
        case LUA_TNUMBER:
            result = LDValue_NewNumber(lua_tonumber(l, i));
            break;
        case LUA_TSTRING:
            result = LDValue_NewString(lua_tostring(l, i));
            break;
        case LUA_TTABLE:
            if (isArray(l, i)) {
                result = LuaArrayToJSON(l, i);
            } else {
                result = LuaTableToJSON(l, i);
            }
            break;
        default:
            result = LDValue_NewNull();
            break;
    }

    return result;
}

static LDValue
LuaArrayToJSON(lua_State *const l, const int i)
{
    LDArrayBuilder result = LDArrayBuilder_New();

    lua_pushvalue(l, i);

    lua_pushnil(l);

    while (lua_next(l, -2) != 0) {
        LDValue value = LuaValueToJSON(l, -1);

        LDArrayBuilder_Add(result, value);

        lua_pop(l, 1);
    }

    lua_pop(l, 1);

    return LDArrayBuilder_Build(result);
}

static LDValue
LuaTableToJSON(lua_State *const l, const int i)
{
    LDObjectBuilder result = LDObjectBuilder_New();

    lua_pushvalue(l, i);

    lua_pushnil(l);

    while (lua_next(l, -2) != 0) {
        const char *const key      = lua_tostring(l, -2);
        LDValue value = LuaValueToJSON(l, -1);

        LDObjectBuilder_Add(result, key, value);

        lua_pop(l, 1);
    }

    lua_pop(l, 1);

    return LDObjectBuilder_Build(result);
}

static void
LuaPushJSONObject(lua_State *const l, LDValue j)
{
    LDValue_ObjectIter iter;

    lua_newtable(l);

    for (iter = LDValue_ObjectIter_New(j); !LDValue_ObjectIter_End(iter); LDValue_ObjectIter_Next(iter)) {
        LuaPushJSON(l, LDValue_ObjectIter_Value(iter));
        lua_setfield(l, -2, LDValue_ObjectIter_Key(iter));
    }
}

static void
LuaPushJSONArray(lua_State *const l, LDValue j)
{
    LDValue_ArrayIter iter;

    lua_newtable(l);

    int index = 1;

    for (iter = LDValue_ArrayIter_New(j); !LDValue_ArrayIter_End(iter); LDValue_ArrayIter_Next(iter)) {
        LuaPushJSON(l, LDValue_ArrayIter_Value(iter));
        lua_rawseti(l, -2, index);
        index++;
    }
}

static void
LuaPushJSON(lua_State *const l, LDValue j)
{
    switch (LDValue_Type(j)) {
        case LDValueType_String:
            lua_pushstring(l, LDValue_GetString(j));
            break;
        case LDValueType_Bool:
            lua_pushboolean(l, LDValue_GetBool(j));
            break;
        case LDValueType_Number:
            lua_pushnumber(l, LDValue_GetNumber(j));
            break;
        case LDValueType_Object:
            LuaPushJSONObject(l, j);
            break;
        case LDValueType_Array:
            LuaPushJSONArray(l, j);
            break;
        default:
            lua_pushnil(l);
            break;
    }

    return;
}

/***
Create a new opaque context object of kind 'user'.
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
@tparam[opt] table fields.privateAttributeNames A list of attributes to
redact
@tparam[opt] table fields.custom A table of attributes to set on the user.
@return an opaque context object
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

    LDContextBuilder builder = LDContextBuilder_New();

    LDContextBuilder_AddKind(builder, "user", key);

    lua_getfield(l, 1, "anonymous");

    if (lua_isboolean(l, -1)) {
        LDContextBuilder_Attributes_SetAnonymous(builder, "user", true);
    }

    lua_getfield(l, 1, "ip");

    if (lua_isstring(l, -1)) {
        LDContextBuilder_Attributes_Set(builder, "user", "ip", LDValue_NewString(luaL_checkstring(l, -1)));
    };

    lua_getfield(l, 1, "firstName");

    if (lua_isstring(l, -1)) {
        LDContextBuilder_Attributes_Set(builder, "user", "firstName", LDValue_NewString(luaL_checkstring(l, -1)));
    }

    lua_getfield(l, 1, "lastName");

    if (lua_isstring(l, -1)) {
        LDContextBuilder_Attributes_Set(builder, "user", "lastName", LDValue_NewString(luaL_checkstring(l, -1)));
    }

    lua_getfield(l, 1, "email");

    if (lua_isstring(l, -1)) {
        LDContextBuilder_Attributes_Set(builder, "user", "email", LDValue_NewString(luaL_checkstring(l, -1)));
    }

    lua_getfield(l, 1, "name");

    if (lua_isstring(l, -1)) {
        LDContextBuilder_Attributes_SetName(builder, "user", luaL_checkstring(l, -1));
    }

    lua_getfield(l, 1, "avatar");

    if (lua_isstring(l, -1)) {
        LDContextBuilder_Attributes_Set(builder, "user", "avatar", LDValue_NewString(luaL_checkstring(l, -1)));
    }

    lua_getfield(l, 1, "country");

    if (lua_isstring(l, -1)) {
        LDContextBuilder_Attributes_Set(builder, "user", "country", LDValue_NewString(luaL_checkstring(l, -1)));
    }



    // TODO: Document that secondary was removed


    lua_getfield(l, 1, "custom");

    // The individual fields of custom are added to the top-level of the context.
    // TODO: document this.
    if (lua_istable(l, -1)) {
        lua_pushnil(l);

        while (lua_next(l, -2) != 0) {
            const char *const key = luaL_checkstring(l, -2);

            LDValue value = LuaValueToJSON(l, -1);

            LDContextBuilder_Attributes_Set(builder, "user", key, value);

            lua_pop(l, 1);
        }
    }

    lua_getfield(l, 1, "privateAttributeNames");

    if (lua_istable(l, -1)) {
        int n = lua_tablelen(l, -1);

        for (int i = 1; i <= n; i++) {
            lua_rawgeti(l, -1, i);

            if (lua_isstring(l, -1)) {
                LDContextBuilder_Attributes_AddPrivateAttribute(builder, "user", luaL_checkstring(l, -1));
            }

            lua_pop(l, 1);
        }
    }

    LDContext context = LDContextBuilder_Build(builder);

    LDContext *u = (LDContext *)lua_newuserdata(l, sizeof(context));

    *u = context;

    luaL_getmetatable(l, "LaunchDarklyContext");
    lua_setmetatable(l, -2);

    return 1;
}


/***
Return SDK version.
@function version
@return SDK version string.
*/
static int
LuaLDVersion(lua_State *const l)
{
    lua_pushstring(l, SDKVersion);
    return 1;
}

/**
Frees a user object.
@deprecated Users are deprecated. Use contexts instead.
*/
static int
LuaLDUserFree(lua_State *const l)
{
    LDContext *context;

    context = (LDContext *)luaL_checkudata(l, 1, "LaunchDarklyContext");

    if (*context) {
        LDContext_Free(*context);
        *context = NULL;
    }

    return 0;
}

// makeConfig({
//     dataSystem: {
//         enabled: bool
//         method: streaming
//
//
//     }
// })



static LDServerConfig
makeConfig(lua_State *const l, const int i)
{
    LDServerConfigBuilder builder;

    luaL_checktype(l, i, LUA_TTABLE);

    lua_getfield(l, i, "key");

    const char *const key = luaL_checkstring(l, -1);

    builder = LDServerConfigBuilder_New(key);

    lua_getfield(l, i, "baseURI");

    if (lua_isstring(l, -1)) {
        LDServerConfigBuilder_ServiceEndpoints_PollingBaseURL(builder, luaL_checkstring(l, -1));
    }

    lua_getfield(l, i, "streamURI");

    if (lua_isstring(l, -1)) {
        LDServerConfigBuilder_ServiceEndpoints_StreamingBaseURL(builder, luaL_checkstring(l, -1));
    }

    lua_getfield(l, i, "eventsURI");

    if (lua_isstring(l, -1)) {
        LDServerConfigBuilder_ServiceEndpoints_EventsBaseURL(builder, luaL_checkstring(l, -1));
    }

/*
    local user = l.makeUser({
        key = "alice",
        dataSystem = {
            enabled = true,
            backgroundSync = {
                source = "launchdarkly_streaming",
                initialReconnectDelayMs = 1000
            }
lazyLoad = {
        source = makeRedisSource("redis://localhost:6379", "prefix")
}
        }
    })
*/

// TODO: stream, useLDD, and pollInterval, and featureStoreBackend all unified under new dataSystem key
    lua_getfield(l, i, "dataSystem");

    if (lua_istable(l, -1)) {
        lua_getfield(l, i, "enabled");
        if (lua_isboolean(l, -1)) {
            LDServerConfigBuilder_DataSystem_Enabled(builder, lua_toboolean(l, -1));
        }

        lua_getfield(l, i, "backgroundSync");
        if (lua_isnil(l, -1)) {
            lua_pop(l, 1); /* not an error, perhaps lazyLoad was configured instead */

            lua_getfield(l, i, "lazyLoad");
            if (lua_isnil(l, -1)) {
                return luaL_error(l, "dataSystem.lazyLoad specified, must specify a source");
            } else if (lua_istable(l, -1)) {
                lua_getfield(l, i, "source");

                if (lua_isnil(l, -1)) {
                    return luaL_error(l, "dataSystem.lazyLoad.source must be provided");
                }

                LDServerLazyLoadSource *source = luaL_checkudata(l, 1, "LaunchDarklyStoreInterface");

                LDServerLazyLoadBuilder lazy_load_builder = LDServerLazyLoadBuilder_New();

                LDServerLazyLoadBuilder_Source(lazy_load_builder, source);
                LDServerLazyLoadBuilder_CacheRefreshMs(lazy_load_builder, 30000);
                LDServerLazyLoadBuilder_CacheEviction(lazy_load_builder, LDServerLazyLoadCacheEviction_Disabled);


                   /* This should be a metatable containing the redis source */

            } else {
                return luaL_error(l, "dataSystem.lazyLoad must be a table");
            }
        } else if (lua_istable(l, -1)) {
            lua_getfield(l, i, "source");

            if (lua_isstring(l, -1)) {
                const char *const source = luaL_checkstring(l, -1);
                if (strcmp(source, "launchdarkly_streaming") == 0) {
                    LDServerDataSourceStreamBuilder stream_builder = LDServerDataSourceStreamBuilder_New();

                    lua_getfield(l, i, "initialReconnectDelayMs");
                    if (lua_isnumber(l, -1)) {
                        LDServerDataSourceStreamBuilder_InitialReconnectDelayMs(stream_builder, luaL_checkinteger(l, -1));
                    }

                    LDServerConfigBuilder_DataSystem_BackgroundSync_Streaming(builder, stream_builder);
                } else if (strcmp(source, "launchdarkly_polling") == 0) {
                    LDServerDataSourcePollBuilder poll_builder = LDServerDataSourcePollBuilder_New();

                    lua_getfield(l, i, "intervalSeconds");
                    if (lua_isnumber(l, -1)) {
                        LDServerDataSourcePollBuilder_IntervalS(poll_builder, luaL_checkinteger(l, -1));
                    }

                    LDServerConfigBuilder_DataSystem_BackgroundSync_Polling(builder, poll_builder);
                } else {
                    return luaL_error(l, "dataSystem.method must be 'streaming' or 'polling'");
                }
            } else {
                return luaL_error(l, "dataSystem.backgroundSync.source must be a string");
            }
        } else {
            return luaL_error(l, "dataSystem.backgroundSync must be a table");
        }
    }

    lua_getfield(l, i, "sendEvents");

    if (lua_isboolean(l, -1)) {
        LDServerConfigBuilder_Events_Enabled(builder, lua_toboolean(l, -1));
    }

    lua_getfield(l, i, "eventsCapacity");

    if (lua_isnumber(l, -1)) {
        LDServerConfigBuilder_Events_Capacity(builder, luaL_checkinteger(l, -1));
    }

    lua_getfield(l, i, "timeout");

    if (lua_isnumber(l, -1)) {
        // TODO: Use timeout
    }

    lua_getfield(l, i, "flushInterval");

    if (lua_isnumber(l, -1)) {
        LDServerConfigBuilder_Events_FlushIntervalMs(builder, luaL_checkinteger(l, -1));
    }

    lua_getfield(l, i, "pollInterval");

    if (lua_isnumber(l, -1)) {
        // TODO: Set poll data system
    }

    lua_getfield(l, i, "offline");

    if (lua_isboolean(l, -1)) {
        // TODO: Document change in behavior (offline now disables events + data system)
        LDServerConfigBuilder_Offline(builder, lua_toboolean(l, -1));
    }

    lua_getfield(l, i, "useLDD");

    if (lua_isboolean(l, -1)) {
        // TODO: Warn that this needs to be setup
    }

    lua_getfield(l, i, "inlineUsersInEvents");

    if (lua_isboolean(l, -1)) {
        // TODO: warn this was removed
    }

    lua_getfield(l, i, "allAttributesPrivate");

    if (lua_isboolean(l, -1)) {
       LDServerConfigBuilder_Events_AllAttributesPrivate(builder, lua_toboolean(l, -1));
    }

    lua_getfield(l, i, "userKeysCapacity");

    if (lua_isnumber(l, -1)) {
        // TODO: We don't have a C binding for this yet
        //LDConfigSetUserKeysCapacity(config, luaL_checkinteger(l, -1));
    }

    lua_getfield(l, i, "featureStoreBackend");

    if (lua_isuserdata(l, -1)) {
        // TODO: Setup the store reader backend
        // struct LDStoreInterface **storeInterface;
        //
        // storeInterface = (struct LDStoreInterface **)
        //     luaL_checkudata(l, -1, "LaunchDarklyStoreInterface");
        //
        // LDConfigSetFeatureStoreBackend(config, *storeInterface);
    }

    lua_getfield(l, 1, "privateAttributeNames");

    if (lua_istable(l, -1)) {
        int n = lua_tablelen(l, -1);

        for (int i = 1; i <= n; i++) {
            lua_rawgeti(l, -1, i);

            if (lua_isstring(l, -1)) {
                LDServerConfigBuilder_Events_PrivateAttribute(builder, luaL_checkstring(l, -1));
            }

            lua_pop(l, 1);
        }
    }


	if (globalLogEnabledCallback != LUA_NOREF && globalLogWriteCallback != LUA_NOREF) {
		struct LDLogBackend backend;
		LDLogBackend_Init(&backend);

		backend.Write = logWrite;
		backend.Enabled = logEnabled;

	    LDLoggingCustomBuilder custom_logging = LDLoggingCustomBuilder_New();
		LDLoggingCustomBuilder_Backend(custom_logging, backend);
		LDServerConfigBuilder_Logging_Custom(builder, custom_logging);
	}

    LDServerConfigBuilder_HttpProperties_WrapperName(builder, "lua-server-sdk");
    LDServerConfigBuilder_HttpProperties_WrapperVersion(builder, SDKVersion);


    LDServerConfig out_config;
    LDServerConfigBuilder_Build(builder, &out_config);

    // TODO: Check result of the call
    return out_config;
}

/***
Initialize a new client, and connect to LaunchDarkly. Applications should instantiate a single instance for the lifetime of their application.
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
    LDServerSDK client;
    LDServerConfig config;
    unsigned int timeout;

    if (lua_gettop(l) != 2) {
        return luaL_error(l, "expecting exactly 2 arguments");
    }

    config = makeConfig(l, 1);

    // TODO: use timeout
    timeout = luaL_checkinteger(l, 2);

    client = LDServerSDK_New(config);

    LDServerSDK *c = (LDServerSDK *) lua_newuserdata(l, sizeof(client));

    *c = client;

    luaL_getmetatable(l, "LaunchDarklyClient");
    lua_setmetatable(l, -2);

    return 1;
}

static int
LuaLDClientClose(lua_State *const l)
{
    LDServerSDK *client;

    client = (LDServerSDK *)luaL_checkudata(l, 1, "LaunchDarklyClient");

    if (client) {
        LDServerSDK_Free(*client);
        *client = NULL;
    }

    return 0;
}

static void LuaPushReason(lua_State *const l, LDEvalReason reason) {
	enum LDEvalReason_Kind reason_kind = LDEvalReason_Kind(reason);

	// Push a string representation opf each reason
	switch (reason_kind) {
		case LD_EVALREASON_OFF:
            lua_pushstring(l, "OFF");
            break;
		case LD_EVALREASON_FALLTHROUGH:
            lua_pushstring(l, "FALLTHROUGH");
            break;
		case LD_EVALREASON_TARGET_MATCH:
			lua_pushstring(l, "TARGET_MATCH");
            break;
		case LD_EVALREASON_RULE_MATCH:
            lua_pushstring(l, "RULE_MATCH");
			break;
		case LD_EVALREASON_PREREQUISITE_FAILED:
            lua_pushstring(l, "PREREQUISITE_FAILED");
		    break;
		case LD_EVALREASON_ERROR:
            lua_pushstring(l, "ERROR");
	        break;
		default:
			lua_pushstring(l, "UNKNOWN");
            break;
	}

	lua_setfield(l, -2, "kind");

	enum LDEvalReason_ErrorKind out_error_kind;
	if (LDEvalReason_ErrorKind(reason, &out_error_kind)) {

		// Switch on out_error_kind, and push the string representation.
		switch (out_error_kind) {
			case LD_EVALREASON_ERROR_CLIENT_NOT_READY:
                lua_pushstring(l, "CLIENT_NOT_READY");
                break;
			case LD_EVALREASON_ERROR_USER_NOT_SPECIFIED:
			    lua_pushstring(l, "USER_NOT_SPECIFIED");
                break;
			case LD_EVALREASON_ERROR_FLAG_NOT_FOUND:
				lua_pushstring(l, "FLAG_NOT_FOUND");
                break;
			case LD_EVALREASON_ERROR_WRONG_TYPE:
				lua_pushstring(l, "WRONG_TYPE");
                break;
			case LD_EVALREASON_ERROR_MALFORMED_FLAG:
                lua_pushstring(l, "MALFORMED_FLAG");
			    break;
			case LD_EVALREASON_ERROR_EXCEPTION:
                lua_pushstring(l, "EXCEPTION");
			    break;
			default:
                lua_pushstring(l, "UNKNOWN");
				break;
		}

		lua_setfield(l, -2, "errorKind");
	}

	bool in_experiment = LDEvalReason_InExperiment(reason);
	lua_pushboolean(l, in_experiment);
	lua_setfield(l, -2, "inExperiment");
}

static void
LuaPushDetails(lua_State *const l, LDEvalDetail details,
    LDValue value)
{
    lua_newtable(l);
    /** TODO: C bindings for this? */


	LDEvalReason out_reason;
	if (LDEvalDetail_Reason(details, &out_reason)) {
		lua_newtable(l);
		LuaPushReason(l, out_reason);
		lua_setfield(l, -2, "reason");
	}

    size_t out_variation_index;
    if (LDEvalDetail_VariationIndex(details, &out_variation_index)) {
        lua_pushnumber(l, out_variation_index);
        lua_setfield(l, -2, "variationIndex");
    }

    LuaPushJSON(l, value);
    lua_setfield(l, -2, "value");

    LDEvalDetail_Free(details);
    LDValue_Free(value);
}

/**
Evaluate a boolean flag
@class function
@name boolVariation
@tparam context context An opaque context object from @{makeUser} or @{makeContext}
@tparam string key The key of the flag to evaluate.
@tparam boolean fallback The value to return on error
@treturn boolean The evaluation result, or the fallback value
*/
static int
LuaLDClientBoolVariation(lua_State *const l)
{
    LDServerSDK *client;
    LDContext *context;

    if (lua_gettop(l) != 4) {
        return luaL_error(l, "expecting exactly 4 arguments");
    }

    client = (LDServerSDK *)luaL_checkudata(l, 1, "LaunchDarklyClient");

    context = (LDContext *)luaL_checkudata(l, 2, "LaunchDarklyContext");

    const char *const key = luaL_checkstring(l, 3);

    const int fallback = lua_toboolean(l, 4);

    const bool result =
        LDServerSDK_BoolVariation(*client, *context, key, fallback);

    lua_pushboolean(l, result);

    return 1;
}

/***
Evaluate a boolean flag and return an explanation
@class function
@name boolVariationDetail
@tparam context context An opaque context object from @{makeUser} or @{makeContext}
@tparam string key The key of the flag to evaluate.
@tparam boolean fallback The value to return on error
@treturn table The evaluation explanation
*/
static int
LuaLDClientBoolVariationDetail(lua_State *const l)
{
    if (lua_gettop(l) != 4) {
        return luaL_error(l, "expecting exactly 4 arguments");
    }

    LDServerSDK *client = (LDServerSDK *)luaL_checkudata(l, 1, "LaunchDarklyClient");

    LDContext *context = (LDContext *) luaL_checkudata(l, 2, "LaunchDarklyContext");

    const char *const key = luaL_checkstring(l, 3);

    const int fallback = lua_toboolean(l, 4);

    LDEvalDetail details;
    const bool result =
        LDServerSDK_BoolVariationDetail(*client, *context, key, fallback, &details);

    LuaPushDetails(l, details, LDValue_NewBool(result));

    return 1;
}

/***
Evaluate an integer flag
@class function
@name intVariation
@tparam context context An opaque context object from @{makeUser} or @{makeContext}
@tparam string key The key of the flag to evaluate.
@tparam int fallback The value to return on error
@treturn int The evaluation result, or the fallback value
*/
static int
LuaLDClientIntVariation(lua_State *const l)
{
    if (lua_gettop(l) != 4) {
        return luaL_error(l, "expecting exactly 4 arguments");
    }

    LDServerSDK *client = (LDServerSDK *) luaL_checkudata(l, 1, "LaunchDarklyClient");

    LDContext *context = (LDContext *) luaL_checkudata(l, 2, "LaunchDarklyContext");

    const char *const key = luaL_checkstring(l, 3);

    const int fallback = luaL_checkinteger(l, 4);

    const int result = LDServerSDK_IntVariation(*client, *context, key, fallback);

    lua_pushnumber(l, result);

    return 1;
}

/***
Evaluate an integer flag and return an explanation
@class function
@name intVariationDetail
@tparam context context An opaque context object from @{makeUser} or @{makeContext}
@tparam string key The key of the flag to evaluate.
@tparam int fallback The value to return on error
@treturn table The evaluation explanation
*/
static int
LuaLDClientIntVariationDetail(lua_State *const l)
{
    if (lua_gettop(l) != 4) {
        return luaL_error(l, "expecting exactly 4 arguments");
    }

    LDServerSDK *client = (LDServerSDK *) luaL_checkudata(l, 1, "LaunchDarklyClient");

    LDContext *context = (LDContext *) luaL_checkudata(l, 2, "LaunchDarklyContext");

    const char *const key = luaL_checkstring(l, 3);

    const int fallback = luaL_checkinteger(l, 4);

    LDEvalDetail details;
    const int result = LDServerSDK_IntVariationDetail(*client, *context, key, fallback, &details);

    LuaPushDetails(l, details, LDValue_NewNumber(result));

    return 1;
}

/***
Evaluate a double flag
@class function
@name doubleVariation
@tparam context context An opaque context object from @{makeUser} or @{makeContext}
@tparam string key The key of the flag to evaluate.
@tparam number fallback The value to return on error
@treturn double The evaluation result, or the fallback value
*/
static int
LuaLDClientDoubleVariation(lua_State *const l)
{
    if (lua_gettop(l) != 4) {
        return luaL_error(l, "expecting exactly 4 arguments");
    }

    LDServerSDK *client = (LDServerSDK *) luaL_checkudata(l, 1, "LaunchDarklyClient");

    LDContext *context = (LDContext *) luaL_checkudata(l, 2, "LaunchDarklyContext");

    const char *const key = luaL_checkstring(l, 3);

    const double fallback = lua_tonumber(l, 4);

    const double result =
        LDServerSDK_DoubleVariation(*client, *context, key, fallback);

    lua_pushnumber(l, result);

    return 1;
}

/***
Evaluate a double flag and return an explanation
@class function
@name doubleVariationDetail
@tparam context context An opaque context object from @{makeUser} or @{makeContext}
@tparam string key The key of the flag to evaluate.
@tparam number fallback The value to return on error
@treturn table The evaluation explanation
*/
static int
LuaLDClientDoubleVariationDetail(lua_State *const l)
{
    if (lua_gettop(l) != 4) {
        return luaL_error(l, "expecting exactly 4 arguments");
    }

    LDServerSDK *client = (LDServerSDK *) luaL_checkudata(l, 1, "LaunchDarklyClient");

    LDContext *context = (LDContext *) luaL_checkudata(l, 2, "LaunchDarklyContext");

    const char *const key = luaL_checkstring(l, 3);

    const double fallback = lua_tonumber(l, 4);

    LDEvalDetail details;
    const double result =
        LDServerSDK_DoubleVariationDetail(*client, *context, key, fallback, &details);

    LuaPushDetails(l, details, LDValue_NewNumber(result));

    return 1;
}

/***
Evaluate a string flag
@class function
@name stringVariation
@tparam context context An opaque context object from @{makeUser} or @{makeContext}
@tparam string key The key of the flag to evaluate.
@tparam string fallback The value to return on error
@treturn string The evaluation result, or the fallback value
*/
static int
LuaLDClientStringVariation(lua_State *const l)
{
    if (lua_gettop(l) != 4) {
        return luaL_error(l, "expecting exactly 4 arguments");
    }

    LDServerSDK *client = (LDServerSDK *)luaL_checkudata(l, 1, "LaunchDarklyClient");

    LDContext *context = (LDContext *) luaL_checkudata(l, 2, "LaunchDarklyContext");

    const char *const key = luaL_checkstring(l, 3);

    const char *const fallback = luaL_checkstring(l, 4);

    char *result =
        LDServerSDK_StringVariation(*client, *context, key, fallback);

    lua_pushstring(l, result);

    LDMemory_FreeString(result);

    return 1;
}

/***
Evaluate a string flag and return an explanation
@class function
@name stringVariationDetail
@tparam context context An opaque context object from @{makeUser} or @{makeContext}
@tparam string key The key of the flag to evaluate.
@tparam string fallback The value to return on error
@treturn table The evaluation explanation
*/
static int
LuaLDClientStringVariationDetail(lua_State *const l)
{

    if (lua_gettop(l) != 4) {
        return luaL_error(l, "expecting exactly 4 arguments");
    }

    LDServerSDK *client = (LDServerSDK *)luaL_checkudata(l, 1, "LaunchDarklyClient");

    LDContext *context = (LDContext *) luaL_checkudata(l, 2, "LaunchDarklyContext");

    const char *const key = luaL_checkstring(l, 3);

    const char *const fallback = luaL_checkstring(l, 4);

    LDEvalDetail details;
    char *result =
        LDServerSDK_StringVariationDetail(*client, *context, key, fallback, &details);

    LuaPushDetails(l, details, LDValue_NewString(result));

    LDMemory_FreeString(result);

    return 1;
}

/***
Evaluate a json flag
@class function
@name jsonVariation
@tparam context context An opaque context object from @{makeUser} or @{makeContext}
@tparam string key The key of the flag to evaluate.
@tparam table fallback The value to return on error
@treturn table The evaluation result, or the fallback value
*/
static int
LuaLDClientJSONVariation(lua_State *const l)
{
    if (lua_gettop(l) != 4) {
        return luaL_error(l, "expecting exactly 4 arguments");
    }

    LDServerSDK *client = (LDServerSDK *) luaL_checkudata(l, 1, "LaunchDarklyClient");

    LDContext *context = (LDContext *) luaL_checkudata(l, 2, "LaunchDarklyContext");

    const char *const key = luaL_checkstring(l, 3);

    LDValue fallback = LuaValueToJSON(l, 4);

    LDValue result = LDServerSDK_JsonVariation(*client, *context, key, fallback);

    LuaPushJSON(l, result);

    LDValue_Free(fallback);
    LDValue_Free(result);

    return 1;
}

/***
Evaluate a json flag and return an explanation
@class function
@name jsonVariationDetail
@tparam context context An opaque context object from @{makeUser} or @{makeContext}
@tparam string key The key of the flag to evaluate.
@tparam table fallback The value to return on error
@treturn table The evaluation explanation
*/
static int
LuaLDClientJSONVariationDetail(lua_State *const l)
{
    if (lua_gettop(l) != 4) {
        return luaL_error(l, "expecting exactly 4 arguments");
    }

    LDServerSDK *client = (LDServerSDK *) luaL_checkudata(l, 1, "LaunchDarklyClient");

    LDContext *context = (LDContext *) luaL_checkudata(l, 2, "LaunchDarklyContext");

    const char *const key = luaL_checkstring(l, 3);

    LDValue fallback = LuaValueToJSON(l, 4);

    LDEvalDetail details;
    LDValue result = LDServerSDK_JsonVariationDetail(*client, *context, key, fallback, &details);

    LuaPushDetails(l, details, result);

    LDValue_Free(fallback);

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
    if (lua_gettop(l) != 1) {
        return luaL_error(l, "expecting exactly 1 argument");
    }

    LDServerSDK *client = (LDServerSDK *) luaL_checkudata(l, 1, "LaunchDarklyClient");

    LDServerSDK_Flush(*client, LD_NONBLOCKING);

    return 0;
}

/***
Reports that a user has performed an event. Custom data, and a metric
can be attached to the event as JSON.
@function track
@tparam string key The name of the event
@tparam context context An opaque context object from @{makeUser} or @{makeContext}
@tparam[opt] table data A value to be associated with an event
@tparam[optchain] number metric A value to be associated with an event
@treturn nil
*/
static int
LuaLDClientTrack(lua_State *const l)
{
    if (lua_gettop(l) < 3 || lua_gettop(l) > 5) {
        return luaL_error(l, "expecting 3-5 arguments");
    }

    LDServerSDK *client = (LDServerSDK *) luaL_checkudata(l, 1, "LaunchDarklyClient");

    const char *const key = luaL_checkstring(l, 2);

    LDContext *context = (LDContext *) luaL_checkudata(l, 3, "LaunchDarklyContext");

    LDValue value;
    if (lua_isnil(l, 4)) {
        value = NULL;
    } else {
        value = LuaValueToJSON(l, 4);
    }

    if (lua_gettop(l) == 5 && lua_isnumber(l, 5)) {
        const double metric = luaL_checknumber(l, 5);

        LDServerSDK_TrackMetric(*client, *context, key, metric, value);
    } else {
        LDServerSDK_TrackData(*client, *context, key, value);
    }

    return 0;
}

// TODO: Document alias was removed, use multi context instead

/***
Check if a client has been fully initialized. This may be useful if the
initialization timeout was reached.
@function isInitialized
@treturn boolean true if fully initialized
*/
static int
LuaLDClientIsInitialized(lua_State *const l)
{
    if (lua_gettop(l) != 1) {
        return luaL_error(l, "expecting exactly 1 argument");
    }

    LDServerSDK *client = (LDServerSDK *) luaL_checkudata(l, 1, "LaunchDarklyClient");

    lua_pushboolean(l, LDServerSDK_Initialized(*client));

    return 1;
}

/***
Generates an identify event for a user.
@function identify
@tparam context context An opaque context object from @{makeUser} or @{makeContext}
@treturn nil
*/
static int
LuaLDClientIdentify(lua_State *const l)
{
    if (lua_gettop(l) != 2) {
        return luaL_error(l, "expecting exactly 2 arguments");
    }

    LDServerSDK *client = (LDServerSDK *) luaL_checkudata(l, 1, "LaunchDarklyClient");
    LDContext *context = (LDContext *) luaL_checkudata(l, 2, "LaunchDarklyContext");

    LDServerSDK_Identify(*client, *context);

    return 0;
}

/***
Returns a map from feature flag keys to values for a given user.
This does not send analytics events back to LaunchDarkly.
@function allFlags
@tparam context context An opaque context object from @{makeUser} or @{makeContext}
@treturn table
*/
static int
LuaLDClientAllFlags(lua_State *const l)
{
    if (lua_gettop(l) != 2) {
        return luaL_error(l, "expecting exactly 2 arguments");
    }

    LDServerSDK *client = (LDServerSDK *) luaL_checkudata(l, 1, "LaunchDarklyClient");

    LDContext *context = (LDContext *) luaL_checkudata(l, 2, "LaunchDarklyContext");

    LDAllFlagsState state = LDServerSDK_AllFlagsState(*client, *context, LD_ALLFLAGSSTATE_DEFAULT);

    char* serialized = LDAllFlagsState_SerializeJSON(state);

    /** TODO: Need to add a C binding to expose this as an LDValue, or have an iterator specific to it. **/
    LDMemory_FreeString(serialized);

    LDAllFlagsState_Free(state);

    /* For now, return an empty table. */
    lua_newtable(l);

    return 1;
}

static const struct luaL_Reg launchdarkly_functions[] = {
    { "clientInit",     LuaLDClientInit     },
    { "makeUser",       LuaLDUserNew        },
    { "registerLogger", LuaLDRegisterLogger },
    { "version",        LuaLDVersion        },
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

    luaL_newmetatable(l, "LaunchDarklyContext");
    lua_pushvalue(l, -1);
    lua_setfield(l, -2, "__index");
    ld_luaL_setfuncs(l, launchdarkly_user_methods, 0);

    luaL_newmetatable(l, "LaunchDarklyStoreInterface");
    lua_pushvalue(l, -1);
    lua_setfield(l, -2, "__index");
    ld_luaL_setfuncs(l, launchdarkly_store_methods, 0);

    #if LUA_VERSION_NUM >= 502
        luaL_newlib(l, launchdarkly_functions);
    #else
        luaL_register(l, "launchdarkly-server-sdk", launchdarkly_functions);
    #endif

    return 1;
}
