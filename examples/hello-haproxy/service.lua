print "loaded"

local os = require("os")
local ld = require("launchdarkly_server_sdk")


-- Set YOUR_SDK_KEY to your LaunchDarkly SDK key.
local YOUR_SDK_KEY = ""

-- Set YOUR_FEATURE_KEY to the feature flag key you want to evaluate.
local YOUR_FEATURE_KEY = "my-boolean-flag"


-- Allows the LaunchDarkly SDK key to be specified as an environment variable (LD_SDK_KEY)
-- or locally in this example code (YOUR_SDK_KEY).
function get_key_from_env_or(name, existing_key)
    if existing_key ~= nil and existing_key ~= "" then
        core.Debug("Using LaunchDarkly SDK key from service.lua file")
        return existing_key
    end

    local env_key = os.getenv("LD_" .. name)
    if env_key ~= nil and env_key ~= "" then
        core.Debug("Using LaunchDarkly SDK key from LD_" .. name .. " environment variable")
        return env_key
    end

    core.log(core.crit, "LaunchDarkly SDK key not provided! SDK won't be initialized.")
    return ""
end

local config = {}
local client = ld.clientInit(get_key_from_env_or("SDK_KEY", YOUR_SDK_KEY), 1000, config)

core.register_service("launchdarkly", "http", function(applet)
    applet:start_response()

    local user = ld.makeContext({
        user =  {
            key = "example-user-key",
            name = "Sandy"
        }
    })

    if client:boolVariation(user, get_key_from_env_or("FLAG_KEY", YOUR_FEATURE_KEY), false) then
        applet:send("<p>Feature flag is true for this user</p>")
    else
        applet:send("<p>Feature flag is false for this user</p>")
    end
end)
