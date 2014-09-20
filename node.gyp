{
  'variables': {
    'v8_use_snapshot%': 'true',
    'node_use_dtrace%': 'false',
    'node_use_etw%': 'false',
    'node_use_perfctr%': 'false',
    'node_has_winsdk%': 'false',
    'node_shared_v8%': 'false',
    'node_shared_zlib%': 'false',
    'node_shared_http_parser%': 'false',
    'node_shared_cares%': 'false',
    'node_shared_libuv%': 'false',
    'node_use_openssl%': 'true',
    'node_use_nacl%': 'true',
    'node_shared_openssl%': 'false',
    'node_use_mdb%': 'false',
    'node_v8_options%': '',
    'library_files': [
      'src/js/node.js',
      # 'src/js/_debugger.js',
      'src/js/_linklist.js',
      'src/js/assert.js',
      'src/js/buffer.js',
      # 'src/js/child_process.js',
      'src/js/console.js',
      'src/js/constants.js',
      'src/js/crypto.js',
      # 'src/js/cluster.js',
      # 'src/js/dgram.js',
      'src/js/dns.js',
      # 'src/js/domain.js',
      'src/js/events.js',
      'src/js/freelist.js',
      'src/js/fs.js',
      'src/js/http.js',
      'src/js/_http_agent.js',
      'src/js/_http_client.js',
      'src/js/_http_common.js',
      'src/js/_http_incoming.js',
      'src/js/_http_outgoing.js',
      'src/js/_http_server.js',
      # 'src/js/https.js',
      'src/js/module.js',
      'src/js/net.js',
      'src/js/os.js',
      'src/js/path.js',
      'src/js/punycode.js',
      'src/js/querystring.js',
      # 'src/js/readline.js',
      # 'src/js/repl.js',
      'src/js/smalloc.js',
      'src/js/stream.js',
      'src/js/_stream_readable.js',
      'src/js/_stream_writable.js',
      'src/js/_stream_duplex.js',
      'src/js/_stream_transform.js',
      'src/js/_stream_passthrough.js',
      'src/js/string_decoder.js',
      # 'src/js/sys.js',
      'src/js/timers.js',
      'src/js/tracing.js',
      # 'src/js/tls.js',
      # 'src/js/_tls_common.js',
      # 'src/js/_tls_legacy.js',
      # 'src/js/_tls_wrap.js',
      # 'src/js/tty.js',
      'src/js/url.js',
      'src/js/util.js',
      'src/js/vm.js',
      # 'src/js/zlib.js',
    ],
  },

  'targets': [
    {
      'target_name': 'node',
      'type': 'executable',

      'dependencies': [
        'node_js2c#host',
      ],

      'include_dirs': [
        'src',
        'tools/msvs/genfiles',
        'deps/uv/src/ares',
        '<(SHARED_INTERMEDIATE_DIR)' # for node_natives.h
      ],

      'sources': [
        # 'src/cpp/fs_event_wrap.cc',
        'src/cpp/cares_wrap.cc',
        'src/cpp/handle_wrap.cc',
        'src/cpp/node.cc',
        'src/cpp/node_async.cc',
        'src/cpp/node_buffer.cc',
        'src/cpp/node_constants.cc',
        'src/cpp/node_contextify.cc',
        'src/cpp/node_file.cc',
        'src/cpp/node_http_parser.cc',
        'src/cpp/node_javascript.cc',
        'src/cpp/node_main.cc',
        'src/cpp/node_os.cc',
        # 'src/cpp/node_v8.cc',
        # 'src/cpp/node_stat_watcher.cc',
        # 'src/cpp/node_watchdog.cc',
        # 'src/cpp/node_zlib.cc',
        'src/cpp/pipe_wrap.cc',
        # 'src/cpp/signal_wrap.cc',
        'src/cpp/smalloc.cc',
        # 'src/cpp/spawn_sync.cc',
        'src/cpp/string_bytes.cc',
        'src/cpp/stream_wrap.cc',
        'src/cpp/tcp_wrap.cc',
        'src/cpp/timer_wrap.cc',
        # 'src/cpp/tty_wrap.cc',
        # 'src/cpp/process_wrap.cc',
        # 'src/cpp/udp_wrap.cc',
        'src/cpp/uv.cc',
        # headers to make for a more pleasant IDE experience
        'src/cpp/async-wrap.h',
        'src/cpp/async-wrap-inl.h',
        'src/cpp/base-object.h',
        'src/cpp/base-object-inl.h',
        'src/cpp/env.h',
        'src/cpp/env-inl.h',
        'src/cpp/handle_wrap.h',
        'src/cpp/node.h',
        'src/cpp/node_async.h',
        'src/cpp/node_buffer.h',
        'src/cpp/node_constants.h',
        'src/cpp/node_file.h',
        'src/cpp/node_http_parser.h',
        'src/cpp/node_internals.h',
        'src/cpp/node_javascript.h',
        'src/cpp/node_root_certs.h',
        'src/cpp/node_version.h',
        'src/cpp/node_watchdog.h',
        'src/cpp/node_wrap.h',
        'src/cpp/pipe_wrap.h',
        'src/cpp/queue.h',
        'src/cpp/smalloc.h',
        # 'src/cpp/tty_wrap.h',
        'src/cpp/tcp_wrap.h',
        # 'src/cpp/udp_wrap.h',
        'src/cpp/req_wrap.h',
        'src/cpp/string_bytes.h',
        'src/cpp/stream_wrap.h',
        'src/cpp/tree.h',
        'src/cpp/util.h',
        'src/cpp/util-inl.h',
        'src/cpp/util.cc',
        'deps/http_parser/http_parser.h',
        '<(SHARED_INTERMEDIATE_DIR)/node_natives.h',
        # javascript files to make for an even more pleasant IDE experience
        '<@(library_files)',
        # node.gyp is added to the project by default.
        'common.gypi',
      ],

      'defines': [
        'NODE_WANT_INTERNALS=1',
        'ARCH="<(target_arch)"',
        'PLATFORM="<(OS)"',
        'NODE_TAG="<(node_tag)"',
        'NODE_V8_OPTIONS="<(node_v8_options)"',
      ],

      'conditions': [
        [ 'node_use_openssl=="true"', {
          'defines': [ 'HAVE_OPENSSL=1' ],
          'sources': [
            'src/cpp/node_crypto.cc',
            'src/cpp/node_crypto_bio.cc',
            'src/cpp/node_crypto_clienthello.cc',
            'src/cpp/node_crypto.h',
            'src/cpp/node_crypto_bio.h',
            'src/cpp/node_crypto_clienthello.h',
            'src/cpp/tls_wrap.cc',
            'src/cpp/tls_wrap.h'
          ],
          'conditions': [
            [ 'node_shared_openssl=="false"', {
              'dependencies': [
                './deps/openssl/openssl.gyp:openssl',

                # For tests
                './deps/openssl/openssl.gyp:openssl-cli',
              ],
              # Do not let unused OpenSSL symbols to slip away
              'xcode_settings': {
                'OTHER_LDFLAGS': [
                  '-Wl,-force_load,<(PRODUCT_DIR)/libopenssl.a',
                ],
              },
              'conditions': [
                ['OS in "linux freebsd"', {
                  'ldflags': [
                    '-Wl,--whole-archive <(PRODUCT_DIR)/libopenssl.a -Wl,--no-whole-archive',
                  ],
                }],
              ],
            }]]
        }, {
          'defines': [ 'HAVE_OPENSSL=0' ]
        }],
        [ 'node_use_dtrace=="true"', {
          'defines': [ 'HAVE_DTRACE=1' ],
          'dependencies': [
            'node_dtrace_header',
            'specialize_node_d',
          ],
          'include_dirs': [ '<(SHARED_INTERMEDIATE_DIR)' ],

          #
          # DTrace is supported on linux, solaris, mac, and bsd.  There are
          # three object files associated with DTrace support, but they're
          # not all used all the time:
          #
          #   node_dtrace.o           all configurations
          #   node_dtrace_ustack.o    not supported on mac and linux
          #   node_dtrace_provider.o  All except OS X.  "dtrace -G" is not
          #                           used on OS X.
          #
          # Note that node_dtrace_provider.cc and node_dtrace_ustack.cc do not
          # actually exist.  They're listed here to trick GYP into linking the
          # corresponding object files into the final "node" executable.  These
          # object files are generated by "dtrace -G" using custom actions
          # below, and the GYP-generated Makefiles will properly build them when
          # needed.
          #
          'sources': [ 'src/node_dtrace.cc' ],
          'conditions': [
            [ 'OS=="linux"', {
              'sources': [
                '<(SHARED_INTERMEDIATE_DIR)/node_dtrace_provider.o',
                '<(SHARED_INTERMEDIATE_DIR)/libuv_dtrace_provider.o',
              ],
            }],
            [ 'OS!="mac" and OS!="linux"', {
              'sources': [
                'src/node_dtrace_ustack.cc',
                'src/node_dtrace_provider.cc',
              ]
            }
          ] ]
        } ],
        [ 'node_use_mdb=="true"', {
          'dependencies': [ 'node_mdb' ],
          'include_dirs': [ '<(SHARED_INTERMEDIATE_DIR)' ],
          'sources': [
            'src/node_mdb.cc',
          ],
        } ],
        [ 'node_use_etw=="true"', {
          'defines': [ 'HAVE_ETW=1' ],
          'dependencies': [ 'node_etw' ],
          'sources': [
            'src/node_win32_etw_provider.h',
            'src/node_win32_etw_provider-inl.h',
            'src/node_win32_etw_provider.cc',
            'src/node_dtrace.cc',
            'tools/msvs/genfiles/node_etw_provider.h',
            'tools/msvs/genfiles/node_etw_provider.rc',
          ]
        } ],
        [ 'node_use_perfctr=="true"', {
          'defines': [ 'HAVE_PERFCTR=1' ],
          'dependencies': [ 'node_perfctr' ],
          'sources': [
            'src/node_win32_perfctr_provider.h',
            'src/node_win32_perfctr_provider.cc',
            'src/node_counters.cc',
            'src/node_counters.h',
            'tools/msvs/genfiles/node_perfctr_provider.rc',
          ]
        } ],
        [ 'v8_postmortem_support=="true"', {
          'dependencies': [ 'deps/v8/tools/gyp/v8.gyp:postmortem-metadata' ],
          'xcode_settings': {
            'OTHER_LDFLAGS': [
              '-Wl,-force_load,<(V8_BASE)',
            ],
          },
        }],
        [ 'node_shared_v8=="false"', {
          'sources': [
            'deps/v8/include/v8.h',
            'deps/v8/include/v8-debug.h',
          ],
          'dependencies': [ 'deps/v8/tools/gyp/v8.gyp:v8' ],
        }],

        [ 'node_shared_zlib=="false"', {
          # 'dependencies': [ 'deps/zlib/zlib.gyp:zlib' ],
        }],

        [ 'node_shared_http_parser=="false"', {
          'dependencies': [ 'deps/http_parser/http_parser.gyp:http_parser' ],
        }],

        [ 'node_shared_cares=="false"', {
          # 'dependencies': [ 'deps/cares/cares.gyp:cares' ],
        }],

        [ 'node_shared_libuv=="false"', {
          'dependencies': [ 'deps/uv/uv.gyp:libuv' ],
        }],

        [ 'OS=="win"', {
          'sources': [
            'src/res/node.rc',
          ],
          'defines': [
            'FD_SETSIZE=1024',
            # we need to use node's preferred "win32" rather than gyp's preferred "win"
            'PLATFORM="win32"',
            '_UNICODE=1',
          ],
          'libraries': [ '-lpsapi.lib' ]
        }, { # POSIX
          'defines': [ '__POSIX__' ],
        }],
        [ 'OS=="mac"', {
          # linking Corefoundation is needed since certain OSX debugging tools
          # like Instruments require it for some features
          'libraries': [ '-framework CoreFoundation' ],
          'defines!': [
            'PLATFORM="mac"',
          ],
          'defines': [
            # we need to use node's preferred "darwin" rather than gyp's preferred "mac"
            'PLATFORM="darwin"',
          ],
        }],
        [ 'OS=="freebsd"', {
          'libraries': [
            '-lutil',
            '-lkvm',
          ],
        }],
        [ 'OS=="solaris"', {
          'libraries': [
            '-lkstat',
            '-lumem',
          ],
          'defines!': [
            'PLATFORM="solaris"',
          ],
          'defines': [
            # we need to use node's preferred "sunos"
            # rather than gyp's preferred "solaris"
            'PLATFORM="sunos"',
          ],
        }],
        [
          'OS in "linux freebsd" and node_shared_v8=="false"', {
            'ldflags': [
              '-Wl,--whole-archive <(V8_BASE) -Wl,--no-whole-archive',
            ],
        }],
      ],
      'msvs_settings': {
        'VCLinkerTool': {
          'SubSystem': 1, # /subsystem:console
        },
      },
    },
    # generate ETW header and resource files
    {
      'target_name': 'node_etw',
      'type': 'none',
      'conditions': [
        [ 'node_use_etw=="true" and node_has_winsdk=="true"', {
          'actions': [
            {
              'action_name': 'node_etw',
              'inputs': [ 'src/res/node_etw_provider.man' ],
              'outputs': [
                'tools/msvs/genfiles/node_etw_provider.rc',
                'tools/msvs/genfiles/node_etw_provider.h',
                'tools/msvs/genfiles/node_etw_providerTEMP.BIN',
              ],
              'action': [ 'mc <@(_inputs) -h tools/msvs/genfiles -r tools/msvs/genfiles' ]
            }
          ]
        } ]
      ]
    },
    # generate perf counter header and resource files
    {
      'target_name': 'node_perfctr',
      'type': 'none',
      'conditions': [
        [ 'node_use_perfctr=="true" and node_has_winsdk=="true"', {
          'actions': [
            {
              'action_name': 'node_perfctr_man',
              'inputs': [ 'src/res/node_perfctr_provider.man' ],
              'outputs': [
                'tools/msvs/genfiles/node_perfctr_provider.h',
                'tools/msvs/genfiles/node_perfctr_provider.rc',
                'tools/msvs/genfiles/MSG00001.BIN',
              ],
              'action': [ 'ctrpp <@(_inputs) '
                          '-o tools/msvs/genfiles/node_perfctr_provider.h '
                          '-rc tools/msvs/genfiles/node_perfctr_provider.rc'
              ]
            },
          ],
        } ]
      ]
    },
    {
      'target_name': 'node_js2c',
      'type': 'none',
      'toolsets': ['host'],
      'actions': [
        {
          'action_name': 'node_js2c',
          'inputs': [
            '<@(library_files)',
            './config.gypi',
          ],
          'outputs': [
            '<(SHARED_INTERMEDIATE_DIR)/node_natives.h',
          ],
          'conditions': [
            [ 'node_use_dtrace=="false" and node_use_etw=="false"', {
              'inputs': [ 'src/notrace_macros.py' ]
            }],
            [ 'node_use_perfctr=="false"', {
              'inputs': [ 'src/perfctr_macros.py' ]
            }]
          ],
          'action': [
            '<(python)',
            'tools/js2c.py',
            '<@(_outputs)',
            '<@(_inputs)',
          ],
        },
      ],
    }, # end node_js2c
    {
      'target_name': 'node_dtrace_header',
      'type': 'none',
      'conditions': [
        [ 'node_use_dtrace=="true"', {
          'actions': [
            {
              'action_name': 'node_dtrace_header',
              'inputs': [ 'src/node_provider.d' ],
              'outputs': [ '<(SHARED_INTERMEDIATE_DIR)/node_provider.h' ],
              'action': [ 'dtrace', '-h', '-xnolibs', '-s', '<@(_inputs)',
                '-o', '<@(_outputs)' ]
            }
          ]
        } ]
      ]
    },
    {
      'target_name': 'node_mdb',
      'type': 'none',
      'conditions': [
        [ 'node_use_mdb=="true"',
          {
            'dependencies': [ 'deps/mdb_v8/mdb_v8.gyp:mdb_v8' ],
            'actions': [
              {
                'action_name': 'node_mdb',
                'inputs': [ '<(PRODUCT_DIR)/obj.target/deps/mdb_v8/mdb_v8.so' ],
                'outputs': [ '<(PRODUCT_DIR)/obj.target/node/src/node_mdb.o' ],
                'conditions': [
                  [ 'target_arch=="ia32"', {
                    'action': [ 'elfwrap', '-o', '<@(_outputs)', '<@(_inputs)' ],
                  } ],
                  [ 'target_arch=="x64"', {
                    'action': [ 'elfwrap', '-64', '-o', '<@(_outputs)', '<@(_inputs)' ],
                  } ],
                ],
              },
            ],
          },
        ],
      ],
    },
    {
      'target_name': 'node_dtrace_provider',
      'type': 'none',
      'conditions': [
        [ 'node_use_dtrace=="true" and OS!="mac" and OS!="linux"', {
          'actions': [
            {
              'action_name': 'node_dtrace_provider_o',
              'inputs': [
                '<(OBJ_DIR)/libuv/deps/uv/src/unix/core.o',
                '<(OBJ_DIR)/node/src/node_dtrace.o',
              ],
              'outputs': [
                '<(OBJ_DIR)/node/src/node_dtrace_provider.o'
              ],
              'action': [ 'dtrace', '-G', '-xnolibs', '-s', 'src/node_provider.d',
                '-s', 'deps/uv/src/unix/uv-dtrace.d', '<@(_inputs)',
                '-o', '<@(_outputs)' ]
            }
          ]
        }],
        [ 'node_use_dtrace=="true" and OS=="linux"', {
          'actions': [
            {
              'action_name': 'node_dtrace_provider_o',
              'inputs': [ 'src/node_provider.d' ],
              'outputs': [
                '<(SHARED_INTERMEDIATE_DIR)/node_dtrace_provider.o'
              ],
              'action': [
                'dtrace', '-C', '-G', '-s', '<@(_inputs)', '-o', '<@(_outputs)'
              ],
            },
            {
              'action_name': 'libuv_dtrace_provider_o',
              'inputs': [ 'deps/uv/src/unix/uv-dtrace.d' ],
              'outputs': [
                '<(SHARED_INTERMEDIATE_DIR)/libuv_dtrace_provider.o'
              ],
              'action': [
                'dtrace', '-C', '-G', '-s', '<@(_inputs)', '-o', '<@(_outputs)'
              ],
            },
          ],
        }],
      ]
    },
    {
      'target_name': 'node_dtrace_ustack',
      'type': 'none',
      'conditions': [
        [ 'node_use_dtrace=="true" and OS!="mac" and OS!="linux"', {
          'actions': [
            {
              'action_name': 'node_dtrace_ustack_constants',
              'inputs': [
                '<(V8_BASE)'
              ],
              'outputs': [
                '<(SHARED_INTERMEDIATE_DIR)/v8constants.h'
              ],
              'action': [
                'tools/genv8constants.py',
                '<@(_outputs)',
                '<@(_inputs)'
              ]
            },
            {
              'action_name': 'node_dtrace_ustack',
              'inputs': [
                'src/v8ustack.d',
                '<(SHARED_INTERMEDIATE_DIR)/v8constants.h'
              ],
              'outputs': [
                '<(OBJ_DIR)/node/src/node_dtrace_ustack.o'
              ],
              'conditions': [
                [ 'target_arch=="ia32"', {
                  'action': [
                    'dtrace', '-32', '-I<(SHARED_INTERMEDIATE_DIR)', '-Isrc',
                    '-C', '-G', '-s', 'src/v8ustack.d', '-o', '<@(_outputs)',
                  ]
                } ],
                [ 'target_arch=="x64"', {
                  'action': [
                    'dtrace', '-64', '-I<(SHARED_INTERMEDIATE_DIR)', '-Isrc',
                    '-C', '-G', '-s', 'src/v8ustack.d', '-o', '<@(_outputs)',
                  ]
                } ],
              ]
            },
          ]
        } ],
      ]
    },
    {
      'target_name': 'specialize_node_d',
      'type': 'none',
      'conditions': [
        [ 'node_use_dtrace=="true"', {
          'actions': [
            {
              'action_name': 'specialize_node_d',
              'inputs': [
                'src/node.d'
              ],
              'outputs': [
                '<(PRODUCT_DIR)/node.d',
              ],
              'action': [
                'tools/specialize_node_d.py',
                '<@(_outputs)',
                '<@(_inputs)',
                '<@(OS)',
                '<@(target_arch)',
              ],
            },
          ],
        } ],
      ]
    }
  ] # end targets
}
