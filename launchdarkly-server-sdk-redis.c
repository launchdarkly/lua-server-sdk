/***
Server-side SDK for LaunchDarkly Redis store.
@module launchdarkly-server-sdk-redis
*/

#include <lua.h>
#include <lauxlib.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <launchdarkly/api.h>
#include <launchdarkly/store/redis.h>

/***
Initialize a store backend
@function makeStore
@tparam table fields list of configuration options
@tparam[opt] string fields.host Hostname for Redis.
@tparam[opt] int fields.port Port for Redis.
@tparam[opt] string fields.prefix Redis key prefix for SDK values.
@tparam[opt] int fields.poolSize Number of Redis connections to maintain.
@return A fresh Redis store backend.
*/
static int
LuaLDRedisMakeStore(lua_State *const l)
{
    struct LDRedisConfig *config;
    struct LDStoreInterface *storeInterface;

    if (lua_gettop(l) != 1) {
        return luaL_error(l, "expecting exactly 1 argument");
    }

    luaL_checktype(l, 1, LUA_TTABLE);

    config = LDRedisConfigNew();

    lua_getfield(l, 1, "host");

    if (lua_isstring(l, -1)) {
        LDRedisConfigSetHost(config, luaL_checkstring(l, -1));
    }

    lua_getfield(l, 1, "prefix");

    if (lua_isstring(l, -1)) {
        LDRedisConfigSetPrefix(config, luaL_checkstring(l, -1));
    }

    lua_getfield(l, 1, "port");

    if (lua_isnumber(l, -1)) {
        LDRedisConfigSetPort(config, luaL_checkinteger(l, -1));
    }

    lua_getfield(l, 1, "poolSize");

    if (lua_isnumber(l, -1)) {
        LDRedisConfigSetPoolSize(config, luaL_checkinteger(l, -1));
    }

    storeInterface = LDStoreInterfaceRedisNew(config);

    struct LDStoreInterface **i =
        (struct LDStoreInterface **)lua_newuserdata(l, sizeof(storeInterface));

    *i = storeInterface;

    luaL_getmetatable(l, "LaunchDarklyStoreInterface");
    lua_setmetatable(l, -2);

    return 1;
}

static const struct luaL_Reg launchdarkly_functions[] = {
    { "makeStore", LuaLDRedisMakeStore },
    { NULL,        NULL                }
};

#if !defined LUA_VERSION_NUM || LUA_VERSION_NUM==501
/*
** Adapted from Lua 5.2.0
*/
static void luaL_setfuncs (lua_State *L, const luaL_Reg *l, int nup) {
  luaL_checkstack(L, nup+1, "too many upvalues");
  for (; l->name != NULL; l++) {  /* fill the table with given functions */
    int i;
    lua_pushstring(L, l->name);
    for (i = 0; i < nup; i++)  /* copy upvalues to the top */
      lua_pushvalue(L, -(nup+1));
    lua_pushcclosure(L, l->func, nup);  /* closure with those upvalues */
    lua_settable(L, -(nup + 3));
  }
  lua_pop(L, nup);  /* remove upvalues */
}
#endif

int
luaopen_launchdarkly_server_sdk_redis(lua_State *const l)
{
    #if LUA_VERSION_NUM == 503 || LUA_VERSION_NUM == 502
        luaL_newlib(l, launchdarkly_functions);
    #else
        luaL_register(l, "launchdarkly-server-sdk-redis",
            launchdarkly_functions);
    #endif

    return 1;
}
