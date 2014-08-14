Native Client module that embeds V8 to run JavaScript contracts

//Download and install the Native Client SDK
https://developer.chrome.com/native-client/sdk/download#download-and-install-the-sdk

//Download and build V8
https://code.google.com/p/v8/wiki/UsingGit
https://code.google.com/p/v8/wiki/BuildingWithGYP

export NACL_SDK_ROOT=<your-path>/nacl_sdk/pepper_<version>
export V8_ROOT=<your-path>/v8/v8

cd $V8_ROOT
make nacl_ia32

//Compile hello_world.c
cd codius-lang-js

$NACL_SDK_ROOT/toolchain/linux_x86_glibc/bin/x86_64-nacl-g++ -m32 -I/$V8_ROOT/include v8_nacl_module.c -o v8_nacl_module.nexe -L/$V8_ROOT/out/nacl_ia32.release/lib.target -lv8

//Run the program
node test-contract-runner.js test-contract.js $NACL_SDK_ROOT/tools/sel_ldr_x86_32 -h 3:3 -a -- $NACL_SDK_ROOT/toolchain/linux_x86_glibc/x86_64-nacl/lib32/runnable-ld.so --library-path $NACL_SDK_ROOT/toolchain/linux_x86_glibc/x86_64-nacl/lib32:/$V8_ROOT/out/nacl_ia32.release/lib.target ./v8_nacl_module.nexe 
