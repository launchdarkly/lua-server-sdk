package = "launchdarkly-server-sdk-redis"

version = "1.0-0"

source = {
   url = "git+https://github.com/launchdarkly/lua-server-sdk.git"
}

dependencies = {
   "lua >= 5.1, <= 5.4",
}

external_dependencies = {
    LDREDIS = {
        header = "launchdarkly/store/redis.h"
    }
}

build = {
   type = "builtin",
   modules = {
      ["launchdarkly_server_sdk_redis"] = {
          sources = { "launchdarkly-server-sdk-redis.c" },
          incdirs = {"$(LDREDIS_INCDIR)"},
          libdirs = {"$(LDREDIS_LIBDIR)"},
          libraries = {"ldserverapi-redis"}
      }
   }
}
