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

#ifdef DEBUG
#define DEBUG_PRINT(fmt, ...) printf(fmt, __VA_ARGS__)
#else
#define DEBUG_PRINT(fmt, ...)
#endif

#define SDKVersion "2.1.1" /* {x-release-please-version} */

static LDValue
LuaValueToJSON(lua_State *const l, const int i);

static LDValue
LuaTableToJSON(lua_State *const l, const int i);

static LDValue
LuaArrayToJSON(lua_State *const l, const int i);

static void
LuaPushJSON(lua_State *const l, LDValue j);

static int lua_tablelen(lua_State *L, int index)
{
    #if LUA_VERSION_NUM >= 502
    return lua_rawlen(L, index);
    #else
    return lua_objlen(L, index);
    #endif
}

/**
* The custom log backend struct (LDLogBackend) contains a void* userdata pointer, and two function pointers
* (enabled and write). This allows C users to avoid global logging functions.

* This doesn't quite match up with Lua's expectations, since:
* 1) The user-defined functions will be stored in the Lua registry
* 2) We need to keep around a lua_State pointer for accessing the registry when we need to invoke the callbacks
*
* This struct is therefore the equivalent of the C LDLogBackend struct, but with registry references and a lua_State pointer.
* We allocate a new Lua userdata and then wire up some global callbacks (lua_log_backend_enabled, lua_log_backend_write)
* to cast the void* to a lua_log_backend*, grab the function references, and forward the arguments.
*/
struct lua_log_backend {
    lua_State *l;
	int enabled_ref;
	int write_ref;
};

/**
* Creates a new custom log backend. The functions provided must be thread safe as the SDK
* does not perform any locking.
* @function makeLogBackend
* @tparam function enabled A function that returns true if the specified log level is enabled. The signature
should be (level: string) -> boolean, where the known level strings are 'debug', 'info', 'warn', and 'error'.
* @tparam function write A function that writes a log message at a specified level. The signature should be
* (level: string, message: string) -> void.
* @treturn A new custom log backend, suitable for use in SDK `logging` configuration.
*/
static int LuaLDLogBackendNew(lua_State *l) {

 	if (lua_gettop(l) != 2) {
        return luaL_error(l, "expecting exactly 2 arguments");
    }

	luaL_checktype(l, 1, LUA_TFUNCTION);
	luaL_checktype(l, 2, LUA_TFUNCTION);

	int write_ref = luaL_ref(l, LUA_REGISTRYINDEX);
	int enabled_ref = luaL_ref(l, LUA_REGISTRYINDEX);

    struct lua_log_backend* backend = lua_newuserdata(l, sizeof(struct lua_log_backend));
    luaL_getmetatable(l, "LaunchDarklyLogBackend");
    lua_setmetatable(l, -2);

    backend->l = l;
	backend->write_ref = write_ref;
	backend->enabled_ref = enabled_ref;

    return 1;
}

static struct lua_log_backend* check_log_backend(lua_State *l, int i) {
	void *ud = luaL_checkudata(l, i, "LaunchDarklyLogBackend");
	luaL_argcheck(l, ud != NULL, i, "`LaunchDarklyLogBackend' expected");
	return ud;
}

bool lua_log_backend_enabled(enum LDLogLevel level, void *user_data) {
    struct lua_log_backend* backend = user_data;
    lua_State *l = backend->l;

    lua_rawgeti(l, LUA_REGISTRYINDEX, backend->enabled_ref);

    lua_pushstring(l, LDLogLevel_Name(level, "unknown"));

    lua_call(l, 1, 1); // one argument (level - string), one result (enabled - boolean).

    return lua_toboolean(l, -1);
}

void lua_log_backend_write(enum LDLogLevel level, const char* msg, void *user_data) {
    struct lua_log_backend* backend = user_data;
    lua_State *l = backend->l;

    lua_rawgeti(l, LUA_REGISTRYINDEX, backend->write_ref);

    lua_pushstring(l, LDLogLevel_Name(level, "unknown"));
    lua_pushstring(l, msg);

    lua_call(l, 2, 0); // two args (level - string, msg - string), no results.
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

This function is deprecated and provided as a convenience. Please transition
to using @{makeContext} instead.

Create a new opaque context object of kind 'user'. This method
has changed from the previous Lua SDK v1.x `makeUser`, as users are no longer
supported.

Specifically:
1. 'secondary' attribute is no longer supported.
2. 'privateAttributeNames' is now called 'privateAttributes' and supports
attribute references (similar to JSON pointer syntax, e.g. `/foo/bar`).
3. all fields under 'custom' become top-level context attributes, rather than
being nested under an attribute named 'custom'.

For example, `{ custom = { foo = "bar" } }` would result in a context with attribute 'foo' equal to 'bar'.

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
@tparam[opt] table fields.privateAttributes A list of attributes to
redact
@tparam[opt] table fields.custom A table of additional context attributes.
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

    lua_getfield(l, 1, "custom");

    // The individual fields of custom are added to the top-level of the context.
    if (lua_istable(l, -1)) {
        lua_pushnil(l);

        while (lua_next(l, -2) != 0) {
            const char *const key = luaL_checkstring(l, -2);

            LDValue value = LuaValueToJSON(l, -1);

            LDContextBuilder_Attributes_Set(builder, "user", key, value);

            lua_pop(l, 1);
        }
    }

    lua_getfield(l, 1, "privateAttributes");

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


static void parse_private_attrs_or_cleanup(lua_State *const l, LDContextBuilder builder, const char* kind);
static void parse_attrs_or_cleanup(lua_State *const l, LDContextBuilder builder, const char* kind);
static bool field_is_type_or_cleanup(lua_State* const l, int actual_field_type, int expected_field_type, LDContextBuilder builder, const char* field_name, const char* kind);

/**

Create a new opaque context object. This method can be used to create single
or multi-kind contexts. The context's kind must always be specified, even if
it is a user.

For example, to create a context with a single user kind:
```
local context = ld.makeContext({
    user = {
        key = "alice-123",
        name = "alice",
        attributes = {
            age = 52,
            contact = {
                email = "alice@mail.com",
                phone = "555-555-5555"
            }
        },
        privateAttributes = { "age", "/contact/phone" }
    }
})
```

A multi-kind context can be useful when targeting based on multiple kinds of data.
For example, to associate a device context with a user:

```
local context = ld.makeContext({
    user = {
        key = "alice-123",
        name = "alice",
        anonymous = true
    },
    device {
        key = "device-123",
        attributes = {
           manufacturer = "bigcorp"
        }
    }
})
```

SDK methods will automatically check for context validity. You may check manually
by calling @{valid} to detect errors earlier.

@function makeContext
@tparam table A table of context kinds, where the table keys are the kind names
and the values are tables containing context's information.
@tparam string [kind.key] The context's key, which is required.
@tparam[opt] [kind.name] A name for the context. This is useful for identifying the context
in the LaunchDarkly dashboard.
@tparam[opt] [kind.anonymous] A boolean indicating whether the context should be marked as anonymous.
@tparam[opt] [kind.attributes] A table of arbitrary attributes to associate with the context.
@tparam[opt] [kind.privateAttributes] An array of attribute references, indicating which
attributes should be marked private. Attribute references may be simple attribute names
(like 'age'), or may use a JSON-pointer-like syntax (like '/contact/phone').
@treturn A fresh context.
*/
static int
LuaLDContextNew(lua_State *const l) {

    // The single argument is a table containing key/value pairs
    // which represent kind/contexts. There is no implicit way of constructing
    // a user context - you need to have a 'user = { .. }'.

    if (lua_gettop(l) != 1) {
        return luaL_error(l, "expecting exactly 1 argument");
    }

    luaL_checktype(l, 1, LUA_TTABLE);

    LDContextBuilder builder = LDContextBuilder_New();

    lua_pushnil(l);
    while (lua_next(l, -2) != 0) {
        lua_pushvalue(l, -2);


        int kind_type = lua_type(l, -1);
        int context_type = lua_type(l, -2);

        if (kind_type != LUA_TSTRING) {
            LDContextBuilder_Free(builder);
            luaL_error(l, "top-level table keys must be context kinds; example: user = { ... }");
        }

        if (context_type != LUA_TTABLE) {
            LDContextBuilder_Free(builder);
            luaL_error(l, "top-level table values must be tables; example: user = { ... }");
        }

        const char* kind = lua_tostring(l, -1);

        DEBUG_PRINT("inspecting %s context\n", kind);

        // The context table is on the top of the stack. It must contain a key.
        lua_getfield(l, -2, "key");
        const char *const key = lua_tostring(l, -1);
        lua_pop(l, 1);

        if (key == NULL) {
            LDContextBuilder_Free(builder);
            luaL_error(l, "context kind %s: must contain key", kind);
        }

        LDContextBuilder_AddKind(builder, kind, key);

        lua_getfield(l, -2, "attributes");
        if (field_is_type_or_cleanup(l, lua_type(l, -1), LUA_TTABLE, builder, "attributes", kind)) {
            parse_attrs_or_cleanup(l, builder, kind);
        }
        lua_pop(l, 1);


        lua_getfield(l, -2, "privateAttributes");
        if (field_is_type_or_cleanup(l, lua_type(l, -1), LUA_TTABLE, builder, "privateAttributes", kind)) {
            parse_private_attrs_or_cleanup(l, builder, kind);
        }
        lua_pop(l, 1);

        lua_getfield(l, -2, "name");
        if (field_is_type_or_cleanup(l, lua_type(l, -1), LUA_TSTRING, builder, "name", kind)) {
            LDContextBuilder_Attributes_SetName(builder, kind, lua_tostring(l, -1));
        }
        lua_pop(l, 1);

        lua_getfield(l, -2, "anonymous");
        if (field_is_type_or_cleanup(l, lua_type(l, -1), LUA_TBOOLEAN, builder, "anonymous", kind)) {
            LDContextBuilder_Attributes_SetAnonymous(builder, kind, lua_toboolean(l, -1));
        }
        lua_pop(l, 1);


        lua_pop(l, 2);
    }

    lua_pop(l, 1);


    LDContext context = LDContextBuilder_Build(builder);

    LDContext *u = (LDContext *)lua_newuserdata(l, sizeof(context));

    *u = context;

    luaL_getmetatable(l, "LaunchDarklyContext");
    lua_setmetatable(l, -2);
    return 1;
}


static bool field_is_type_or_cleanup(lua_State* const l, int actual_field_type, int expected_field_type, LDContextBuilder builder, const char* field_name, const char* kind) {
    if (actual_field_type == expected_field_type) {
        DEBUG_PRINT("field %s for %s context is a %s\n", field_name, kind, lua_typename(l, actual_field_type));
        return true;
    } else if (actual_field_type == LUA_TNIL) {
        DEBUG_PRINT("no %s for %s context\n", field_name, kind);
    } else {
        LDContextBuilder_Free(builder);
        luaL_error(l, "context kind %s: %s must be a %s", kind, field_name, lua_typename(l, expected_field_type));
    }
    return false;
}

static void parse_attrs_or_cleanup(lua_State *const l, LDContextBuilder builder, const char* kind) {
    lua_pushnil(l);
    while (lua_next(l, -2) != 0) {
        lua_pushvalue(l, -2);

        int key_type = lua_type(l, -1);
        if (key_type != LUA_TSTRING) {
            LDContextBuilder_Free(builder);
            luaL_error(l, "context kind %s: top-level attribute keys must be strings", kind);
        }

        const char* key = lua_tostring(l, -1);

        DEBUG_PRINT("context kind %s: parsing attribute %s\n", kind, key);

        LDValue value = LuaValueToJSON(l, -2);

        LDContextBuilder_Attributes_Set(builder, kind, key, value);

        lua_pop(l, 2);
    }
}


static void parse_private_attrs_or_cleanup(lua_State *const l, LDContextBuilder builder, const char* kind) {
    int n = lua_tablelen(l, -1);
    for (int i = 1; i <= n; i++) {
        lua_rawgeti(l, -1, i);

        if (lua_isstring(l, -1)) {
            LDContextBuilder_Attributes_AddPrivateAttribute(builder, kind, luaL_checkstring(l, -1));
        } else {
            LDContextBuilder_Free(builder);
            luaL_error(l, "context kind %s: privateAttributes must be a table of strings", kind);
        }

        lua_pop(l, 1);
    }
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
Frees a context object.
*/
static int
LuaLDContextFree(lua_State *const l)
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
// The field delegates to a parse function, which handles extracting the actual
// type.
struct field_validator {
    // Name of the field used in Lua.
    const char* key;

    // Expected Lua type of the field.
    int type;

    // Function that parses the value at stack index i.
    //
    // The function must agree on the type of the argument 'setter', which is
    // the value of this struct's 'setter' field.
    //
    // For example, parse might handle parsing bools. So the actual
    // signature for 'setter' might be:
    // void (*setter)(LDServerConfigBuilder, bool)
    //
    // Which would allow the implementation of parse to call setter(builder, value).
    void (*parse) (lua_State *const l, int i, void* builder, void* setter);

    // Stores a function that is capable of setting a value on an arbitrary builder.
    // The type of the value being stored is erased here so that field_validator
    // can handle all necessary types.
    void *setter;
};

#define FIELD(key, type, parse, user_data) {key, type, parse, user_data}

// Parses a string.
// The setter must have the signature (void*, const char*).
static void parse_string(lua_State *const l, int i, void* builder, void* setter) {
    const char *const value = lua_tostring(l, i);
    DEBUG_PRINT("string = %s\n", value ? value : "NULL");
    void (*string_setter)(void*, const char*) = setter;
    string_setter(builder, value);
}

static void parse_log_level(lua_State *const l, int i, void* builder, void* setter) {
    const char *const value = lua_tostring(l, i);
    DEBUG_PRINT("log level = %s\n", value ? value : "NULL");
    void (*log_level_setter)(void*, enum LDLogLevel) = setter;

	// Issue an error if the log level isn't known. Another option would be
	// to silently default to a known level, such as LD_LOG_INFO.
	const int unknown_level_sentinel = 255;
	enum LDLogLevel level = LDLogLevel_Enum(value, 255);
	if (level == unknown_level_sentinel) {
		luaL_error(l, "unknown log level: '%s' (known options include 'debug', 'info', 'warn', and 'error')", value);
	}
    log_level_setter(builder, level);
}

// Parses a bool.
// The setter must have the signature (void*, bool).
static void parse_bool(lua_State *const l, int i, void* builder, void* setter) {
    const bool value = lua_toboolean(l, i);
    DEBUG_PRINT("bool = %s\n", value ? "true" : "false");
    void (*bool_setter)(void*, bool) = setter;
    bool_setter(builder, value);
}

// Parses a number.
// The setter must have the signature (void*, unsigned int).
static void parse_unsigned(lua_State *const l, int i, void* builder, void* setter) {
    const int value = lua_tointeger(l, i);
    DEBUG_PRINT("number = %d\n", value);
    if (value < 0) {
        luaL_error(l, "got %d, expected positive int", value);
    }
    if (setter) {
        void (*unsigned_int_setter)(void*, unsigned int) = setter;
        unsigned_int_setter(builder, value);
    }
}

// Forward declaration of the config used in traverse_config, to keep parse_table
// in the same place as the others.
struct config;

// Forward declaration for same reason as above. Traverses a config recursively,
// calling the appropriate parse function for each field. It expects a table to be
// on top of the stack.
void traverse_config(lua_State *const l, LDServerConfigBuilder builder, struct config *cfg);

// Parses a table using traverse_config. Can only be invoked on top level LDServerConfigBuilder
// configurations, not child builders.
static void parse_table(lua_State *const l, int i, void* builder, void* user_data) {
    // since traverse_config expects the table to be on top of the stack,
    // make it so.
    lua_pushvalue(l, i);
    traverse_config(l, (LDServerConfigBuilder) builder, user_data);
}

// Parses an array of strings. Items that aren't strings will trigger an error.
// The setter must have the signature (void*, const char*).
static void parse_string_array(lua_State *const l, int i, void* builder, void* setter) {
    void (*string_setter)(void*, const char*) = setter;
    int n = lua_tablelen(l, i);
    DEBUG_PRINT("parsing string array of length %d\n", n);

    for (int j = 1; j <= n; j++) {
        lua_rawgeti(l, i, j);
        if (lua_isstring(l, -1)) {
            const char* elem = lua_tostring(l, -1);
            DEBUG_PRINT("array[%d] = %s\n", j, elem);
            string_setter(builder, elem);
        } else {
            luaL_error(l, "array[%d] is not a string", j);
        }
        lua_pop(l, 1);
    }
}

// Special purpose parser for extracting a LaunchDarklySourceInterface from
// the stack. Setter must have the signature (void*, void*).
static void parse_lazyload_source(lua_State *const l, int i, void* builder, void* setter) {
    LDServerLazyLoadSourcePtr *source = luaL_checkudata(l, i, "LaunchDarklySourceInterface");
    DEBUG_PRINT("source = %p\n", *source);
    void (*source_setter)(void*, void*) = setter;

    // Dereferencing source because lua_touserdata returns a pointer (to our pointer).
    source_setter(builder, *source);
}


static void parse_log_backend(lua_State *const l, int i, void* builder, void* setter) {
	struct LDLogBackend backend;
	LDLogBackend_Init(&backend);

	struct lua_log_backend* lua_backend = check_log_backend(l, i);
	backend.UserData = lua_backend;
	backend.Enabled = lua_log_backend_enabled;
	backend.Write = lua_log_backend_write;

	LDLoggingCustomBuilder custom_logging = LDLoggingCustomBuilder_New();
	LDLoggingCustomBuilder_Backend(custom_logging, backend);

   	void (*backend_setter)(void*, LDLoggingCustomBuilder) = setter;
    backend_setter(builder, custom_logging);
}

// Function that returns a new child builder. This is used to allocate builders
// which are necessary for building a child config. This is needed when the C++ SDK's
// API requires a builder to be allocated and passed in to the top-level builder, e.g.
// the streaming or polling config builders.
typedef void* (*new_child_builder_fn)(void);

// Function that consumes the builder created by a new_child_builder_fn. This passes
// ownership of the builder back to the top-level configuration builder.
typedef void (*consume_child_builder_fn)(LDServerConfigBuilder, void*);

// Represents a logical chunk of config with a list of fields.
// Individual fields might themselves be configs.
//
// If a child config requires its own builder, then new_builder and consume_builder must be set.
// In this case, before any fields are parsed, new_builder will be invoked
// and stored in child_builder. After all fields are parsed, consume_builder will be invoked
// to transfer ownership of the child to the parent top-level config.
struct config {
    // Name of the config, used for errors / logging.
    const char *name;

    // List of fields and length.
    struct field_validator* fields;
    int n;

    // Null at compile-time; stores the result of new_builder at runtime (if set.)
    void *child_builder;

    // Assign both if this config needs a child builder.
    new_child_builder_fn new_child_builder;
    consume_child_builder_fn consume_child_builder;
};

#define ARR_SIZE(x) (sizeof(x) / sizeof(x[0]))

// Use this macro to define new config tables.
#define DEFINE_CONFIG(name, path, fields) \
    struct config name = {path, fields, ARR_SIZE(fields), NULL, NULL, NULL}

// Use this macro to define a config table which requires a child builder.
#define DEFINE_CHILD_CONFIG(name, path, fields, new_builder, consume_builder) \
    struct config name = {path, fields, ARR_SIZE(fields), NULL, (new_child_builder_fn) new_builder, (consume_child_builder_fn) consume_builder}

// Invokes a field's parse method, varying the builder argument depending on if this
// is a top-level or child config.
void config_invoke_parse(struct config *cfg, struct field_validator *field, LDServerConfigBuilder builder, lua_State *const l) {
    if (cfg->child_builder) {
        DEBUG_PRINT("invoking parser for %s with child builder (%p)\n", field->key, cfg->child_builder);
        field->parse(l, -2, cfg->child_builder, field->setter);
    } else {
        DEBUG_PRINT("invoking parser for %s with top-level builder\n", field->key);
        field->parse(l, -2, builder, field->setter);
    }
}

struct field_validator lazyload_fields[] = {
    FIELD("source", LUA_TUSERDATA, parse_lazyload_source, LDServerLazyLoadBuilder_SourcePtr),
    FIELD("cacheRefreshMilliseconds", LUA_TNUMBER, parse_unsigned, LDServerLazyLoadBuilder_CacheRefreshMs),
    FIELD("cacheEvictionPolicy", LUA_TNUMBER, parse_unsigned, LDServerLazyLoadBuilder_CachePolicy)
};

DEFINE_CHILD_CONFIG(lazyload_config,
    "dataSystem.lazyLoad",
    lazyload_fields,
    LDServerLazyLoadBuilder_New,
    LDServerConfigBuilder_DataSystem_LazyLoad
);

struct field_validator streaming_fields[] = {
    FIELD("initialReconnectDelayMilliseconds", LUA_TNUMBER, parse_unsigned, LDServerDataSourceStreamBuilder_InitialReconnectDelayMs)
};

DEFINE_CHILD_CONFIG(streaming_config,
    "dataSystem.backgroundSync.streaming",
    streaming_fields,
    LDServerDataSourceStreamBuilder_New,
    LDServerConfigBuilder_DataSystem_BackgroundSync_Streaming
);

struct field_validator polling_fields[] = {
    FIELD("intervalSeconds", LUA_TNUMBER, parse_unsigned, LDServerDataSourcePollBuilder_IntervalS)
};

DEFINE_CHILD_CONFIG(polling_config,
    "dataSystem.backgroundSync.polling",
    polling_fields,
    LDServerDataSourcePollBuilder_New,
    LDServerConfigBuilder_DataSystem_BackgroundSync_Polling
);

struct field_validator backgroundsync_fields[] = {
    /* Mutually exclusive */
    FIELD("streaming", LUA_TTABLE, parse_table, &streaming_config),
    FIELD("polling", LUA_TTABLE, parse_table, &polling_config)
};


DEFINE_CONFIG(backgroundsync_config, "dataSystem.backgroundSync", backgroundsync_fields);

struct field_validator datasystem_fields[] = {
    FIELD("enabled", LUA_TBOOLEAN, parse_bool, LDServerConfigBuilder_DataSystem_Enabled),
    FIELD("backgroundSync", LUA_TTABLE, parse_table, &backgroundsync_config),
    FIELD("lazyLoad", LUA_TTABLE, parse_table, &lazyload_config)
};

DEFINE_CONFIG(datasystem_config, "dataSystem", datasystem_fields);

struct field_validator event_fields[] = {
    FIELD("enabled", LUA_TBOOLEAN, parse_bool, LDServerConfigBuilder_Events_Enabled),
    FIELD("contextKeysCapacity", LUA_TNUMBER, parse_unsigned, LDServerConfigBuilder_Events_ContextKeysCapacity),
    FIELD("capacity", LUA_TNUMBER, parse_unsigned, LDServerConfigBuilder_Events_Capacity),
    FIELD("flushIntervalMilliseconds", LUA_TNUMBER, parse_unsigned, LDServerConfigBuilder_Events_FlushIntervalMs),
    FIELD("allAttributesPrivate", LUA_TBOOLEAN, parse_bool, LDServerConfigBuilder_Events_AllAttributesPrivate),
    FIELD("privateAttributes", LUA_TTABLE, parse_string_array, LDServerConfigBuilder_Events_PrivateAttribute)
};

DEFINE_CONFIG(event_config, "events", event_fields);

struct field_validator endpoint_fields[] = {
   FIELD("pollingBaseURL", LUA_TSTRING, parse_string, LDServerConfigBuilder_ServiceEndpoints_PollingBaseURL),
   FIELD("streamingBaseURL", LUA_TSTRING, parse_string, LDServerConfigBuilder_ServiceEndpoints_StreamingBaseURL),
   FIELD("eventsBaseURL", LUA_TSTRING, parse_string, LDServerConfigBuilder_ServiceEndpoints_EventsBaseURL)
};

DEFINE_CONFIG(endpoint_config, "serviceEndpoints", endpoint_fields);

struct field_validator appinfo_fields[] = {
    FIELD("identifier", LUA_TSTRING, parse_string, LDServerConfigBuilder_AppInfo_Identifier),
    FIELD("version", LUA_TSTRING, parse_string, LDServerConfigBuilder_AppInfo_Version)
};

DEFINE_CONFIG(appinfo_config, "appInfo", appinfo_fields);

struct field_validator basic_logging_fields[] = {
	FIELD("level", LUA_TSTRING, parse_log_level, LDLoggingBasicBuilder_Level),
	FIELD("tag", LUA_TSTRING, parse_string, LDLoggingBasicBuilder_Tag)
};

DEFINE_CHILD_CONFIG(
	basic_logging_config,
	"logging.basic",
	basic_logging_fields,
	LDLoggingBasicBuilder_New,
	LDServerConfigBuilder_Logging_Basic
);


struct field_validator logging_fields[] = {
	/* These are mutually exclusive */
	FIELD("basic", LUA_TTABLE, parse_table, &basic_logging_config),
	FIELD("custom", LUA_TUSERDATA, parse_log_backend, LDServerConfigBuilder_Logging_Custom)
};

DEFINE_CONFIG(logging_config, "logging", logging_fields);

struct field_validator top_level_fields[] = {
    FIELD("appInfo", LUA_TTABLE, parse_table, &appinfo_config),
    FIELD("serviceEndpoints", LUA_TTABLE, parse_table, &endpoint_config),
    FIELD("offline", LUA_TBOOLEAN, parse_bool, LDServerConfigBuilder_Offline),
    FIELD("dataSystem", LUA_TTABLE, parse_table, &datasystem_config),
    FIELD("events", LUA_TTABLE, parse_table, &event_config),
    FIELD("logging", LUA_TTABLE, parse_table, &logging_config)
};

DEFINE_CONFIG(top_level_config, "config", top_level_fields);

// Finds a field by key in the given config, or returns NULL.
struct field_validator * find_field(const char *key, struct config* cfg);

void traverse_config(lua_State *const l, LDServerConfigBuilder builder, struct config *cfg) {
    DEBUG_PRINT("traversing %s\n", cfg->name);
    if (lua_type(l, -1) != LUA_TTABLE) {
        luaL_error(l, "%s must be a table", cfg->name);
    }

    if (cfg->new_child_builder != NULL) {
        cfg->child_builder = cfg->new_child_builder();
        DEBUG_PRINT("created child builder (%p) for %s\n", cfg->child_builder, cfg->name);
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
        if (field->parse == NULL) {
            luaL_error(l, "%s missing field parser for %s", cfg->name, key);
        } else {
            config_invoke_parse(cfg, field, builder, l);
        }

        lua_pop(l, 2);
    }

    if (cfg->child_builder != NULL) {
        DEBUG_PRINT("invoking child builder consumer (%p) on child builder (%p)\n", cfg->consume_child_builder, cfg->child_builder);
        cfg->consume_child_builder(builder, cfg->child_builder);
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
makeConfig(lua_State *const l, const char *const sdk_key)
{
    LDServerConfigBuilder builder = LDServerConfigBuilder_New(sdk_key);


    // Allow users to pass in a nil config, which is equivalent to passing an
    // empty table (i.e. default configuration.)
    if (lua_type(l, -1) != LUA_TNIL) {
        // Recursively visit the hierarchical configs, modifying the builder
        // as we go along.
        traverse_config(l, builder, &top_level_config);
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
Initialize a new client, and connect to LaunchDarkly.
Applications should instantiate a single instance for the lifetime of their application.

To initialize with default configuration, you may pass nil or an empty table as the third argument.

@function clientInit
@param string Environment SDK key
@param int Initialization timeout in milliseconds. clientInit will
block for this long before returning a non-fully initialized client. Pass 0 to return
immediately without waiting (note that the client will continue initializing in the background.)
@tparam table config optional configuration options, or nil/empty table for default configuration.
@tparam[opt] boolean config.offline Sets whether this client is offline.
An offline client will not make any network connections to LaunchDarkly or
a data source like Redis, nor send any events, and will return application-defined
default values for all feature flags.
@tparam[opt] table config.logging Options related to the SDK's logging facilities. The `basic` and `custom` configs
are mutually exclusive. If you only need to change logging verbosity or the log tag, use `basic`. If you need to
replace the log verbosity logic & printing entirely, use `custom`.
@tparam[opt] table config.logging.basic Modify the SDK's default logger.
@tparam[opt] string config.logging.basic.tag A tag to include in log messages, for example 'launchdarkly'.
@tparam[opt] string config.logging.basic.level The minimum level of log messages to include. Known options include
'debug', 'info', 'warn', or 'error'.
@tparam[opt] userdata config.logging.custom A custom log backend, created with @{makeLogBackend}.
@tparam[opt] table config.serviceEndpoints If you set one custom service endpoint URL,
you must set all of them. You probably don't need to set this unless instructed by
LaunchDarkly.
@tparam[opt] string config.serviceEndpoints.streamingBaseURL Set the streaming URL
for connecting to LaunchDarkly.
@tparam[opt] string config.serviceEndpoints.eventsBaseURL Set the events URL for
connecting to LaunchDarkly.
@tparam[opt] string config.serviceEndpoints.pollingBaseURL Set the polling URL for
connecting to LaunchDarkly.
@tparam[opt] table config.events Options related to event generation and
delivery.
@tparam[opt] bool config.events.enabled Sets whether to send analytics events
back to LaunchDarkly. By default, the client will send events. This differs
from top-level config option 'offline' in that it only affects sending events,
not receiving data.
@tparam[opt] int config.events.capacity The capacity of the events buffer.
The client buffers up to this many events in memory before flushing. If the
capacity is exceeded before the buffer is flushed, events will be discarded.
@tparam[opt] int config.events.flushIntervalMilliseconds The time between flushes of the event
buffer. Decreasing the flush interval means that the event buffer is less
likely to reach capacity.
@tparam[opt] boolean config.events.allAttributesPrivate Sets whether or not all context
attributes (other than the key) should be hidden from LaunchDarkly. If this
is true, all context attribute values will be private, not just the attributes
specified via events.privateAttributes.
@tparam[opt] int config.events.contextKeysCapacity The number of context keys that the
event processor can remember at an one time, so that duplicate context details
will not be sent in analytics.
@tparam[opt] table config.events.privateAttributes Marks a set of context attribute
as private. Any contexts sent to LaunchDarkly with this configuration active
will have attributes refered to removed.
@tparam[opt] table config.appInfo Specify metadata related to your application.
@tparam[opt] string config.appInfo.identifier An identifier for the application.
@tparam[opt] string config.appInfo.version The version of the application.
@tparam[opt] table config.dataSystem Change configuration of the default streaming
data source, switch to a polling source, or specify a read-only database source.
@tparam[opt] bool config.dataSystem.enabled Set to false to disable receiving any data
from any LaunchDarkly data source (streaming or polling) or a database source (like Redis.)
@tparam[opt] table config.dataSystem.backgroundSync Change streaming or polling
configuration. The SDK uses streaming by default. Note that streaming and polling are mutually exclusive.
@tparam[opt] int config.dataSystem.backgroundSync.streaming.initialReconnectDelayMilliseconds
The time to wait before the first reconnection attempt, if the streaming connection is dropped.
@tparam[opt] int config.dataSystem.backgroundSync.polling.intervalSeconds The time between individual
polling requests.
@tparam[opt] int config.dataSystem.lazyLoad.cacheRefreshMilliseconds How long a data item (flag/segment)
remains cached in memory before requiring a refresh from the source.
@tparam[opt] userdata config.dataSystem.lazyLoad.source A custom data source. Currently
only Redis is supported.
@return A fresh client.
*/
static int
LuaLDClientInit(lua_State *const l)
{

    if (lua_gettop(l) != 3) {
        return luaL_error(l, "expecting exactly 3 arguments");
    }

    const char *const sdk_key = luaL_checkstring(l, 1);

    const int timeout = luaL_checkinteger(l, 2);

    LDServerConfig config = makeConfig(l, sdk_key);

    LDServerSDK client = LDServerSDK_New(config);

    LDServerSDK_Start(client, timeout, NULL);

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
Returns true if the context is valid.

@class function
@name valid
@tparam context context An opaque context object from @{makeUser} or @{makeContext}
@treturn True if valid, otherwise false.
*/
static int
LuaLDContextValid(lua_State *const l)
{

    LDContext *context = luaL_checkudata(l, 1, "LaunchDarklyContext");

    lua_pushboolean(l, LDContext_Valid(*context));

    return 1;
}

/**
Returns an error string if the context is invalid.

@class function
@name errors
@tparam context context An opaque context object from @{makeUser} or @{makeContext}
@treturn Error string if valid, otherwise nil.
*/
static int
LuaLDContextErrors(lua_State *const l)
{
    if (lua_gettop(l) != 1) {
        return luaL_error(l, "expecting exactly 1 argument");
    }

    LDContext *context = luaL_checkudata(l, 1, "LaunchDarklyContext");

    const char* error = LDContext_Errors(*context);

    if (error && strlen(error) > 0) {
        lua_pushstring(l, error);
    } else {
        lua_pushnil(l);
    }

    return 1;
}

/**
Returns the canonical key of the context.

@class function
@name canonicalKey
@tparam context context An opaque context object from @{makeUser} or @{makeContext}
@treturn Canonical key of the context, or nil if the context isn't valid.
*/
static int
LuaLDContextCanonicalKey(lua_State *const l)
{
    if (lua_gettop(l) != 1) {
        return luaL_error(l, "expecting exactly 1 argument");
    }

    LDContext *context = luaL_checkudata(l, 1, "LaunchDarklyContext");

    const char* key = LDContext_CanonicalKey(*context);

    if (key && strlen(key) > 0) {
        lua_pushstring(l, key);
    } else {
        lua_pushnil(l);
    }

    return 1;
}

/**
Returns the value of a context attribute.

@class function
@name getAttribute
@tparam context context An opaque context object from @{makeUser} or @{makeContext}
@tparam string kind The kind of the context.
@tparam string attribute_reference An attribute reference naming the attribute to get.
@treturn Lua value of the attribute, or nil if the attribute isn't present in the context.
*/
static int
LuaLDContextGetAttribute(lua_State *const l)
{
    if (lua_gettop(l) != 3) {
        return luaL_error(l, "expecting exactly 3 arguments");
    }

    LDContext *context = luaL_checkudata(l, 1, "LaunchDarklyContext");
    const char *const kind = luaL_checkstring(l, 2);
    const char *const attribute_reference = luaL_checkstring(l, 3);

    LDValue val = LDContext_Get(*context, kind, attribute_reference);

    if (val == NULL) {
        lua_pushnil(l);
        return 1;
    }

    LuaPushJSON(l, val);

    /* Don't need to free the value as it's owned by the context. */
    return 1;
}

/**
Returns the private attribute references associated with a particular
context kind.

@class function
@name privateAttributes
@tparam context context An opaque context object from @{makeUser} or @{makeContext}
@tparam string kind The kind of the context.
@treturn Array of private attribute references, or nil if the kind isn't present in
the context.
*/
static int
LuaLDContextPrivateAttributes(lua_State *const l)
{
    if (lua_gettop(l) != 2) {
        return luaL_error(l, "expecting exactly 2 arguments");
    }

    LDContext *context = luaL_checkudata(l, 1, "LaunchDarklyContext");

    const char* kind = luaL_checkstring(l, 2);

    LDContext_PrivateAttributesIter iter = LDContext_PrivateAttributesIter_New(*context, kind);
    if (iter == NULL) {
        lua_pushnil(l);
        return 1;
    }

    lua_newtable(l);
    int count = 1;
    while (!LDContext_PrivateAttributesIter_End(iter)) {
        const char* attr = LDContext_PrivateAttributesIter_Value(iter);
        lua_pushstring(l, attr);
        lua_rawseti(l, -2, count);
        LDContext_PrivateAttributesIter_Next(iter);
        count++;
    }

    LDContext_PrivateAttributesIter_Free(iter);

    return 1;
}

/**
EvaluationDetail contains extra information related to evaluation of a flag.

@field kind string, one of: "OFF","FALLTHROUGH", "TARGET_MATCH", "RULE_MATCH",
"PREREQUISITE_FAILED", "ERROR", "UNKNOWN".
@field errorKind string, only present if 'kind' is "ERROR"; one of: "CLIENT_NOT_READY",
"FLAG_NOT_FOUND", "USER_NOT_SPECIFIED", "MALFORMED_FLAG", "WRONG_TYPE",
"MALFORMED_FLAG", "EXCEPTION", "UNKNOWN".
@field inExperiment bool, whether the flag was part of an experiment.
@field variationIndex int, only present if the evaluation was successful. The zero-based index of the variation.
@field value LDJSON, the value of the flag.
@table EvaluationDetail
*/

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
@return @{EvaluationDetail}
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
@return @{EvaluationDetail}
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
@return @{EvaluationDetail}
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
@return @{EvaluationDetail}
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
@return @{EvaluationDetail}
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
Reports that a context has performed an event. Custom data, and a metric
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
Generates an identify event for a context.
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
Returns a map from feature flag keys to values for a given context.
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

	LDValue owned_map = LDAllFlagsState_Map(state);

	LuaPushJSON(l, owned_map);

	LDValue_Free(owned_map);
    LDAllFlagsState_Free(state);

    return 1;
}

static const struct luaL_Reg launchdarkly_functions[] = {
    { "clientInit",     LuaLDClientInit     },
    { "makeUser",       LuaLDUserNew        },
    { "makeContext",    LuaLDContextNew     },
    { "version",        LuaLDVersion        },
	{ "makeLogBackend", LuaLDLogBackendNew  },
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

static const struct luaL_Reg launchdarkly_context_methods[] = {
    { "valid",  LuaLDContextValid  },
    { "errors", LuaLDContextErrors },
    { "canonicalKey", LuaLDContextCanonicalKey },
    { "privateAttributes", LuaLDContextPrivateAttributes },
    { "getAttribute", LuaLDContextGetAttribute },
    { "__gc", LuaLDContextFree },
    { NULL,   NULL          }
};

static const struct luaL_Reg launchdarkly_source_methods[] = {
    { NULL, NULL }
};

static const struct luaL_Reg launchdarkly_log_backend_methods[] = {
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
    ld_luaL_setfuncs(l, launchdarkly_context_methods, 0);

    luaL_newmetatable(l, "LaunchDarklySourceInterface");
    lua_pushvalue(l, -1);
    lua_setfield(l, -2, "__index");
    ld_luaL_setfuncs(l, launchdarkly_source_methods, 0);

	luaL_newmetatable(l, "LaunchDarklyLogBackend");
    lua_pushvalue(l, -1);
    lua_setfield(l, -2, "__index");
    ld_luaL_setfuncs(l, launchdarkly_log_backend_methods, 0);

    #if LUA_VERSION_NUM >= 502
        luaL_newlib(l, launchdarkly_functions);
    #else
        luaL_register(l, "launchdarkly-server-sdk", launchdarkly_functions);
    #endif

    return 1;
}
