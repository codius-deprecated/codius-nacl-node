#!/bin/sh

python tools/js2c.py src/cpp/node_natives.h src/js/*.js

g++ -m32 -I/$V8_ROOT/include src/cpp/node_main.cc src/cpp/node.cc src/cpp/util.cc src/cpp/node_constants.cc src/cpp/node_javascript.cc src/cpp/string_bytes.cc src/cpp/node_buffer.cc src/cpp/smalloc.cc src/cpp/node_contextify.cc src/cpp/pipe_wrap.cc -o node_nacl -Ldeps/v8/out/ia32.debug/lib.target -Wl,-rpath=deps/v8/out/ia32.debug/lib.target -lv8 -lz -g -O0
