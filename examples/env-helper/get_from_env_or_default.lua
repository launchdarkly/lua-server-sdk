-- This is a helper function to get a value from an existing variable, if not empty or nil, or otherwise
-- from an environment variable.
function get_from_env_or_default(env_variable_name, local_variable)
    if local_variable ~= nil and local_variable ~= "" then
        return local_variable
    end

    local env_var = os.getenv(env_variable_name)
    if env_var ~= nil and env_var ~= "" then
        return env_var
    end

    return ""
end


return get_from_env_or_default
