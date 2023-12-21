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
    return l.clientInit("sdk-test", 0, {
        dataSystem = {
            enabled = true,
            lazyLoad = {
                cacheRefreshMilliseconds = 1000,
                source = r.makeRedisSource('redis://localhost:1234', 'test-prefix')
            },
        }
    })
end

local user = l.makeUser({
    key = "alice"
})

function TestAll:tearDown()
    collectgarbage("collect")
end

function TestAll:testInvalidRedisArguments()
    u.assertErrorMsgContains('invalid URI', r.makeRedisSource, 'not a uri', 'test-prefix')
end

function TestAll:testVariationWithRedisSource()
    local e = false
    u.assertEquals(e, makeTestClient():boolVariation(user, "test", e))
end

function TestAll:testVariationDetailWithRedisSource()
    local e = {
        value  = true,
        reason = {
            kind      = "ERROR",
            errorKind = "CLIENT_NOT_READY",
            inExperiment = false
        }
    }
    u.assertEquals(makeTestClient():boolVariationDetail(user, "test", true), e)
end

local runner = u.LuaUnit.new()
os.exit(runner:runSuite())