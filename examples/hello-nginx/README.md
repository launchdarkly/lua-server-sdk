# LaunchDarkly Lua server-side SDK NGINX example

We've built a minimal dockerized example of using the Lua SDK with [OpenResty NGINX framework](https://openresty-reference.readthedocs.io/en/latest/Lua_Nginx_API/). For more comprehensive instructions, you can visit the [Using the Lua SDK with NGINX](https://docs.launchdarkly.com/guides/sdk/nginx) guide or the [Lua reference guide](https://docs.launchdarkly.com/sdk/server-side/lua).

## Build instructions

1. On the command line from the **root** of the repo, build the image from this directory with `docker build -t hello-nginx -f ./examples/hello-nginx/Dockerfile .`.
2. Run the demo with
    ```bash
    docker run --rm --name hello-nginx -p 8123:80 --env LD_SDK_KEY="my-sdk-key" --env LD_FLAG_KEY="my-boolean-flag" hello-nginx
    ```
3. **Note:** the SDK key and flag key are passed with environment variables into the container. The `LD_FLAG_KEY` should be a boolean-type flag in your environment.
4. Open `localhost:8123` in your browser. Toggle the flag on to see a change in the page (refresh the page.)

You should receive the message:
> Feature flag is <true/false> for this user context
