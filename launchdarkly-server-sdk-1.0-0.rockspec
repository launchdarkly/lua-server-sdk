package = "launchdarkly-server-sdk"

version = "1.0-0"

source = {
   url = "git+https://github.com/launchdarkly/lua-server-sdk.git",
   tag = "1.2.2" -- {{ x-release-please-version }}
}

dependencies = {
   "lua >= 5.1, <= 5.4",
}

external_dependencies = {
    LD = {
        header = "launchdarkly/server_side/bindings/c/sdk.h"
    }
}

build = {
   type = "builtin",
   modules = {
      ["launchdarkly_server_sdk"] = {
          sources = { "launchdarkly-server-sdk.c" },
          incdirs = {"$(LD_INCDIR)"},
          libdirs = {"$(LD_LIBDIR)"},
          libraries = {"launchdarkly-cpp-server"}
      }
   }
}
