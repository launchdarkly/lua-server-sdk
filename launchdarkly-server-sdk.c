/***
Server-side SDK for LaunchDarkly.
@module launchdarkly-server-sdk
*/

#include <lua.h>
#include <lauxlib.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include <launchdarkly/server_side/bindings/c/sdk.h>
#include <launchdarkly/server_side/bindings/c/config/builder.h>

#include <launchdarkly/bindings/c/logging/log_level.h>
#include <launchdarkly/bindings/c/context_builder.h>
#include <launchdarkly/bindings/c/array_builder.h>
#include <launchdarkly/bindings/c/object_builder.h>

#define DEBUG 1

#ifdef DEBUG
#define DEBUG_PRINT(fmt, ...) printf(fmt, __VA_ARGS__)
#else
#define DEBUG_PRINT(fmt, ...)
#endif

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

// field_validator is used to validate a single field in a config table.
struct field_validator {
    // Name of the field.
    const char* key;
    // Expected Lua type of the field.
    int type;
    // Function to parse the value on the top of the stack. If NULL, an error will be reported
    // at runtime if the field exists.
    void (*parse) (lua_State *const l, int i, LDServerConfigBuilder builder, void* user_data);
    // Store a pointer to arbitrary data for use in 'parse'.
    void *user_data;
};

// Parses a string and then calls a setter function stored in user_data.
// The setter must have the signature (LDServerConfigBuilder, const char*).
static void parse_string(lua_State *const l, int i, LDServerConfigBuilder builder, void* user_data) {
    const char *const uri = lua_tostring(l, i);
    void (*setter)(LDServerConfigBuilder, const char*) = user_data;
    setter(builder, uri);
}

// Parses a bool and then calls a setter function stored in user_data.
// The setter must have the signature (LDServerConfigBuilder, bool).
static void parse_bool(lua_State *const l, int i, LDServerConfigBuilder builder, void* user_data) {
    const bool value = lua_toboolean(l, i);
    void (*setter)(LDServerConfigBuilder, bool) = user_data;
    setter(builder, value);
}

// Parses a number and then calls a setter function stored in user_data.
// The setter must have the signature (LDServerConfigBuilder, int).
static void parse_number(lua_State *const l, int i, LDServerConfigBuilder builder, void* user_data) {
    const int value = lua_tointeger(l, i);
    void (*setter)(LDServerConfigBuilder, int) = user_data;
    setter(builder, value);
}

// Forward declaration of the config used in traverse_config, to keep parse_table
// in the same place as the others.
struct config;

// Forward declaration for same reason as above. Traverses a config recursively,
// calling the appropriate parse function for each field. It expects a table to be
// on top of the stack.
void traverse_config(lua_State *const l, LDServerConfigBuilder builder, struct config *cfg);

// Parses a table using traverse_config.
static void parse_table(lua_State *const l, int i,  LDServerConfigBuilder builder, void* user_data) {
    // since traverse_config expects the table to be on top of the stack,
    // make it so.
    lua_pushvalue(l, i);
    traverse_config(l, builder, user_data);
}



// Parses an array of strings. Items that aren't strings are silently ignored.
static void parse_string_array(lua_State *const l, int i, LDServerConfigBuilder builder, void* user_data) {
    void (*setter)(LDServerConfigBuilder, const char*) = user_data;
    int n = lua_tablelen(l, i);
    DEBUG_PRINT("parsing string array of length %d\n", n);

    for (int j = 1; j <= n; j++) {
        lua_rawgeti(l, i, j);
        if (lua_isstring(l, -1)) {
            const char* elem = lua_tostring(l, -1);
            DEBUG_PRINT("array[%d] = %s\n", j, elem);
            setter(builder, elem);
        }
        lua_pop(l, 1);
    }
}

// Special purpose parser for grabbing a store interface from a userdata.
static void parse_lazyload_source(lua_State *const l, int i, LDServerConfigBuilder builder, void* user_data) {
// TODO: replace checkudata
    LDServerLazyLoadSourcePtr *source = lua_touserdata(l, i);
    LDServerLazyLoadBuilder lazy_load_builder = LDServerLazyLoadBuilder_New();
    LDServerLazyLoadBuilder_SourcePtr(lazy_load_builder, *source);

    LDServerConfigBuilder_DataSystem_LazyLoad(builder, lazy_load_builder);
}

// Stores a list of fields and the field count. The name is used for error reporting.
struct config {
    const char *name;
    struct field_validator* fields;
    int n;
};

#define ARR_SIZE(x) (sizeof(x) / sizeof(x[0]))

// Use this macro to define new config tables. Config tables can be nested arbitrarily.
#define DEFINE_CONFIG(name, path, fields) \
    struct config name = {path, fields, ARR_SIZE(fields)}


struct field_validator lazyload_fields[] = {
    {"source", LUA_TUSERDATA, parse_lazyload_source, NULL /* not needed */},
    {"cacheRefreshMilliseconds", LUA_TNUMBER, parse_number, LDServerLazyLoadBuilder_CacheRefreshMs},
    {"cacheEvictionPolicy", LUA_TNUMBER, parse_number, LDServerLazyLoadBuilder_CachePolicy}
};

DEFINE_CONFIG(lazyload_config, "dataSystem.lazyLoad", lazyload_fields);

struct field_validator streaming_fields[] = {
    {"initialReconnectDelayMilliseconds", LUA_TNUMBER, parse_number, LDServerDataSourceStreamBuilder_InitialReconnectDelayMs},
};

DEFINE_CONFIG(streaming_config, "dataSystem.backgroundSync.streaming", streaming_fields);

struct field_validator polling_fields[] = {
    {"intervalSeconds", LUA_TNUMBER, parse_number, LDServerDataSourcePollBuilder_IntervalS},
};

DEFINE_CONFIG(polling_config, "dataSystem.backgroundSync.polling", polling_fields);

struct field_validator backgroundsync_fields[] = {
    /* Mutually exclusive */
    {"streaming", LUA_TTABLE, parse_table, &streaming_config},
    {"polling", LUA_TTABLE, parse_table, &polling_config}
};


DEFINE_CONFIG(backgroundsync_config, "dataSystem.backgroundSync", backgroundsync_fields);

struct field_validator datasystem_fields[] = {
    {"enabled", LUA_TBOOLEAN, parse_bool, LDServerConfigBuilder_DataSystem_Enabled}
    //{"backgroundSync", LUA_TTABLE, parse_table, &backgroundsync_config},
   // {"lazyLoad", LUA_TTABLE, parse_table, &lazyload_config}
};

DEFINE_CONFIG(datasystem_config, "dataSystem", datasystem_fields);

struct field_validator event_fields[] = {
    {"enabled", LUA_TBOOLEAN, parse_bool, LDServerConfigBuilder_Events_Enabled},
    {"contextKeysCapacity", LUA_TNUMBER, NULL, NULL},
    {"capacity", LUA_TNUMBER, parse_number, LDServerConfigBuilder_Events_Capacity},
    {"flushIntervalMilliseconds", LUA_TNUMBER, parse_number, LDServerConfigBuilder_Events_FlushIntervalMs},
    {"allAttributesPrivate", LUA_TBOOLEAN, parse_bool, LDServerConfigBuilder_Events_AllAttributesPrivate},
    {"privateAttributes", LUA_TTABLE, parse_string_array, LDServerConfigBuilder_Events_PrivateAttribute}
};

DEFINE_CONFIG(event_config, "events", event_fields);

struct field_validator endpoint_fields[] = {
   {"pollingBaseURL", LUA_TSTRING, parse_string, LDServerConfigBuilder_ServiceEndpoints_PollingBaseURL},
   {"streamingBaseURL", LUA_TSTRING, parse_string, LDServerConfigBuilder_ServiceEndpoints_StreamingBaseURL},
   {"eventsBaseURL", LUA_TSTRING, parse_string, LDServerConfigBuilder_ServiceEndpoints_EventsBaseURL}
};

DEFINE_CONFIG(endpoint_config, "serviceEndpoints", endpoint_fields);

struct field_validator appinfo_fields[] = {
    {"identifier", LUA_TSTRING, parse_string, LDServerConfigBuilder_AppInfo_Identifier},
    {"version", LUA_TSTRING, parse_string, LDServerConfigBuilder_AppInfo_Version}
};

DEFINE_CONFIG(appinfo_config, "appInfo", appinfo_fields);

struct field_validator top_level_fields[] = {
    {"appInfo", LUA_TTABLE, parse_table, &appinfo_config},
    {"serviceEndpoints", LUA_TTABLE, parse_table, &endpoint_config},
    {"offline", LUA_TBOOLEAN, parse_bool, LDServerConfigBuilder_Offline},
    {"dataSystem", LUA_TTABLE, parse_table, &datasystem_config },
    {"events", LUA_TTABLE, parse_table, &event_config }
};

DEFINE_CONFIG(top_level_config, "config", top_level_fields);

// Finds a field by key in the given config, or returns NULL.
struct field_validator * find_field(const char *key, struct config* cfg);

void traverse_config(lua_State *const l, LDServerConfigBuilder builder, struct config *cfg) {
    DEBUG_PRINT("traversing %s\n", cfg->name);
    if (lua_type(l, -1) != LUA_TTABLE) {
        luaL_error(l, "%s must be a table", cfg->name);
    }
    lua_pushnil(l);
    while (lua_next(l, -2) != 0) {
        lua_pushvalue(l, -2);
        const char* key = lua_tostring(l, -1);
        int type = lua_type(l, -2);

        DEBUG_PRINT("inspecting field %s (%s)\n", key, lua_typename(l, type));

        struct field_validator *field = find_field(key, cfg);
        if (field == NULL) {
            luaL_error(l, "unrecognized %s field: %s", cfg->name, key);
        }
        if (field->type != type) {
            luaL_error(l, "%s field %s must be a %s", cfg->name, key, lua_typename(l, field->type));
        }
        if (field->parse != NULL) {
            field->parse(l, -2, builder, field->user_data);
        } else {
            luaL_error(l, "%s missing field parser for %s", cfg->name, key);
        }
        lua_pop(l, 2);
    }
    lua_pop(l, 1);
}

struct field_validator * find_field(const char *key, struct config* cfg) {
    for (int i = 0; i < cfg->n; i++) {
        if (strcmp(cfg->fields[i].key, key) == 0) {
            return &cfg->fields[i];
        }
    }
    return NULL;
}

static LDServerConfig
makeConfig(lua_State *const l)
{
    // We have been passed two arguments:
    // First: the SDK key (string)
    // Second: the config structure (table)

    const char* sdk_key = luaL_checkstring(l, 1);
    LDServerConfigBuilder builder = LDServerConfigBuilder_New(sdk_key);

    // Recursively visit the heirarchical configs, modifying the builder
    // as we go along.
    traverse_config(l, builder, &top_level_config);

    bool logging_callbacks_set =
            globalLogEnabledCallback != LUA_NOREF &&
            globalLogWriteCallback != LUA_NOREF;

    DEBUG_PRINT("logging callbacks set? %s\n", logging_callbacks_set ? "true" : "false");

	if (logging_callbacks_set) {
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
    LDStatus status = LDServerConfigBuilder_Build(builder, &out_config);
    if (!LDStatus_Ok(status)) {
        lua_pushstring(l, LDStatus_Error(status));
        LDStatus_Free(status);
        luaL_error(l, "SDK configuration invalid: %s", lua_tostring(l, -1));
    }
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

    config = makeConfig(l);

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
