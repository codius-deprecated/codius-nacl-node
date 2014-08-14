Native Client port of V8 to run JavaScript Codius contracts
===

### To run test:

Prerequisites (Unix only):
+ [Native Client SDK](https://developer.chrome.com/native-client/sdk/download)

```sh
export NACL_SDK_ROOT=<your-path>/nacl_sdk/pepper_<version>
```

Run the test:

```sh
npm start
```

OR

```sh
node test-contract-runner.js test-contract.js $NACL_SDK_ROOT/tools/sel_ldr_x86_32 \
-h 3:3 -a -- $NACL_SDK_ROOT/toolchain/linux_x86_glibc/x86_64-nacl/lib32/runnable-ld.so \
--library-path ./:$NACL_SDK_ROOT/toolchain/linux_x86_glibc/x86_64-nacl/lib32 \
./v8_nacl_module.nexe 
```

### To build ported V8:

Download and build V8:

https://code.google.com/p/v8/wiki/UsingGit
https://code.google.com/p/v8/wiki/BuildingWithGYP

```sh
export V8_ROOT=<your-path>/v8/v8

make nacl_ia32 $V8_ROOT
```

Build NaCl module with V8:

```sh
$NACL_SDK_ROOT/toolchain/linux_x86_glibc/bin/x86_64-nacl-g++ -m32 -I/$V8_ROOT/include v8_nacl_module.c -o v8_nacl_module.nexe -L/$V8_ROOT/out/nacl_ia32.release/lib.target -lv8
```
