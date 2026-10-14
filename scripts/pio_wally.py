Import('env')
import subprocess
from pathlib import Path
root=Path(env.subst('$PROJECT_DIR'))
subprocess.run([str(root/'.venv/bin/python'),str(root/'scripts/build_wally.py'),'esp32'],check=True,cwd=root,stdout=subprocess.DEVNULL)
env.Append(CPPPATH=[str(root/'.deps/libwally/include')])
env.Append(LIBS=[env.File(str(root/'.pio/wally-esp32/src/libwallycore.a')),env.File(str(root/'.pio/wally-esp32/src/secp256k1/lib/libsecp256k1.a'))])
