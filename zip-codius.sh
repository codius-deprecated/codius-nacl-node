#!/bin/bash 
tar --absolute-names --transform "s,$NACL_SDK_ROOT,deps/nacl_sdk/pepper_35," --transform "s,out/Release/,deps/," --transform "s,deps/,," -cz -v -f nacl_x86_32.tar.gz \
$NACL_SDK_ROOT/tools/sel_ldr_x86_32 \
$NACL_SDK_ROOT/toolchain/linux_x86_glibc/x86_64-nacl/lib32/libc.so.6b567007 \
$NACL_SDK_ROOT/toolchain/linux_x86_glibc/x86_64-nacl/lib32/libgcc_s.so.1 \
$NACL_SDK_ROOT/toolchain/linux_x86_glibc/x86_64-nacl/lib32/libpthread.so.6b567007 \
$NACL_SDK_ROOT/toolchain/linux_x86_glibc/x86_64-nacl/lib32/libstdc++.so.6 \
$NACL_SDK_ROOT/toolchain/linux_x86_glibc/x86_64-nacl/lib32/libdl.so.6b567007 \
$NACL_SDK_ROOT/toolchain/linux_x86_glibc/x86_64-nacl/lib32/libm.so.6b567007 \
$NACL_SDK_ROOT/toolchain/linux_x86_glibc/x86_64-nacl/lib32/librt.so.6b567007 \
$NACL_SDK_ROOT/toolchain/linux_x86_glibc/x86_64-nacl/lib32/runnable-ld.so \
deps/v8/out/nacl_ia32.release/lib.target/libv8.so \
out/Release/codius_node.nexe