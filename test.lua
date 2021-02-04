local u = require('luaunit')
local l = require("launchdarkly_server_sdk")
local r = require("launchdarkly_server_sdk_redis")

function logger(level, line)
    print(level .. ": " .. line)
end

l.registerLogger("TRACE", logger)

TestAll = {}

function makeTestClient()
    local c = l.clientInit({
        key     = "sdk-test",
        offline = true
    }, 0)

    return c
end

local user = l.makeUser({ key = "alice" })

function TestAll:tearDown()
    collectgarbage("collect")
end

function TestAll:testBoolVariation()
    local e = false
    u.assertEquals(e, makeTestClient():boolVariation(user, "test", e))
end

function TestAll:testBoolVariationDetail()
    local e = {
        value  = true,
        reason = {
            kind      = "ERROR",
            errorKind = "CLIENT_NOT_READY"
        }
    }
    u.assertEquals(e, makeTestClient():boolVariationDetail(user, "test", true))
end

function TestAll:testIntVariation()
    local e = 3
    u.assertEquals(e, makeTestClient():intVariation(user, "test", e))
end

function TestAll:testIntVariationDetail()
    local e = {
        value  = 5,
        reason = {
            kind      = "ERROR",
            errorKind = "CLIENT_NOT_READY"
        }
    }
    u.assertEquals(e, makeTestClient():intVariationDetail(user, "test", 5))
end

function TestAll:testDoubleVariation()
    local e = 5.3
    u.assertEquals(e, makeTestClient():doubleVariation(user, "test", e))
end

function TestAll:testDoubleVariationDetail()
    local e = {
        value  = 6.2,
        reason = {
            kind      = "ERROR",
            errorKind = "CLIENT_NOT_READY"
        }
    }
    u.assertEquals(e, makeTestClient():doubleVariationDetail(user, "test", 6.2))
end

function TestAll:testStringVariation()
    local e = "a"
    u.assertEquals(e, makeTestClient():stringVariation(user, "test", e))
end

function TestAll:testStringVariationDetail()
    local e = {
        value  = "f",
        reason = {
            kind      = "ERROR",
            errorKind = "CLIENT_NOT_READY"
        }
    }
    u.assertEquals(e, makeTestClient():stringVariationDetail(user, "test", "f"))
end

function TestAll:testJSONVariation()
    local e = { ["a"] = "b" }
    u.assertEquals(e, makeTestClient():jsonVariation(user, "test", e))
end

function TestAll:testJSONVariationDetail()
    local e = {
        value  = { a = "b" },
        reason = {
            kind      = "ERROR",
            errorKind = "CLIENT_NOT_READY"
        }
    }
    u.assertEquals(e, makeTestClient():jsonVariationDetail(user, "test", { a = "b" }))
end

function TestAll:testIdentify()
    makeTestClient():identify(user)
end

function TestAll:testAlias()
    makeTestClient():alias(user, l.makeUser({ key = "bob" }))
end

function TestAll:testRedisBasic()
    local c = l.clientInit({
        key                 = "sdk-test",
        featureStoreBackend = r.makeStore({}),
        offline             = true
    }, 0)

    local e = false

    u.assertEquals(e, c:boolVariation(user, "test", e))
end

local runner = u.LuaUnit.new()
os.exit(runner:runSuite())
