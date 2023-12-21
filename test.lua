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
    return l.clientInit("sdk-test", 0, { offline = true })
end

local user = l.makeUser({
    key = "alice"
})

function TestAll:tearDown()
    collectgarbage("collect")
end

function TestAll:testSetAllConfigFields()
    local c = l.clientInit("sdk-test", 0, {
        offline = true,
        appInfo = {
            identifier = "MyApp",
            version = "1.0.0"
        },
        serviceEndpoints = {
            streamingBaseURL = "foo",
            pollingBaseURL = "bar",
            eventsBaseURL = "baz"
        },
        dataSystem = {
            enabled = true,
            backgroundSync = {
                streaming = {
                    initialReconnectDelayMilliseconds = 10
                }
            }
        },
        events = {
            capacity = 1000,
            contextKeysCapacity = 100,
            enabled = true,
            flushIntervalMilliseconds = 100,
            allAttributesPrivate = true,
            privateAttributes = {"/foo", "/bar"}
        }
    })
end

function TestAll:testImplicitUserContext()
    local c = l.makeContext({
        user = {
            key = "bob",
            attributes = {
                helmet = {
                    type = "construction"
                }
            },
            privateAttributes = {
              "/helmet/type"
            }
        }
    })
    u.assertEquals(c.kinds, {"user"})
    u.assertEquals(c.canonicalKey, "bob")
end


function TestAll:testMultiKindContext()
    l.makeContext({
        user = {
            key = "bob",
            attributes = {
                age = 42
            },
            privateAttributes = {
                "/age"
            }
        },
        vehicle = {
            key = "tractor",
            attributes = {
                horsepower = 2000
            }
        }
    })
    u.assertEquals(c.kinds, {"user", "vehicle"})
    u.assertEquals(c.canonicalKey, "user:bob:vehicle:tractor")
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
            errorKind = "FLAG_NOT_FOUND",
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
            errorKind = "FLAG_NOT_FOUND",
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
            errorKind = "FLAG_NOT_FOUND",
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
            errorKind = "FLAG_NOT_FOUND",
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
            errorKind = "FLAG_NOT_FOUND",
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
