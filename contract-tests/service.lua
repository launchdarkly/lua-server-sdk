-- Contract test service for the Lua server-side SDK.
-- The protocol is in https://github.com/launchdarkly/sdk-test-harness/blob/main/docs/service_spec.md.

local cjson = require("cjson.safe")
local ld = require("launchdarkly_server_sdk")

cjson.encode_number_precision(16)

local M = {}

local DEFAULT_STREAMING_URL = "https://stream.launchdarkly.com"
local DEFAULT_POLLING_URL = "https://sdk.launchdarkly.com"
local DEFAULT_EVENTS_URL = "https://events.launchdarkly.com"

local CLIENT_PATH = "/clients/"

local CAPABILITIES = {
    "server-side",
    "strongly-typed",
    "service-endpoints",
    "tags",
    "server-side-polling",
    "inline-context-all",
    "anonymous-redaction",
    "client-prereq-events",
    "instance-id",
}

local clients = {}
local next_client_id = 1

-- The harness treats a JSON null the same as an absent property.
local function opt(value)
    if value == cjson.null then
        return nil
    end
    return value
end

-- A table error stops a request with that status. Any other error gives a 500.
local function fail(status, message)
    error({ status = status, message = message }, 0)
end

local function base_uri(section, endpoints, service, default)
    local uri = section and opt(section.baseUri)
    if uri == nil or uri == "" then
        uri = endpoints and opt(endpoints[service])
    end
    if uri == nil or uri == "" then
        uri = default
    end
    return uri
end

local function make_config(cfg)
    local config = {}

    local streaming = opt(cfg.streaming)
    local polling = opt(cfg.polling)
    local events = opt(cfg.events)
    local endpoints = opt(cfg.serviceEndpoints)

    -- The Lua SDK requires all three service endpoints if one is set.
    config.serviceEndpoints = {
        streamingBaseURL = base_uri(streaming, endpoints, "streaming", DEFAULT_STREAMING_URL),
        pollingBaseURL = base_uri(polling, endpoints, "polling", DEFAULT_POLLING_URL),
        eventsBaseURL = base_uri(events, endpoints, "events", DEFAULT_EVENTS_URL),
    }

    if polling and not streaming then
        local polling_config = {}
        local interval_ms = opt(polling.pollIntervalMs)
        if interval_ms then
            polling_config.intervalSeconds = math.ceil(interval_ms / 1000)
        end
        config.dataSystem = { backgroundSync = { polling = polling_config } }
    elseif streaming then
        local streaming_config = {}
        local delay_ms = opt(streaming.initialRetryDelayMs)
        if delay_ms then
            streaming_config.initialReconnectDelayMilliseconds = delay_ms
        end
        config.dataSystem = { backgroundSync = { streaming = streaming_config } }
    end

    if events then
        config.events = { enabled = true }
        local capacity = opt(events.capacity)
        if capacity and capacity > 0 then
            config.events.capacity = capacity
        end
        local flush_ms = opt(events.flushIntervalMs)
        if flush_ms and flush_ms > 0 then
            config.events.flushIntervalMilliseconds = flush_ms
        end
        if opt(events.allAttributesPrivate) then
            config.events.allAttributesPrivate = true
        end
        local private_attributes = opt(events.globalPrivateAttributes)
        if private_attributes then
            config.events.privateAttributes = private_attributes
        end
    else
        config.events = { enabled = false }
    end

    local tags = opt(cfg.tags)
    if tags then
        config.appInfo = {
            identifier = opt(tags.applicationId),
            version = opt(tags.applicationVersion),
        }
    end

    return config
end

-- Converts one context kind from the standard context JSON to a makeContext entry.
local function make_kind(json)
    local kind = { attributes = {} }
    for name, value in pairs(json) do
        if name == "key" or name == "name" or name == "anonymous" then
            kind[name] = opt(value)
        elseif name == "_meta" then
            local meta = opt(value)
            kind.privateAttributes = meta and opt(meta.privateAttributes)
        elseif name ~= "kind" then
            kind.attributes[name] = value
        end
    end
    return kind
end

local function make_context(json)
    if type(json) ~= "table" then
        fail(400, "context is required")
    end
    local kinds = {}
    if json.kind == "multi" then
        for kind, value in pairs(json) do
            if kind ~= "kind" then
                kinds[kind] = make_kind(value)
            end
        end
    else
        kinds[opt(json.kind) or "user"] = make_kind(json)
    end
    return ld.makeContext(kinds)
end

-- Closes a client. The SDK closes a client when Lua collects it.
local function close_client(id)
    clients[id] = nil
    collectgarbage("collect")
end

local function create_client(body)
    local cfg = opt(body.configuration)
    if type(cfg) ~= "table" or type(opt(cfg.credential)) ~= "string" then
        fail(400, "configuration.credential is required")
    end
    ngx.log(ngx.INFO, "creating client for test: ", tostring(opt(body.tag)))

    local timeout = opt(cfg.startWaitTimeMs)
    if timeout == nil or timeout == 0 then
        timeout = 5000
    end

    local id = next_client_id
    next_client_id = next_client_id + 1
    clients[id] = ld.clientInit(cfg.credential, timeout, make_config(cfg))

    if not clients[id]:isInitialized() and not opt(cfg.initCanFail) then
        close_client(id)
        fail(500, "client initialization failed")
    end

    ngx.header["Location"] = CLIENT_PATH .. id
    return 201
end

local VARIATIONS = {
    bool = { "boolVariation", "boolVariationDetail" },
    int = { "intVariation", "intVariationDetail" },
    double = { "doubleVariation", "doubleVariationDetail" },
    string = { "stringVariation", "stringVariationDetail" },
    any = { "jsonVariation", "jsonVariationDetail" },
}

local function evaluate(client, params)
    local methods = VARIATIONS[params.valueType]
    if methods == nil then
        fail(400, "unknown valueType: " .. tostring(params.valueType))
    end
    local context = make_context(opt(params.context))
    if opt(params.detail) then
        local detail = client[methods[2]](client, context, params.flagKey, params.defaultValue)
        return { value = detail.value, variationIndex = detail.variationIndex, reason = detail.reason }
    end
    return { value = client[methods[1]](client, context, params.flagKey, params.defaultValue) }
end

local function custom_event(client, params)
    local context = make_context(opt(params.context))
    local data = opt(params.data)
    local metric = opt(params.metricValue)
    if metric ~= nil then
        client:track(params.eventKey, context, data, metric)
    elseif data ~= nil or not opt(params.omitNullData) then
        client:track(params.eventKey, context, data)
    else
        client:track(params.eventKey, context)
    end
end

local COMMANDS = {
    evaluate = evaluate,
    evaluateAll = function(client, params)
        return { state = client:allFlags(make_context(opt(params.context))) }
    end,
    identifyEvent = function(client, params)
        client:identify(make_context(opt(params.context)))
    end,
    customEvent = custom_event,
    flushEvents = function(client)
        client:flush()
    end,
}

local function run_command(id, body)
    local client = clients[id]
    if client == nil then
        fail(404, "unknown client: " .. id)
    end
    local command = COMMANDS[body.command]
    if command == nil then
        fail(400, "unsupported command: " .. tostring(body.command))
    end
    local result = command(client, opt(body[body.command]) or {})
    if result == nil then
        return 204
    end
    return 200, result
end

local function read_json_body()
    ngx.req.read_body()
    local body, err = cjson.decode(ngx.req.get_body_data() or "")
    if type(body) ~= "table" then
        fail(400, "invalid JSON body: " .. tostring(err))
    end
    return body
end

local function stop_service()
    local master_pid = require("ngx.process").get_master_pid()
    ngx.timer.at(0, function()
        os.execute("kill -QUIT " .. master_pid)
    end)
    return 204
end

local function route(method, uri)
    if uri == "/" then
        if method == "GET" then
            return 200, { name = "lua-server-sdk", clientVersion = ld.version(), capabilities = CAPABILITIES }
        elseif method == "POST" then
            return create_client(read_json_body())
        elseif method == "DELETE" then
            return stop_service()
        end
    end

    local id = tonumber(uri:match("^" .. CLIENT_PATH .. "(%d+)$"))
    if id then
        if method == "POST" then
            return run_command(id, read_json_body())
        elseif method == "DELETE" then
            close_client(id)
            return 204
        end
    end

    fail(404, "not found: " .. method .. " " .. uri)
end

function M.handle()
    local ok, status, body = pcall(route, ngx.req.get_method(), ngx.var.uri)
    if not ok then
        local err = status
        if type(err) == "table" then
            status, body = err.status, err.message
        else
            status, body = 500, tostring(err)
        end
        ngx.log(ngx.ERR, "request failed with ", status, ": ", body)
    end

    if body == nil then
        return ngx.exit(status)
    end

    ngx.status = status
    if type(body) == "table" then
        ngx.header["Content-Type"] = "application/json"
        ngx.print(cjson.encode(body))
    else
        ngx.header["Content-Type"] = "text/plain"
        ngx.print(body)
    end
    return ngx.exit(ngx.HTTP_OK)
end

return M
