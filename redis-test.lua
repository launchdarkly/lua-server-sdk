local u = require('luaunit')
local l = require("launchdarkly_server_sdk")
local r = require("launchdarkly_server_sdk_redis")

function logWrite(level, line)
    print("[" .. level .. "]" .. " " .. line)
end

function logEnabled(level)
    return level == "warn" or level == "error"
end

l.registerLogger(logWrite, logEnabled)

TestAll = {}

function makeTestClient()
    local c = l.clientInit({
        key     = "sdk-test",
        offline = True
    }, 0)

    return c
end

local user = l.makeUser({
    key = "alice",
    dataSystem = {
        enabled = true,
        method = "streaming",
        params = {
            initialReconnectDelayMs = 1000
        }
    }
})

function TestAll:tearDown()
    collectgarbage("collect")
end

function TestAll:testRedisBasic()
    local c = l.clientInit({
        key                 = "sdk-test",
        dataSystem = {
            backgroundSync = {
                lazyLoad = {
                    source = r.makeRedisSource("redis://localhost:1234", "test-prefix")
                }
            }
        }
    }, 0)

    local e = false

    u.assertEquals(e, c:boolVariation(user, "test", e))
end

local runner = u.LuaUnit.new()
os.exit(runner:runSuite())
