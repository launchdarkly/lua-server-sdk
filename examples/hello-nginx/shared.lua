local ld = require("launchdarkly_server_sdk")
local os = require("os")
local get_from_env_or_default = require("get_from_env_or_default")

-- Set MY_SDK_KEY to your LaunchDarkly SDK key. To specify the SDK key as an environment variable instead,
-- set LAUNCHDARKLY_SDK_KEY using '--env LAUNCHDARKLY_SDK_KEY=my-sdk-key' as a 'docker run' argument.
local MY_SDK_KEY = ""

local config = {}

return ld.clientInit(get_from_env_or_default("LAUNCHDARKLY_SDK_KEY", MY_SDK_KEY), 1000, config)
