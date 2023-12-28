package = "launchdarkly-server-sdk"

rockspec_format = "3.0"

version = "1.0-0"

source = {
   url = "git+https://github.com/launchdarkly/lua-server-sdk.git",
   tag = "2.0.0" -- {{ x-release-please-version }}
}

dependencies = {
   "lua >= 5.1, <= 5.4",
}

external_dependencies = {
    LD = {
        header = "launchdarkly/server_side/bindings/c/sdk.h",
        library = "launchdarkly-cpp-server"
    },
}

test = {
    type = "command",
    script = "test.lua"
}

build = {
   type = "builtin",
   modules = {
      ["launchdarkly_server_sdk"] = {
          sources = { "launchdarkly-server-sdk.c" },
          -- Uncomment to compile with debug messages, mainly to help debug parsing configuration/context
          -- builders.
          -- defines = {"DEBUG=1"},
          incdirs = {"$(LD_INCDIR)"},
          libdirs = {"$(LD_LIBDIR)"},
          libraries = {"launchdarkly-cpp-server"},
      }
   }
}
