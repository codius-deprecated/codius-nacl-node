#!/bin/sh

if [ -z "$NACL_SDK_ROOT" ]; then
    echo "Error: NaCl SDK not found, please set NACL_SDK_ROOT"
    echo " e.g. NACL_SDK_ROOT=<path-to>/nacl_sdk/pepper_<version>"
    exit 1
fi

python tools/js2c.py src/cpp/node_natives.h src/js/*.js

$NACL_SDK_ROOT/toolchain/linux_x86_glibc/bin/x86_64-nacl-g++ -m32 -Ideps/v8/include -I$NACL_SDK_ROOT/ports/include/ src/cpp/node_main.cc src/cpp/node.cc src/cpp/util.cc src/cpp/node_constants.cc src/cpp/node_javascript.cc src/cpp/string_bytes.cc src/cpp/node_buffer.cc src/cpp/smalloc.cc src/cpp/node_contextify.cc src/cpp/pipe_wrap.cc -o node_nacl.nexe -Ldeps/v8/out/nacl_ia32.release/lib.target -L$NACL_SDK_ROOT/ports/lib/glibc_x86_32/Debug -lv8 -lz -gdwarf-2 -O0

#  -Wl,-rpath=deps/v8/out/nacl_ia32.debug/lib.target
