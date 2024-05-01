# LaunchDarkly Lua server-side SDK Debian Bookworm example

We've built a minimal dockerized example of using the Lua SDK within a Debian Bookworm Docker container. 

## Build instructions

1. On the command line from the **root** of the repo, build the Docker image:
`docker build -t hello-debian -f ./examples/hello-debian/Dockerfile .`.
2. Run the demo with
    ```bash
    docker run --rm --name hello-debian --env LD_SDK_KEY="my-sdk-key" --env LD_FLAG_KEY="my-boolean-flag" hello-debian
    ```
3. **Note:** the SDK key and flag key are passed with environment variables into the container. The `LD_FLAG_KEY` should be a boolean-type flag in your environment. If you don't pass 
`LD_FLAG_KEY`, then the default flag key `my-boolean-flag` will be used.

You should receive the message:
> Feature flag is <true/false> for this user context
