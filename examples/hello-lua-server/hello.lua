local ld = require("launchdarkly_server_sdk")

-- Allows the LaunchDarkly SDK key to be specified as an environment variable (LD_SDK_KEY)
-- or locally in this example code (YOUR_SDK_KEY).
function get_key_from_env_or(existing_key)
    if existing_key ~= "" then
        return existing_key
    end

    local env_key = os.getenv("LD_SDK_KEY")
    if env_key ~= "" then
        return env_key
    end

    error("No SDK key specified (use LD_SDK_KEY environment variable or set local YOUR_SDK_KEY)")
end

-- Set YOUR_SDK_KEY to your LaunchDarkly SDK key.
local YOUR_SDK_KEY = ""

-- Set YOUR_FEATURE_KEY to the feature flag key you want to evaluate.
local YOUR_FEATURE_KEY = "my-boolean-flag"


local config = {}

local client = ld.clientInit(get_key_from_env_or(YOUR_SDK_KEY), 1000, config)

local user = ld.makeContext({
    user = {
        key = "example-user-key",
        name = "Sandy"
    }
})

local value = client:boolVariation(user, YOUR_FEATURE_KEY, false)
print("feature flag "..YOUR_FEATURE_KEY.." is "..tostring(value).." for this user")
