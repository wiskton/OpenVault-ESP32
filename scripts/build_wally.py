#!/usr/bin/env python3
"""Build the pinned upstream libwally for host tests or ESP32 (no custom crypto)."""
from pathlib import Path
import shutil,subprocess,sys
ROOT=Path(__file__).resolve().parents[1]
REV='374df231d05410a31d605b31c1cc5256581381d7'
def run(args): subprocess.run([str(a) for a in args],cwd=ROOT,check=True)
def build(target='host'):
    src=ROOT/'.deps/libwally'
    if not src.exists():
        run(['git','clone','https://github.com/ElementsProject/libwally-core.git',src])
        run(['git','-C',src,'checkout',REV])
        run(['git','-C',src,'submodule','update','--init','--recursive'])
    actual=subprocess.check_output(['git','-C',str(src),'rev-parse','HEAD'],text=True).strip()
    if actual != REV: raise RuntimeError('Unexpected libwally revision')
    for a,b in [('_CMakeLists.txt','CMakeLists.txt'),('src/_CMakeLists.txt','src/CMakeLists.txt')]: shutil.copy2(src/a,src/b)
    shutil.copytree(src/'_cmake',src/'cmake',dirs_exist_ok=True)
    cmake=ROOT/'.venv/bin/cmake'
    dest=ROOT/f'.pio/wally-{target}'
    args=[cmake,'-S',src,'-B',dest,'-DCMAKE_BUILD_TYPE=MinSizeRel','-DWALLYCORE_BUILD_ELEMENTS=OFF','-DSECP256K1_ECMULT_WINDOW_SIZE=4','-DSECP256K1_ECMULT_GEN_KB=2']
    flags='-ffunction-sections -fdata-sections'
    if target=='esp32':
        compiler=Path.home()/'.platformio/packages/toolchain-xtensa-esp32/bin/xtensa-esp32-elf-gcc'
        args += ['-DCMAKE_SYSTEM_NAME=Generic','-DCMAKE_SYSTEM_PROCESSOR=xtensa',f'-DCMAKE_C_COMPILER={compiler}','-DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY']
        args += [f'-D{name}=0' for name in ['HAVE_MEMSET_S','HAVE_EXPLICIT_BZERO','HAVE_EXPLICIT_MEMSET','HAVE_MMAP','HAVE_POSIX_MEMALIGN']]
        flags+=' -mlongcalls -mtext-section-literals'
    args += ['-DCMAKE_C_FLAGS='+flags]
    run(args);run([cmake,'--build',dest,'-j','4'])
if __name__=='__main__': build(sys.argv[1] if len(sys.argv)>1 else 'host')
