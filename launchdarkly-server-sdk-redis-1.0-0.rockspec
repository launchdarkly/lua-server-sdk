package = "launchdarkly-server-sdk-redis"

rockspec_format = "3.0"

supported_platforms = {"linux"}

version = "1.0-0"


source = {
   url = "git+https://github.com/launchdarkly/lua-server-sdk.git"
}

dependencies = {
   "lua >= 5.1, <= 5.4",
}

external_dependencies = {
    LDREDIS = {
        header = "launchdarkly/server_side/bindings/c/integrations/redis/redis_source.h",
        library = "launchdarkly-cpp-server-redis-source"
    }
}

test = {
    type = "command",
    script = "redis-test.lua"
}

build = {
   type = "builtin",
   modules = {
      ["launchdarkly_server_sdk_redis"] = {
          sources = { "launchdarkly-server-sdk-redis.c" },
          incdirs = {"$(LDREDIS_INCDIR)"},
          libdirs = {"$(LDREDIS_LIBDIR)"},
          libraries = {"launchdarkly-cpp-server-redis-source"}
      }
   }
}
