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
    local c = l.clientInit("sdk-test", {
        dataSystem = {
            enabled = true,
            lazyLoad = {
                source = r.makeRedisSource('redis://localhost:1234', 'test-prefix')
            },
        }
    })
    return c
end

local user = l.makeUser({
    key = "alice"
})

function TestAll:tearDown()
    collectgarbage("collect")
end

function TestAll:testRedisBasic()
    local e = false
    u.assertEquals(e, makeTestClient():boolVariation(user, "test", e))
end

local runner = u.LuaUnit.new()
os.exit(runner:runSuite())
