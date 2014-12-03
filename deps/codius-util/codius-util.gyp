# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'is_clang': 0,
    'gcc_version': 0
  },

  'targets': [
    {
      'target_name': 'codius-util',
      'type': 'static_library',
      'toolsets': ['host','target'],
      'sources': [
        'include/codius-util.h',
        'src/json.c',
        'src/jsmn.c',
        'src/codius-util.c'
      ],
      'include_dirs': [
        'include',
        'src/',
      ],
      'direct_dependent_settings': {
        'include_dirs': ['include'],
      },
    }
  ],
  'target_defaults': {
    'include_dirs': [
      'include',
    ],
  },
}

# Local Variables:
# tab-width:2
# indent-tabs-mode:nil
# End:
# vim: set expandtab tabstop=2 shiftwidth=2:
