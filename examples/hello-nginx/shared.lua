local ld = require("launchdarkly_server_sdk")
local os = require("os")
local get_key_from_env_or = require("get_key_from_env_or")

-- Set YOUR_SDK_KEY to your LaunchDarkly SDK key.
local YOUR_SDK_KEY = ""

local config = {}

return ld.clientInit(get_key_from_env_or("SDK_KEY", YOUR_SDK_KEY), 1000, config)
