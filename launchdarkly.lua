local ffi   = require("ffi")
local cjson = require("cjson")

local ld = {}

ffi.cdef[[
    struct LDJSON;
    typedef enum {
        LDNull = 0,
        LDText,
        LDNumber,
        LDBool,
        LDObject,
        LDArray
    } LDJSONType;
    struct LDJSON * LDNewNull();
    struct LDJSON * LDNewBool(const bool boolean);
    struct LDJSON * LDNewNumber(const double number);
    struct LDJSON * LDNewText(const char *const text);
    struct LDJSON * LDNewObject();
    struct LDJSON * LDNewArray();
    bool LDSetNumber(struct LDJSON *const node, const double number);
    void LDJSONFree(struct LDJSON *const json);
    struct LDJSON * LDJSONDuplicate(const struct LDJSON *const json);
    LDJSONType LDJSONGetType(const struct LDJSON *const json);
    bool LDJSONCompare(const struct LDJSON *const left,
        const struct LDJSON *const right);
    bool LDGetBool(const struct LDJSON *const node);
    double LDGetNumber(const struct LDJSON *const node);
    const char * LDGetText(const struct LDJSON *const node);
    struct LDJSON * LDIterNext(const struct LDJSON *const iter);
    struct LDJSON * LDGetIter(const struct LDJSON *const collection);
    const char * LDIterKey(const struct LDJSON *const iter);
    unsigned int LDCollectionGetSize(
        const struct LDJSON *const collection);
    struct LDJSON * LDCollectionDetachIter(
        struct LDJSON *const collection, struct LDJSON *const iter);
    struct LDJSON * LDArrayLookup(const struct LDJSON *const array,
        const unsigned int index);
    bool LDArrayPush(struct LDJSON *const array,
        struct LDJSON *const item);
    bool LDArrayAppend(struct LDJSON *const prefix,
        const struct LDJSON *const suffix);
    struct LDJSON * LDObjectLookup(const struct LDJSON *const object,
        const char *const key);
    bool LDObjectSetKey(struct LDJSON *const object,
        const char *const key, struct LDJSON *const item);
    void LDObjectDeleteKey(struct LDJSON *const object,
        const char *const key);
    struct LDJSON * LDObjectDetachKey(struct LDJSON *const object,
        const char *const key);
    bool LDObjectMerge(struct LDJSON *const to,
        const struct LDJSON *const from);
    char * LDJSONSerialize(const struct LDJSON *const json);
    struct LDJSON * LDJSONDeserialize(const char *const text);
    struct LDStoreInterface; struct LDConfig;
    struct LDConfig * LDConfigNew(const char *const key);
    void LDConfigFree(struct LDConfig *const config);
    bool LDConfigSetBaseURI(struct LDConfig *const config,
        const char *const baseURI);
    bool LDConfigSetStreamURI(struct LDConfig *const config,
        const char *const streamURI);
    bool LDConfigSetEventsURI(struct LDConfig *const config,
        const char *const eventsURI);
    void LDConfigSetStream(struct LDConfig *const config,
        const bool stream);
    void LDConfigSetSendEvents(struct LDConfig *const config,
        const bool sendEvents);
    void LDConfigSetEventsCapacity(struct LDConfig *const config,
        const unsigned int eventsCapacity);
    void LDConfigSetTimeout(struct LDConfig *const config,
        const unsigned int milliseconds);
    void LDConfigSetFlushInterval(struct LDConfig *const config,
        const unsigned int milliseconds);
    void LDConfigSetPollInterval(struct LDConfig *const config,
        const unsigned int milliseconds);
    void LDConfigSetOffline(struct LDConfig *const config,
        const bool offline);
    void LDConfigSetUseLDD(struct LDConfig *const config,
        const bool useLDD);
    void LDConfigSetAllAttributesPrivate(struct LDConfig *const config,
        const bool allAttributesPrivate);
    void LDConfigInlineUsersInEvents(struct LDConfig *const config,
        const bool inlineUsersInEvents);
    void LDConfigSetUserKeysCapacity(struct LDConfig *const config,
        const unsigned int userKeysCapacity);
    void LDConfigSetUserKeysFlushInterval(struct LDConfig *const config,
        const unsigned int milliseconds);
    bool LDConfigAddPrivateAttribute(struct LDConfig *const config,
        const char *const attribute);
    void LDConfigSetFeatureStoreBackend(struct LDConfig *const config,
        struct LDStoreInterface *const backend);
    struct LDUser * LDUserNew(const char *const userkey);
    void LDUserFree(struct LDUser *const user);
    void LDUserSetAnonymous(struct LDUser *const user, const bool anon);
    bool LDUserSetIP(struct LDUser *const user, const char *const ip);
    bool LDUserSetFirstName(struct LDUser *const user,
        const char *const firstName);
    bool LDUserSetLastName(struct LDUser *const user,
        const char *const lastName);
    bool LDUserSetEmail(struct LDUser *const user,
        const char *const email);
    bool LDUserSetName(struct LDUser *const user,
        const char *const name);
    bool LDUserSetAvatar(struct LDUser *const user,
        const char *const avatar);
    bool LDUserSetCountry(struct LDUser *const user,
        const char *const country);
    bool LDUserSetSecondary(struct LDUser *const user,
        const char *const secondary);
    void LDUserSetCustom(struct LDUser *const user,
        struct LDJSON *const custom);
    bool LDUserAddPrivateAttribute(struct LDUser *const user,
        const char *const attribute);
    struct LDClient * LDClientInit(struct LDConfig *const config,
        const unsigned int maxwaitmilli);
    void LDClientClose(struct LDClient *const client);
    bool LDClientIsInitialized(struct LDClient *const client);
    bool LDClientTrack(struct LDClient *const client,
        const char *const key, const struct LDUser *const user,
        struct LDJSON *const data);
    bool LDClientTrackMetric(struct LDClient *const client,
        const char *const key, const struct LDUser *const user,
        struct LDJSON *const data, const double metric);
    bool LDClientIdentify(struct LDClient *const client,
        const struct LDUser *const user);
    bool LDClientIsOffline(struct LDClient *const client);
    void LDClientFlush(struct LDClient *const client);
    void * LDAlloc(const size_t bytes);
    void LDFree(void *const buffer);
    char * LDStrDup(const char *const string);
    void * LDRealloc(void *const buffer, const size_t bytes);
    void * LDCalloc(const size_t nmemb, const size_t size);
    char * LDStrNDup(const char *const str, const size_t n);
    void LDSetMemoryRoutines(void *(*const newMalloc)(const size_t),
        void (*const newFree)(void *const),
        void *(*const newRealloc)(void *const, const size_t),
        char *(*const newStrDup)(const char *const),
        void *(*const newCalloc)(const size_t, const size_t),
        char *(*const newStrNDup)(const char *const, const size_t));
    void LDGlobalInit();
    typedef enum {
        LD_LOG_FATAL = 0,
        LD_LOG_CRITICAL,
        LD_LOG_ERROR,
        LD_LOG_WARNING,
        LD_LOG_INFO,
        LD_LOG_DEBUG,
        LD_LOG_TRACE
    } LDLogLevel;
    void LDi_log(const LDLogLevel level, const char *const format, ...);
    void LDBasicLogger(const LDLogLevel level, const char *const text);
    void LDConfigureGlobalLogger(const LDLogLevel level,
        void (*logger)(const LDLogLevel level, const char *const text));
    const char * LDLogLevelToString(const LDLogLevel level);
    enum LDEvalReason {
        LD_UNKNOWN = 0,
        LD_ERROR,
        LD_OFF,
        LD_PREREQUISITE_FAILED,
        LD_TARGET_MATCH,
        LD_RULE_MATCH,
        LD_FALLTHROUGH
    };
    enum LDEvalErrorKind {
        LD_CLIENT_NOT_READY,
        LD_NULL_KEY,
        LD_STORE_ERROR,
        LD_FLAG_NOT_FOUND,
        LD_USER_NOT_SPECIFIED,
        LD_MALFORMED_FLAG,
        LD_WRONG_TYPE,
        LD_OOM
    };
    struct LDDetailsRule {
        unsigned int ruleIndex;
        char *id;
    };
    struct LDDetails {
        unsigned int variationIndex;
        bool hasVariation;
        enum LDEvalReason reason;
        union {
            enum LDEvalErrorKind errorKind;
            char *prerequisiteKey;
            struct LDDetailsRule rule;
        } extra;
    };
    void LDDetailsInit(struct LDDetails *const details);
    void LDDetailsClear(struct LDDetails *const details);
    const char * LDEvalReasonKindToString(const enum LDEvalReason kind);
    const char * LDEvalErrorKindToString(
        const enum LDEvalErrorKind kind);
    struct LDJSON * LDReasonToJSON(
        const struct LDDetails *const details);
    bool LDBoolVariation(struct LDClient *const client,
        struct LDUser *const user, const char *const key, const bool fallback,
        struct LDDetails *const details);
    int LDIntVariation(struct LDClient *const client,
        struct LDUser *const user, const char *const key, const int fallback,
        struct LDDetails *const details);
    double LDDoubleVariation(struct LDClient *const client,
        struct LDUser *const user, const char *const key, const double fallback,
        struct LDDetails *const details);
    char * LDStringVariation(struct LDClient *const client,
        struct LDUser *const user, const char *const key,
        const char* const fallback, struct LDDetails *const details);
    struct LDJSON * LDJSONVariation(struct LDClient *const client,
        struct LDUser *const user, const char *const key,
        const struct LDJSON *const fallback, struct LDDetails *const details);
    struct LDJSON * LDAllFlags(struct LDClient *const client,
        struct LDUser *const user);
]]

ld.so = ffi.load("ldserverapi")

local function applyWhenNotNil(subject, operation, value)
    if value ~= nil and value ~= cjson.null then
        operation(subject, value)
    end
end

local function toLaunchDarklyJSON(x)
    return ffi.gc(ld.so.LDJSONDeserialize(cjson.encode(x)), ld.so.LDJSONFree)
end

local function toLaunchDarklyJSONTransfer(x)
    return ld.so.LDJSONDeserialize(cjson.encode(x))
end

local function fromLaunchDarklyJSON(x)
    local raw = ld.so.LDJSONSerialize(x)
    local native = ffi.string(raw)
    ld.so.LDFree(raw)
    return cjson.decode(native)
end

local function convertDetails(cDetails, value)
    local details = {}
    local cReasonJSON = ld.so.LDReasonToJSON(cDetails)
    details.reason = fromLaunchDarklyJSON(cReasonJSON)

    if cDetails.hasVariation then
        details.variationIndex = cDetails.variationIndex
    end

    details.value = value
    return details
end

local function genericVariationDetail(client, user, key, fallback, variation, valueConverter)
    local cDetails = ffi.new("struct LDDetails")
    ld.so.LDDetailsInit(cDetails)
    local value = variation(client, user, key, fallback, cDetails)
    if valueConverter ~= nil then
        value = valueConverter(value)
    end
    local details = convertDetails(cDetails, value)
    ld.so.LDDetailsClear(cDetails)
    return details
end

local function makeConfig(fields)
    local config = ld.so.LDConfigNew(fields["key"])

    applyWhenNotNil(config, ld.so.LDConfigSetBaseURI,               fields["baseURI"])
    applyWhenNotNil(config, ld.so.LDConfigSetStreamURI,             fields["streamURI"])
    applyWhenNotNil(config, ld.so.LDConfigSetEventsURI,             fields["eventsURI"])
    applyWhenNotNil(config, ld.so.LDConfigSetStream,                fields["stream"])
    applyWhenNotNil(config, ld.so.LDConfigSetSendEvents,            fields["sendEvents"])
    applyWhenNotNil(config, ld.so.LDConfigSetEventsCapacity,        fields["eventsCapacity"])
    applyWhenNotNil(config, ld.so.LDConfigSetTimeout,               fields["timeout"])
    applyWhenNotNil(config, ld.so.LDConfigSetFlushInterval,         fields["flushInterval"])
    applyWhenNotNil(config, ld.so.LDConfigSetPollInterval,          fields["pollInterval"])
    applyWhenNotNil(config, ld.so.LDConfigSetOffline,               fields["offline"])
    applyWhenNotNil(config, ld.so.LDConfigSetAllAttributesPrivate,  fields["allAttributesPrivate"])
    applyWhenNotNil(config, ld.so.LDConfigInlineUsersInEvents,      fields["inlineUsersInEvents"])
    applyWhenNotNil(config, ld.so.LDConfigSetUserKeysCapacity,      fields["userKeysCapacity"])
    applyWhenNotNil(config, ld.so.LDConfigSetUserKeysFlushInterval, fields["userKeysFlushInterval"])

    local names = fields["privateAttributeNames"]

    if names ~= nil and names ~= cjson.null then
        for _, v in ipairs(names) do
            ld.so.LDConfigAddPrivateAttribute(config, v)
        end
    end

    return config
end

ld.makeUser = function(fields)
    local user = ffi.gc(ld.so.LDUserNew(fields["key"]), ld.so.LDUserFree)

    applyWhenNotNil(user, ld.so.LDUserSetAnonymous, fields["anonymous"])
    applyWhenNotNil(user, ld.so.LDUserSetIP,        fields["ip"])
    applyWhenNotNil(user, ld.so.LDUserSetFirstName, fields["firstName"])
    applyWhenNotNil(user, ld.so.LDUserSetLastName,  fields["lastName"])
    applyWhenNotNil(user, ld.so.LDUserSetEmail,     fields["email"])
    applyWhenNotNil(user, ld.so.LDUserSetName,      fields["name"])
    applyWhenNotNil(user, ld.so.LDUserSetAvatar,    fields["avatar"])
    applyWhenNotNil(user, ld.so.LDUserSetCountry,   fields["country"])
    applyWhenNotNil(user, ld.so.LDUserSetSecondary, fields["secondary"])

    if fields["custom"] ~= nil then
        ld.so.LDUserSetCustom(user, toLaunchDarklyJSONTransfer(fields["custom"]))
    end

    local names = fields["privateAttributeNames"]

    if names ~= nil and names ~= cjson.null then
        for _, v in ipairs(names) do
            ngx.log(ngx.ERR, "value: " .. v)
            ld.so.LDUserAddPrivateAttribute(user, v)
        end
    end

    return user
end

ld.clientInit = function(config, timoutMilliseconds)
    local interface = {}

    local client = ffi.gc(ld.so.LDClientInit(makeConfig(config), 1000), ld.so.LDClientClose)

    interface.isInitialized = function()
        return ld.so.LDClientIsInitialized(client)
    end

    interface.identify = function(user)
        ld.so.LDClientIdentify(client, user)
    end

    interface.isOffline = function()
        ld.so.LDClientIsOffline(client)
    end

    interface.flush = function()
        ld.so.LDClientFlush(client)
    end

    interface.track = function(key, user, data, metric)
        local json = nil

        if data ~= nil then
            json = toLaunchDarklyJSON(data)
        end

        if metric ~= nil then
            ld.so.LDClientTrackMetric(client, key, user, json, metric)
        else
            ld.so.LDClientTrack(client, key, user, json)
        end
    end

    interface.allFlags = function(user)
        local x = ld.so.LDAllFlags(client, user)
        if x ~= nil then
            return fromLaunchDarklyJSON(x)
        else
            return nil
        end
    end

    interface.boolVariation = function(user, key, fallback)
        return ld.so.LDBoolVariation(client, user, key, fallback, nil)
    end

    interface.boolVariationDetail = function(user, key, fallback)
        return genericVariationDetail(client, user, key, fallback, ld.so.LDBoolVariation, nil)
    end

    interface.intVariation = function(user, key, fallback)
        return ld.so.LDIntVariation(client, user, key, fallback, nil)
    end

    interface.intVariationDetail = function(user, key, fallback)
        return genericVariationDetail(client, user, key, fallback, ld.so.LDIntVariation, nil)
    end

    interface.doubleVariation = function(user, key, fallback)
        return ld.so.LDDoubleVariation(client, user, key, fallback, nil)
    end

    interface.doubleVariationDetail = function(user, key, fallback)
        return genericVariationDetail(client, user, key, fallback, ld.so.LDDoubleVariation, nil)
    end

    interface.stringVariation = function(user, key, fallback)
        local raw = ld.so.LDStringVariation(client, user, key, fallback, nil)
        local native = ffi.string(raw)
        ld.so.LDFree(raw)
        return native
    end

    interface.stringVariationDetail = function(user, key, fallback)
        local valueConverter = function(raw)
            local native = ffi.string(raw)
            ld.so.LDFree(raw)
            return native
        end

        return genericVariationDetail(client, user, key, fallback, ld.so.LDStringVariation, valueConverter)
    end

    interface.jsonVariation = function(user, key, fallback)
        local raw = ld.so.LDJSONVariation(client, user, key, toLaunchDarklyJSON(fallback), nil)
        local native = fromLaunchDarklyJSON(raw)
        ld.so.LDJSONFree(raw)
        return native
    end

    interface.jsonVariationDetail = function(user, key, fallback)
        local valueConverter = function(raw)
            local native = fromLaunchDarklyJSON(raw)
            ld.so.LDJSONFree(raw)
            return native
        end

        return genericVariationDetail(client, user, key, toLaunchDarklyJSON(fallback), ld.so.LDJSONVariation, valueConverter)
    end

    return interface
end

return ld
