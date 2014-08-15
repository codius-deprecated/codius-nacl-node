#!/bin/sh

$NACL_SDK_ROOT/toolchain/linux_x86_glibc/bin/x86_64-nacl-g++ -m32 -I/$V8_ROOT/include src/cpp/node_main.cc src/cpp/node.cc -o v8_nacl_module.nexe -L/$V8_ROOT/out/nacl_ia32.release/lib.target -lv8
