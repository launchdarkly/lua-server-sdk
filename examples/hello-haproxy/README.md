# LaunchDarkly Lua server-side SDK HAProxy example

We've built a minimal dockerized example of using the Lua SDK with [HAProxy](https://www.haproxy.org/). For more comprehensive instructions, you can visit the [Using the Lua SDK with HAProxy guide](https://docs.launchdarkly.com/guides/sdk/haproxy) or the [Lua reference guide](https://docs.launchdarkly.com/sdk/server-side/lua).

## Build instructions

1. On the command line from the **root** of the repo, build the image from this directory with `docker build -t hello-haproxy -f ./examples/hello-haproxy/Dockerfile .`.
2. Run the demo with:  
    ```bash
    docker run -it --rm --name hello-haproxy -p 8123:8123 --env LD_SDK_KEY="my-sdk-key" --env LD_FLAG_KEY="my-boolean-flag" hello-haproxy
    ```
3. **Note:** the SDK key and flag key are passed with environment variables into the container. The `LD_FLAG_KEY` should be a boolean-type flag in your environment.
4. Open `localhost:8123` in your browser. Toggle the flag on to see a change in the page (refresh the page.)

You should receive the message: 
> Feature flag is <true/false> for this user context
