{
    'targets': [
        {
            'target_name': 'uci',
						#'type': '<(component)',
						'type': 'static_library',
            'sources': [
                'delta.c',
                'file.c',
                'libuci.c',
                'ucimap.c',
                'ucimap.h',
                'uci_config.h',
                'uci_internal.h',
                'uci.h',
                'util.c',
            ],
            'cflags': [
                '-Wno-implicit-function-declaration',
            ],
        },
    ],
}
