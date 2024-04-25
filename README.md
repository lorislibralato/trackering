```sh
make build_server BUILD_DIR=$(pwd)/build && sudo setcap "CAP_SYS_RESOURCE=ep" build/server && ./build/server
make build_client BUILD_DIR=$(pwd)/build && sudo setcap "CAP_SYS_RESOURCE=ep" build/client && ./build/client
make clean BUILD_DIR=$(pwd)/build
```

## musl

`-target x86_64-pc-linux-musl`
`-static`
`#define _GNU_SOURCE`
