--- Server-side SDK for LaunchDarkly.
-- @module launchdarkly-server-sdk

local ffi   = require("ffi")
local cjson = require("cjson")

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

local so = ffi.load("ldserverapi")

local function applyWhenNotNil(subject, operation, value)
    if value ~= nil and value ~= cjson.null then
        operation(subject, value)
    end
end

local function toLaunchDarklyJSON(x)
    return ffi.gc(so.LDJSONDeserialize(cjson.encode(x)), so.LDJSONFree)
end

local function toLaunchDarklyJSONTransfer(x)
    return so.LDJSONDeserialize(cjson.encode(x))
end

local function fromLaunchDarklyJSON(x)
    local raw = so.LDJSONSerialize(x)
    local native = ffi.string(raw)
    so.LDFree(raw)
    return cjson.decode(native)
end

--- Details associated with an evaluation
-- @name Details
-- @class table
-- @tfield[opt] int variationIndex The index of the returned value within the
-- flag's list of variations.
-- @field value The resulting value of an evaluation
-- @tfield table reason The reason a specific value was returned
-- @tfield string reason.kind The kind of reason
-- @tfield[opt] string reason.errorKind If the kind is LD_ERROR, this contains
-- the error string.
-- @tfield[opt] string reason.ruleId If the kind is LD_RULE_MATCH this contains
-- the id of the rule.
-- @tfield[opt] int reason.ruleIndex If the kind is LD_RULE_MATCH this contains
-- the index of the rule.
-- @tfield[opt] string reason.prerequisiteKey If the kind is
-- LD_PREREQUISITE_FAILED this contains the key of the failed prerequisite.

local function convertDetails(cDetails, value)
    local details = {}
    local cReasonJSON = so.LDReasonToJSON(cDetails)
    details.reason = fromLaunchDarklyJSON(cReasonJSON)

    if cDetails.hasVariation then
        details.variationIndex = cDetails.variationIndex
    end

    details.value = value
    return details
end

local function genericVariationDetail(client, user, key, fallback, variation, valueConverter)
    local cDetails = ffi.new("struct LDDetails")
    so.LDDetailsInit(cDetails)
    local value = variation(client, user, key, fallback, cDetails)
    if valueConverter ~= nil then
        value = valueConverter(value)
    end
    local details = convertDetails(cDetails, value)
    so.LDDetailsClear(cDetails)
    return details
end

--- make a config
local function makeConfig(fields)
    local config = so.LDConfigNew(fields["key"])

    applyWhenNotNil(config, so.LDConfigSetBaseURI,               fields["baseURI"])
    applyWhenNotNil(config, so.LDConfigSetStreamURI,             fields["streamURI"])
    applyWhenNotNil(config, so.LDConfigSetEventsURI,             fields["eventsURI"])
    applyWhenNotNil(config, so.LDConfigSetStream,                fields["stream"])
    applyWhenNotNil(config, so.LDConfigSetSendEvents,            fields["sendEvents"])
    applyWhenNotNil(config, so.LDConfigSetEventsCapacity,        fields["eventsCapacity"])
    applyWhenNotNil(config, so.LDConfigSetTimeout,               fields["timeout"])
    applyWhenNotNil(config, so.LDConfigSetFlushInterval,         fields["flushInterval"])
    applyWhenNotNil(config, so.LDConfigSetPollInterval,          fields["pollInterval"])
    applyWhenNotNil(config, so.LDConfigSetOffline,               fields["offline"])
    applyWhenNotNil(config, so.LDConfigSetAllAttributesPrivate,  fields["allAttributesPrivate"])
    applyWhenNotNil(config, so.LDConfigInlineUsersInEvents,      fields["inlineUsersInEvents"])
    applyWhenNotNil(config, so.LDConfigSetUserKeysCapacity,      fields["userKeysCapacity"])
    applyWhenNotNil(config, so.LDConfigSetUserKeysFlushInterval, fields["userKeysFlushInterval"])

    local names = fields["privateAttributeNames"]

    if names ~= nil and names ~= cjson.null then
        for _, v in ipairs(names) do
            so.LDConfigAddPrivateAttribute(config, v)
        end
    end

    return config
end

--- Create a new opaque user object.
-- @tparam table fields list of user fields.
-- @tparam string fields.key The user's key
-- @tparam[opt] boolean fields.anonymous Mark the user as anonymous
-- @tparam[opt] string fields.ip Set the user's IP
-- @tparam[opt] string fields.firstName Set the user's first name
-- @tparam[opt] string fields.lastName Set the user's last name
-- @tparam[opt] string fields.email Set the user's email
-- @tparam[opt] string fields.name Set the user's name
-- @tparam[opt] string fields.avatar Set the user's avatar
-- @tparam[opt] string fields.country Set the user's country
-- @tparam[opt] string fields.secondary Set the user's secondary key
-- @tparam[opt] table fields.privateAttributeNames A list of attributes to
-- redact
-- @tparam[opt] table fields.custom Set the user's custom JSON
-- @return an opaque user object
local function makeUser(fields)
    local user = ffi.gc(so.LDUserNew(fields["key"]), so.LDUserFree)

    applyWhenNotNil(user, so.LDUserSetAnonymous, fields["anonymous"])
    applyWhenNotNil(user, so.LDUserSetIP,        fields["ip"])
    applyWhenNotNil(user, so.LDUserSetFirstName, fields["firstName"])
    applyWhenNotNil(user, so.LDUserSetLastName,  fields["lastName"])
    applyWhenNotNil(user, so.LDUserSetEmail,     fields["email"])
    applyWhenNotNil(user, so.LDUserSetName,      fields["name"])
    applyWhenNotNil(user, so.LDUserSetAvatar,    fields["avatar"])
    applyWhenNotNil(user, so.LDUserSetCountry,   fields["country"])
    applyWhenNotNil(user, so.LDUserSetSecondary, fields["secondary"])

    if fields["custom"] ~= nil then
        so.LDUserSetCustom(user, toLaunchDarklyJSONTransfer(fields["custom"]))
    end

    local names = fields["privateAttributeNames"]

    if names ~= nil and names ~= cjson.null then
        for _, v in ipairs(names) do
            so.LDUserAddPrivateAttribute(user, v)
        end
    end

    return user
end

--- Initialize a new client, and connect to LaunchDarkly.
-- @tparam table config list of configuration options
-- @tparam string config.key Environment SDK key
-- @tparam[opt] string config.baseURI Set the base URI for connecting to
-- LaunchDarkly. You probably don't need to set this unless instructed by
-- LaunchDarkly.
-- @tparam[opt] string config.streamURI Set the streaming URI for connecting to
-- LaunchDarkly. You probably don't need to set this unless instructed by
-- LaunchDarkly.
-- @tparam[opt] string config.eventsURI Set the events URI for connecting to
-- LaunchDarkly. You probably don't need to set this unless instructed by
-- LaunchDarkly.
-- @tparam[opt] boolean config.stream Enables or disables real-time streaming
-- flag updates. When set to false, an efficient caching polling mechanism is
-- used. We do not recommend disabling streaming unless you have been instructed
-- to do so by LaunchDarkly support. Defaults to true.
-- @tparam[opt] string config.sendEvents Sets whether to send analytics events
-- back to LaunchDarkly. By default, the client will send events. This differs
-- from Offline in that it only affects sending events, not streaming or
-- polling.
-- @tparam[opt] int config.eventsCapacity The capacity of the events buffer.
-- The client buffers up to this many events in memory before flushing. If the
-- capacity is exceeded before the buffer is flushed, events will be discarded.
-- @tparam[opt] int config.timeout The connection timeout to use when making
-- requests to LaunchDarkly.
-- @tparam[opt] int config.flushInterval he time between flushes of the event
-- buffer. Decreasing the flush interval means that the event buffer is less
-- likely to reach capacity.
-- @tparam[opt] int config.pollInterval The polling interval
-- (when streaming is disabled) in milliseconds.
-- @tparam[opt] boolean config.offline Sets whether this client is offline.
-- An offline client will not make any network connections to LaunchDarkly,
-- and will return default values for all feature flags.
-- @tparam[opt] boolean config.allAttributesPrivate Sets whether or not all user
-- attributes (other than the key) should be hidden from LaunchDarkly. If this
-- is true, all user attribute values will be private, not just the attributes
-- specified in PrivateAttributeNames.
-- @tparam[opt] boolean config.inlineUsersInEvents Set to true if you need to
-- see the full user details in every analytics event.
-- @tparam[opt] int config.userKeysCapacity The number of user keys that the
-- event processor can remember at an one time, so that duplicate user details
-- will not be sent in analytics.
-- @tparam[opt] int config.userKeysFlushInterval The interval at which the event
-- processor will reset its set of known user keys, in milliseconds.
-- @tparam[opt] table config.privateAttributeNames Marks a set of user attribute
-- names private. Any users sent to LaunchDarkly with this configuration active
-- will have attributes with these names removed.
-- @tparam int timeoutMilliseconds How long to wait for flags to download.
-- If the timeout is reached a non fully initialized client will be returned.
-- @return A fresh client.
local function clientInit(config, timeoutMilliseconds)
    local interface = {}

    --- An opaque client object
    -- @type Client

    local client = ffi.gc(so.LDClientInit(makeConfig(config), 1000), so.LDClientClose)

    --- Check if a client has been fully initialized. This may be useful if the
    -- initialization timeout was reached.
    -- @class function
    -- @name isInitialized
    -- @treturn boolean true if fully initialized
    interface.isInitialized = function()
        return so.LDClientIsInitialized(client)
    end

    --- Generates an identify event for a user.
    -- @class function
    -- @name identify
    -- @tparam user user An opaque user object from @{makeUser}
    -- @treturn nil
    interface.identify = function(user)
        so.LDClientIdentify(client, user)
    end

    --- Whether the LaunchDarkly client is in offline mode.
    -- @class function
    -- @name isOffline
    -- @treturn boolean true if offline
    interface.isOffline = function()
        so.LDClientIsOffline(client)
    end

    --- Immediately flushes queued events.
    -- @class function
    -- @name flush
    -- @treturn nil
    interface.flush = function()
        so.LDClientFlush(client)
    end

    --- Reports that a user has performed an event. Custom data, and a metric
    -- can be attached to the event as JSON.
    -- @class function
    -- @name track
    -- @tparam string key The name of the event
    -- @tparam user user An opaque user object from @{makeUser}
    -- @tparam[opt] table data A value to be associated with an event
    -- @tparam[optchain] number metric A value to be associated with an event
    -- @treturn nil
    interface.track = function(key, user, data, metric)
        local json = nil

        if data ~= nil then
            json = toLaunchDarklyJSON(data)
        end

        if metric ~= nil then
            so.LDClientTrackMetric(client, key, user, json, metric)
        else
            so.LDClientTrack(client, key, user, json)
        end
    end

    --- Returns a map from feature flag keys to values for a given user.
    -- This does not send analytics events back to LaunchDarkly.
    -- @class function
    -- @name allFlags
    -- @tparam user user An opaque user object from @{makeUser}
    -- @treturn table
    interface.allFlags = function(user)
        local x = so.LDAllFlags(client, user)
        if x ~= nil then
            return fromLaunchDarklyJSON(x)
        else
            return nil
        end
    end

    --- Evaluate a boolean flag
    -- @class function
    -- @name boolVariation
    -- @tparam user user An opaque user object from @{makeUser}
    -- @tparam string key The key of the flag to evaluate.
    -- @tparam boolean fallback The value to return on error
    -- @treturn boolean The evaluation result, or the fallback value
    interface.boolVariation = function(user, key, fallback)
        return so.LDBoolVariation(client, user, key, fallback, nil)
    end

    --- Evaluate a boolean flag and return an explanation
    -- @class function
    -- @name boolVariationDetail
    -- @tparam user user An opaque user object from @{makeUser}
    -- @tparam string key The key of the flag to evaluate.
    -- @tparam boolean fallback The value to return on error
    -- @treturn table The evaluation explanation
    interface.boolVariationDetail = function(user, key, fallback)
        return genericVariationDetail(client, user, key, fallback, so.LDBoolVariation, nil)
    end

    --- Evaluate an integer flag
    -- @class function
    -- @name intVariation
    -- @tparam user user An opaque user object from @{makeUser}
    -- @tparam string key The key of the flag to evaluate.
    -- @tparam int fallback The value to return on error
    -- @treturn int The evaluation result, or the fallback value
    interface.intVariation = function(user, key, fallback)
        return so.LDIntVariation(client, user, key, fallback, nil)
    end

    --- Evaluate an integer flag and return an explanation
    -- @class function
    -- @name intVariationDetail
    -- @tparam user user An opaque user object from @{makeUser}
    -- @tparam string key The key of the flag to evaluate.
    -- @tparam int fallback The value to return on error
    -- @treturn table The evaluation explanation
    interface.intVariationDetail = function(user, key, fallback)
        return genericVariationDetail(client, user, key, fallback, so.LDIntVariation, nil)
    end

    --- Evaluate a double flag
    -- @class function
    -- @name doubleVariation
    -- @tparam user user An opaque user object from @{makeUser}
    -- @tparam string key The key of the flag to evaluate.
    -- @tparam number fallback The value to return on error
    -- @treturn double The evaluation result, or the fallback value
    interface.doubleVariation = function(user, key, fallback)
        return so.LDDoubleVariation(client, user, key, fallback, nil)
    end

    --- Evaluate a double flag and return an explanation
    -- @class function
    -- @name doubleVariationDetail
    -- @tparam user user An opaque user object from @{makeUser}
    -- @tparam string key The key of the flag to evaluate.
    -- @tparam number fallback The value to return on error
    -- @treturn table The evaluation explanation
    interface.doubleVariationDetail = function(user, key, fallback)
        return genericVariationDetail(client, user, key, fallback, so.LDDoubleVariation, nil)
    end

    --- Evaluate a string flag
    -- @class function
    -- @name stringVariation
    -- @tparam user user An opaque user object from @{makeUser}
    -- @tparam string key The key of the flag to evaluate.
    -- @tparam string fallback The value to return on error
    -- @treturn string The evaluation result, or the fallback value
    interface.stringVariation = function(user, key, fallback)
        local raw = so.LDStringVariation(client, user, key, fallback, nil)
        local native = ffi.string(raw)
        so.LDFree(raw)
        return native
    end

    --- Evaluate a string flag and return an explanation
    -- @class function
    -- @name stringVariationDetail
    -- @tparam user user An opaque user object from @{makeUser}
    -- @tparam string key The key of the flag to evaluate.
    -- @tparam string fallback The value to return on error
    -- @treturn table The evaluation explanation
    interface.stringVariationDetail = function(user, key, fallback)
        local valueConverter = function(raw)
            local native = ffi.string(raw)
            so.LDFree(raw)
            return native
        end

        return genericVariationDetail(client, user, key, fallback, so.LDStringVariation, valueConverter)
    end

    --- Evaluate a json flag
    -- @class function
    -- @name jsonVariation
    -- @tparam user user An opaque user object from @{makeUser}
    -- @tparam string key The key of the flag to evaluate.
    -- @tparam table fallback The value to return on error
    -- @treturn table The evaluation result, or the fallback value
    interface.jsonVariation = function(user, key, fallback)
        local raw = so.LDJSONVariation(client, user, key, toLaunchDarklyJSON(fallback), nil)
        local native = fromLaunchDarklyJSON(raw)
        so.LDJSONFree(raw)
        return native
    end

    --- Evaluate a json flag and return an explanation
    -- @class function
    -- @name jsonVariationDetail
    -- @tparam user user An opaque user object from @{makeUser}
    -- @tparam string key The key of the flag to evaluate.
    -- @tparam table fallback The value to return on error
    -- @treturn table The evaluation explanation
    interface.jsonVariationDetail = function(user, key, fallback)
        local valueConverter = function(raw)
            local native = fromLaunchDarklyJSON(raw)
            so.LDJSONFree(raw)
            return native
        end

        return genericVariationDetail(client, user, key, toLaunchDarklyJSON(fallback), so.LDJSONVariation, valueConverter)
    end

    --- @type end

    return interface
end

--- @export
return {
    makeUser   = makeUser,
    clientInit = clientInit
}
