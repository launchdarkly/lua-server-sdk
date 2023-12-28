local u = require('luaunit')
local l = require("launchdarkly_server_sdk")
local r = require("launchdarkly_server_sdk_redis")

TestAll = {}

function makeTestClient()
    return l.clientInit("sdk-test", 0, {
        dataSystem = {
            enabled = true,
            lazyLoad = {
                cacheRefreshMilliseconds = 1000,
                source = r.makeRedisSource('redis://localhost:1234', 'test-prefix')
            },
        },
        events = {
            enabled = false
        },
        logging = {
            basic = {
                tag = "LaunchDarklyLua",
                level = "warn"
            }
        }
    })
end

local context = l.makeContext({
    user = {
        key = "alice"
    }
})

function TestAll:tearDown()
    collectgarbage("collect")
end

function TestAll:testInvalidRedisArguments()
    u.assertErrorMsgContains('invalid URI', r.makeRedisSource, 'not a uri', 'test-prefix')
end

function TestAll:testVariationWithRedisSource()
    local e = false
    u.assertEquals(e, makeTestClient():boolVariation(context, "test", e))
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
    u.assertEquals(makeTestClient():boolVariationDetail(context, "test", true), e)
end

function TestAll:testAllFlags()
    u.assertEquals(makeTestClient():allFlags(context), {})
end

local runner = u.LuaUnit.new()
os.exit(runner:runSuite())
