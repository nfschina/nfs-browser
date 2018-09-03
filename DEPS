hooks = [
  {
    'action': [
      'python',
      'src/build/linux/sysroot_scripts/install-sysroot.py',
      '--running-as-hook'
    ],
    'pattern': '.',
    'name': 'sysroot'
  },
  {
    'action': [
      'python',
      'src/build/download_nacl_toolchains.py',
      '--mode',
      'nacl_core_sdk',
      'sync',
      '--extract'
    ],
    'pattern': '.',
    'name': 'nacltools'
  },
  {
    'action': [
      'python',
      'src/tools/clang/scripts/update.py',
      '--if-needed'
    ],
    'pattern': '.',
    'name': 'clang'
  },
  {
    'action': [
      'python',
      'src/third_party/binutils/download.py'
    ],
    'pattern': 'src/third_party/binutils',
    'name': 'binutils'
  },
  {
    'action': [
      'download_from_google_storage',
      '--no_resume',
      '--platform=linux*',
      '--extract',
      '--no_auth',
      '--bucket',
      'chromium-fonts',
      '-s',
      'src/third_party/blimp_fonts/font_bundle.tar.gz.sha1'
    ],
    'pattern':
      '.',
    'name':
      'blimp_fonts'
  },
  {
    'action': [
      'download_from_google_storage',
      '--no_resume',
      '--no_auth',
      '-u',
      '--bucket',
      'v8-wasm-fuzzer',
      '-s',
      'src/v8/test/fuzzer/wasm.tar.gz.sha1'
    ],
    'pattern':
      '.',
    'name':
      'wasm_fuzzer'
  },
  {
    'action': [
      'download_from_google_storage',
      '--no_resume',
      '--no_auth',
      '-u',
      '--bucket',
      'v8-wasm-asmjs-fuzzer',
      '-s',
      'src/v8/test/fuzzer/wasm_asmjs.tar.gz.sha1'
    ],
    'pattern':
      '.',
    'name':
      'wasm_asmjs_fuzzer'
  }
]
