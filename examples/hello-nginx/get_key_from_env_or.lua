function get_key_from_env_or(name, existing_key)
    if existing_key ~= nil and existing_key ~= "" then
        --core.Debug("Using LaunchDarkly SDK key from service.lua file")
        return existing_key
    end

    local env_key = os.getenv("LD_" .. name)
    if env_key ~= nil and env_key ~= "" then
        --core.Debug("Using LaunchDarkly SDK key from LD_" .. name .. " environment variable")
        return env_key
    end

    --core.log(core.crit, "LaunchDarkly SDK key not provided! SDK won't be initialized.")
    return ""
end


return get_key_from_env_or
