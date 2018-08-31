{
  'targets': [
    {
      'target_name': 'jansson',
      'type': 'static_library',
      'sources': [
        'src/dump.c',
        'src/error.c',
        'src/hashtable.c',
        'src/hashtable.h',
        'src/hashtable_seed.c',
        'src/jansson.h',
        'src/jansson_config.h',
        'src/jansson_private.h',
        'src/jansson_private_config.h',
        'src/load.c',
        'src/lookup3.h',
        'src/memory.c',
        'src/pack_unpack.c',
        'src/strbuffer.c',
        'src/strbuffer.h',
        'src/strconv.c',
        'src/utf.c',
        'src/utf.h',
        'src/value.c'
      ],
      'include_dirs': [
        'src'
      ],
      'defines': [
        'HAVE_CONFIG_H',
        'HAVE_STDINT_H',
        'HAVE_SYS_PARAM_H',
        'HAVE_ENDIAN_H'
      ]
    },
    {
      'target_name': 'simple_parse',
      'type': 'executable',
      'sources': [
        'examples/simple_parse.c'
      ],
      'include_dirs': [
        'src'
      ],
      'dependencies': [
        'jansson'
      ]
    }
  ]
}
