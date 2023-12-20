package = "launchdarkly-server-sdk"

rockspec_format = "3.0"

-- supported_platforms = {"linux"}

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
        header = "launchdarkly/server_side/bindings/c/sdk.h",
        library = "launchdarkly-cpp-server"
    },
    platforms = {
        linux = {
            BOOST = {
                -- The library depends on boost_json and boost_url, but we only need to test for the existence
                -- of one of them since they are both part of the boost distribution.
                library = "libboost_json-mt-x64.so.1.81.0"
            }
        }
    }
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
          incdirs = {"$(LD_INCDIR)"},
          libdirs = {"$(LD_LIBDIR)"},
          libraries = {"launchdarkly-cpp-server"}
      }
   }
}
