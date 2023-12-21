/***
Server-side SDK for LaunchDarkly Redis store.
@module launchdarkly-server-sdk-redis
*/

#include <lua.h>
#include <lauxlib.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <launchdarkly/server_side/bindings/c/integrations/redis/redis_source.h>



/***
Create a Redis data source, which can be used instead
of a LaunchDarkly Streaming or Polling data source.
@function makeRedisSource
@tparam string uri Redis URI.
@tparam string prefix Prefix to use when reading SDK data from Redis.
@return A new Redis data source, suitable for configuration in the SDK's data system.
*/
static int
LuaLDRedisMakeSource(lua_State *const l)
{
    if (lua_gettop(l) != 2) {
        return luaL_error(l, "expecting exactly 2 arguments");
    }


	const char* uri = luaL_checkstring(l, 1);
	const char* prefix = luaL_checkstring(l, 2);

    struct LDServerLazyLoadRedisResult out_result;
   	bool success = LDServerLazyLoadRedisSource_New(uri, prefix, &out_result);
    if (!success) {
        return luaL_error(l, "failed to create Redis source: %s", out_result.error_message);
    }

    LDServerLazyLoadRedisSource *i =
        (LDServerLazyLoadRedisSource *) lua_newuserdata(l, sizeof(out_result.source));

    *i = out_result.source;

    luaL_getmetatable(l, "LaunchDarklySourceInterface");
    lua_setmetatable(l, -2);

    return 1;
}

static const struct luaL_Reg launchdarkly_functions[] = {
    { "makeRedisSource", LuaLDRedisMakeSource },
    { NULL, NULL}
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
    #if LUA_VERSION_NUM >= 502
        luaL_newlib(l, launchdarkly_functions);
    #else
        luaL_register(l, "launchdarkly-server-sdk-redis",
            launchdarkly_functions);
    #endif

    return 1;
}
