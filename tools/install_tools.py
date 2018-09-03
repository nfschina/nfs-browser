#!/usr/bin/env python

import os
import platform
import subprocess
import sys
import tarfile

os.environ['LLVM_DOWNLOAD_GOLD_PLUGIN'] = '1'

src_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
nacl_script_dir = os.path.join(src_dir, 'build/download_nacl_toolchains.py')
sysroot_script_dir = os.path.join(src_dir, 'build/linux/sysroot_scripts/install-sysroot.py')
clang_script_dir = os.path.join(src_dir, 'tools/clang/scripts/update.py')
binutils_script_dir = os.path.join(src_dir, 'third_party/binutils/download.py')
fonts_dir = os.path.join(src_dir, 'third_party/blimp_fonts/font_bundle.tar.gz')
v8_wasm_dir = os.path.join(src_dir, 'v8/test/fuzzer/wasm.tar.gz')
v8_wasm_asmjs_dir = os.path.join(src_dir, 'v8/test/fuzzer/wasm_asmjs.tar.gz')

if not os.path.exists(v8_wasm_dir):
    print v8_wasm_dir + " is not exist!"
    os._exit(0)

if not os.path.exists(v8_wasm_asmjs_dir):
    print v8_wasm_asmjs_dir + " is not exist!"
    os._exit(0)

commands = [
    ['python', nacl_script_dir, '--mode', 'nacl_core_sdk', 'sync', '--extract'],
    ['python', sysroot_script_dir, '--running-as-hook'],
    ['python', clang_script_dir, '--if-needed'],
    ['python', binutils_script_dir]
]

for command in commands:
    subprocess.call(command)

tarfile.open(v8_wasm_dir, 'r:gz').extractall(os.path.dirname(v8_wasm_dir))
tarfile.open(v8_wasm_asmjs_dir, 'r:gz').extractall(os.path.dirname(v8_wasm_asmjs_dir))

if sys.platform.startswith('linux'):
    if not os.path.exists(fonts_dir):
        print fonts_sha_dir + " is not exist!"
        os._exit(0)

    tarfile.open(fonts_dir, 'r:gz').extractall(os.path.dirname(fonts_dir))
