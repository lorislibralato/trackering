```sh
make build BUILD_DIR=$(pwd)/build && sudo setcap "CAP_SYS_RESOURCE=ep" main
ldd main && ls -al main && file main
./main
```

## musl
`-target x86_64-pc-linux-musl`
`-static`
`#define _GNU_SOURCE`