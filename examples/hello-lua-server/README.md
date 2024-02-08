# LaunchDarkly Lua server-side SDK basic example

First, modify `hello.lua` to inject your SDK key and boolean-type feature flag key.

Then, use the Lua interpreter to run `hello.lua`:
```bash
lua hello.lua
```

If you'd rather use environment variables to specify the SDK key or flag key, set `LD_SDK_KEY` or `LD_FLAG_KEY`.
```bash
LD_SDK_KEY=my-sdk-key LD_FLAG_KEY=my-boolean-flag lua hello.lua
```

The program should output:
> Feature flag is <true/false> for this user context
