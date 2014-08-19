Native Client port of V8 to run JavaScript Codius contracts
===

### To run test:

Prerequisites:
+ Linux
+ [Native Client SDK](https://developer.chrome.com/native-client/sdk/download)

```sh
export NACL_SDK_ROOT=<your-path>/nacl_sdk/pepper_<version>
```

Run the test:

```sh
node test.js
```

To run plain without outside JS:

```sh
$NACL_SDK_ROOT/tools/sel_ldr_x86_32 -h 3:3 -a -- $NACL_SDK_ROOT/toolchain/linux_x86_glibc/x86_64-nacl/lib32/runnable-ld.so --library-path .:deps/v8/out/nacl_ia32.release/lib.target:$NACL_SDK_ROOT/ports/lib/glibc_x86_32/Release:$NACL_SDK_ROOT/toolchain/linux_x86_glibc/x86_64-nacl/lib32 ./v8_nacl_module.nexe
```

### To build NaCl-sandboxed Node.js-lite

Prerequisites:
+ Linux
+ [Native Client SDK](https://developer.chrome.com/native-client/sdk/download)

```sh
cd deps/v8
make dependencies
make library=shared nacl_ia32.debug
make library=shared nacl_ia32.release
```

Build NaCl module with V8:

```sh
./build.sh
```

This will create a nexe called `node_nacl.nexe`.

### To build unsandboxed Node.js-lite

Prerequisites:
+ Linux
+ Build essentials
+ zlib (32-bit version, e.g. `lib32z1-dev`)

```sh
cd deps/v8
make dependencies
make library=shared i18nsupport=off ia32.debug
make library=shared i18nsupport=off ia32.release
```

Build Node.js-lite executable with V8:

```sh
./build-host.sh
```

This will create an executable called `node_nacl`.
