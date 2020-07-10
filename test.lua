local u = require('luaunit')
local l = require("launchdarkly-server-sdk")
local r = require("launchdarkly-server-sdk-redis")

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
    u.assertEquals(e, makeTestClient().boolVariation(user, "test", e))
end

function TestAll:testIntVariation()
    local e = 3
    u.assertEquals(e, makeTestClient().intVariation(user, "test", e))
end

function TestAll:testDoubleVariation()
    local e = 5.3
    u.assertEquals(e, makeTestClient().doubleVariation(user, "test", e))
end

function TestAll:testStringVariation()
    local e = "a"
    u.assertEquals(e, makeTestClient().stringVariation(user, "test", e))
end

function TestAll:testJSONVariation()
    local e = { ["a"] = "b" }
    u.assertEquals(e, makeTestClient().jsonVariation(user, "test", e))
end

function TestAll:testRedisBasic()
    local c = l.clientInit({
        key                 = "sdk-test",
        featureStoreBackend = r.makeStore({}),
        offline             = true
    }, 0)

    local e = false

    u.assertEquals(e, makeTestClient().boolVariation(user, "test", e))
end

local runner = u.LuaUnit.new()
os.exit(runner:runSuite())
