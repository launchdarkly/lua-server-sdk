# LaunchDarkly Lua server-side SDK basic example

First, modify `hello.lua` to inject your SDK key and boolean-type feature flag key.

Then, use the Lua interpreter to run `hello.lua`:
```bash
lua hello.lua
```

If you'd rather use environment variables to specify the SDK key or flag key, set `LAUNCHDARKLY_SDK_KEY` or `LAUNCHDARKLY_FLAG_KEY`.
```bash
LAUNCHDARKLY_SDK_KEY=my-sdk-key LAUNCHDARKLY_FLAG_KEY=my-boolean-flag lua hello.lua
```

The program should output:
> The (flag key) feature flag evaluates to (true/false).
