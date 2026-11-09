# Lua SDK contract test service

This directory contains the test service for the LaunchDarkly [SDK test harness](https://github.com/launchdarkly/sdk-test-harness). The service is an OpenResty application that runs the Lua SDK built from this checkout. It implements the harness's [service specification](https://github.com/launchdarkly/sdk-test-harness/blob/main/docs/service_spec.md).

## Running locally

From the **root** of the repository, build and start the service. It listens on port 8000:

```bash
docker build -t lua-contract-tests -f ./contract-tests/Dockerfile .
docker run --rm --name lua-contract-tests --network host lua-contract-tests
```

Then run the test harness against it. The harness calls back to the SDK on port 8111, so the service uses host networking:

```bash
./sdk-test-harness -url http://localhost:8000 -skip-from ./contract-tests/test-suppressions.txt
```

`test-suppressions.txt` lists the tests that are known to fail. Remove a line from it when the SDK starts to pass that test.

## Known failures

- Evaluation reasons do not include `ruleIndex`, `ruleId` or `prerequisiteKey`. The C API of the C++ SDK does not expose these fields.
- `allFlags` returns only flag values. The harness expects the full flags state, with `$flagsState` and `$valid`.
- Lua tables cannot tell an empty JSON object from an empty JSON array, so `{}` becomes `[]` in contexts, event data and default values.
- JSON null values inside objects are lost, because the SDK converts them to `nil`.
