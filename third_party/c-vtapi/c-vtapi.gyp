{
  'variables': {
	  'murl_db_path%': 'db/murl.db',
  },
  'targets': [
    {
      'target_name': 'vtapi',
      'type': 'static_library',
      'sources': [
        'lib/VtApiPage.c',
        'lib/VtApiPage.h',
        'lib/VtComments.c',
        'lib/VtComments.h',
        'lib/VtDebug.c',
        'lib/VtDebug.h',
        'lib/VtDomain.c',
        'lib/VtDomain.h',
        'lib/VtFile.c',
        'lib/VtFileDist.c',
        'lib/VtFileDist.h',
        'lib/VtFile.h',
        'lib/VtIpAddr.c',
        'lib/VtIpAddr.h',
        'lib/VtObject.c',
        'lib/VtObject.h',
        'lib/VtResponse.c',
        'lib/VtResponse.h',
        'lib/VtUrl.c',
        'lib/VtUrlDist.c',
        'lib/VtUrlDist.h',
        'lib/VtUrl.h',
        'lib/c-vtapi_config.h',
        'lib/vtcapi_common.h',
        'lib/cJSON.c',
        'lib/cJSON.h'
      ],
      'include_dirs': [
        '<(DEPTH)'
      ],
      'dependencies': [
        '<(DEPTH)/third_party/curl/curl.gyp:libcurl',
        '<(DEPTH)/third_party/jansson/jansson.gyp:jansson'
      ],
      'defines': [
        'HAVE_CONFIG_H'
      ],
      'conditions': [
        ['OS=="win"', {
          'defines': [
          ]
        }]
      ]
    },
    # {
    #   'target_name': 'comments',
    #   'type': 'executable',
    #   'sources': [
    #     'examples/comments.c'
    #   ],
    #   'include_dirs': [
    #     'lib',
    #     '<(DEPTH)'
    #   ],
    #   'dependencies': [
    #     'vtapi'
    #   ]
    # },
    # {
    #   'target_name': 'domain_report',
    #   'type': 'executable',
    #   'sources': [
    #     'examples/domain_report.c'
    #   ],
    #   'include_dirs': [
    #     'lib',
    #     '<(DEPTH)'
    #   ],
    #   'dependencies': [
    #     'vtapi'
    #   ]
    # },
    # {
    #   'target_name': 'file_dist',
    #   'type': 'executable',
    #   'sources': [
    #     'examples/file_dist.c'
    #   ],
    #   'include_dirs': [
    #     'lib',
    #     '<(DEPTH)'
    #   ],
    #   'dependencies': [
    #     'vtapi'
    #   ]
    # },
    # {
    #   'target_name': 'ip_report',
    #   'type': 'executable',
    #   'sources': [
    #     'examples/ip.c'
    #   ],
    #   'include_dirs': [
    #     'lib',
    #     '<(DEPTH)'
    #   ],
    #   'dependencies': [
    #     'vtapi'
    #   ]
    # },
    # {
    #   'target_name': 'scan',
    #   'type': 'executable',
    #   'sources': [
    #     'examples/scan.c'
    #   ],
    #   'include_dirs': [
    #     'lib',
    #     '<(DEPTH)'
    #   ],
    #   'dependencies': [
    #     'vtapi'
    #   ]
    # },
    # {
    #   'target_name': 'search',
    #   'type': 'executable',
    #   'sources': [
    #     'examples/search.c'
    #   ],
    #   'include_dirs': [
    #     'lib',
    #     '<(DEPTH)'
    #   ],
    #   'dependencies': [
    #     'vtapi'
    #   ]
    # },
    {
      'target_name': 'url',
      'type': 'static_library',
      'sources': [
        'examples/url.c'
      ],
      'include_dirs': [
        'lib',
        '<(DEPTH)',
        #'examples/url.h'
      ],
      'dependencies': [
        'vtapi'
      ]
    },
    {
      'target_name': 'url_dist',
      'type': 'executable',
      'sources': [
        'examples/url_dist.c'
      ],
      'include_dirs': [
        'lib',
        '<(DEPTH)'
      ],
      'dependencies': [
        'vtapi'
      ]
    },
    {
      'target_name': 'murl_gen',
      'type': 'none',
      'copies':[{
        'destination': '<(PRODUCT_DIR)/Murl',
        'files': ['<@(murl_db_path)'],
      }],
    },
  ],
}
