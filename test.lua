local u = require('luaunit')
local l = require("launchdarkly-server-sdk")

function logger(level, line)
    print(level .. ": " .. line)
end

l.registerLogger("TRACE", logger)

function makeTestClient()
    local c = l.clientInit({
        key = "sdk-test"
    }, 0)

    return c
end

local user = l.makeUser({ key = "alice" })

function testBoolVariation()
    local e = false
    u.assertEquals(e, makeTestClient().boolVariation(user, "test", e))
end

function testIntVariation()
    local e = 3
    u.assertEquals(e, makeTestClient().intVariation(user, "test", e))
end

function testDoubleVariation()
    local e = 5.3
    u.assertEquals(e, makeTestClient().doubleVariation(user, "test", e))
end

function testStringVariation()
    local e = "a"
    u.assertEquals(e, makeTestClient().stringVariation(user, "test", e))
end

function testJSONVariation()
    local e = { ["a"] = "b" }
    u.assertEquals(e, makeTestClient().jsonVariation(user, "test", e))
end

local runner = u.LuaUnit.new()
os.exit(runner:runSuite())