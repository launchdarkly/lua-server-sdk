package = "launchdarkly-server-sdk"

version = "1.0-0"

source = {
   url = "." -- not online yet!
}

dependencies = {
   "lua >= 5.1, <= 5.4",
}

external_dependencies = {
    LD = {
        header = "launchdarkly/api.h"
    }
}

build = {
   type = "builtin",
   modules = {
      ["launchdarkly_server_sdk"] = {
          sources = { "launchdarkly-server-sdk.c" },
          incdirs = {"$(LD_INCDIR)"},
          libdirs = {"$(LD_LIBDIR)"},
          libraries = {"ldserverapi"}
      }
   }
}
