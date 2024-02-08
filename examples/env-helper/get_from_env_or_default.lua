-- This is a helper function to get a value from an existing variable, if not empty or nil, or otherwise
-- from an environment variable.
function get_from_env_or_default(env_variable_name, local_variable)
    if local_variable ~= nil and local_variable ~= "" then
        core.Debug("Using LaunchDarkly SDK key from service.lua file")
        return local_variable
    end

    local env_var = os.getenv(env_variable_name)
    if env_var ~= nil and env_var ~= "" then
        core.Debug("Using LaunchDarkly SDK key from " .. env_variable_name .. " environment variable")
        return env_var
    end

 core.log(core.crit, "LaunchDarkly SDK key not provided! SDK won't be initialized.")
    return ""
end


return get_from_env_or_default
