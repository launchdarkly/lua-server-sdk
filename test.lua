local u = require('luaunit')
local l = require("launchdarkly_server_sdk")

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
    key = "alice"
})

function TestAll:tearDown()
    collectgarbage("collect")
end

function TestAll:testBoolVariation()
    local e = false
    u.assertEquals(makeTestClient():boolVariation(user, "test", e), e)
end

function TestAll:testBoolVariationDetail()
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

function TestAll:testIntVariation()
    local e = 3
    u.assertEquals(makeTestClient():intVariation(user, "test", e), e)
end

function TestAll:testIntVariationDetail()
    local e = {
        value  = 5,
        reason = {
            kind      = "ERROR",
            errorKind = "CLIENT_NOT_READY",
            inExperiment = false
        }
    }
    u.assertEquals(makeTestClient():intVariationDetail(user, "test", 5), e)
end

function TestAll:testDoubleVariation()
    local e = 12.5
    u.assertEquals(makeTestClient():doubleVariation(user, "test", e), e)
end

function TestAll:testDoubleVariationDetail()
    local e = {
        value  = 6.2,
        reason = {
            kind      = "ERROR",
            errorKind = "CLIENT_NOT_READY",
            inExperiment = false
        }
    }
    u.assertEquals( makeTestClient():doubleVariationDetail(user, "test", 6.2), e)
end

function TestAll:testStringVariation()
    local e = "a"
    u.assertEquals(makeTestClient():stringVariation(user, "test", e), e)
end

function TestAll:testStringVariationDetail()
    local e = {
        value  = "f",
        reason = {
            kind      = "ERROR",
            errorKind = "CLIENT_NOT_READY",
            inExperiment = false
        }
    }
    u.assertEquals(makeTestClient():stringVariationDetail(user, "test", "f"), e)
end

function TestAll:testJSONVariation()
    local e = { ["a"] = "b" }
    u.assertEquals(makeTestClient():jsonVariation(user, "test", e), e)
end

function TestAll:testJSONVariationDetail()
    local e = {
        value  = { a = "b" },
        reason = {
            kind      = "ERROR",
            errorKind = "CLIENT_NOT_READY",
            inExperiment = false
        }
    }
    u.assertEquals(makeTestClient():jsonVariationDetail(user, "test", { a = "b" }), e)
end

function TestAll:testIdentify()
    makeTestClient():identify(user)
end

function TestAll:testVersion()
    local version = l.version()
    u.assertNotIsNil(version)
    u.assertStrMatches(version, "(%d+)%.(%d+)%.(%d+)(.*)")
end

local runner = u.LuaUnit.new()
os.exit(runner:runSuite())
