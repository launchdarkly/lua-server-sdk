local ld = require("launchdarkly_server_sdk")
local get_from_env_or_default = dofile("../env-helper/get_from_env_or_default.lua")

-- Set MY_SDK_KEY to your LaunchDarkly SDK key.
local MY_SDK_KEY = ""

-- Set MY_FLAG_KEY to the boolean-type feature flag key you want to evaluate.
local MY_FLAG_KEY = ""


local config = {}

local sdk_key = get_from_env_or_default("LAUNCHDARKLY_SDK_KEY", MY_SDK_KEY)
local client = ld.clientInit(sdk_key, 1000, config)

local user = ld.makeContext({
    user = {
        key = "example-user-key",
        name = "Sandy"
    }
})

local flag_key = get_from_env_or_default("LAUNCHDARKLY_FLAG_KEY", MY_FLAG_KEY)
local value = client:boolVariation(user, flag_key, false)
print("The ".. flag_key .." feature flag evaluates to "..tostring(value)..".")
