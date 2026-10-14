#!/usr/bin/env python3
"""Read-only host/device inventory. Does not open a serial port or flash hardware."""
import glob
import json
from pathlib import Path
import shutil
import sys

root = Path(__file__).resolve().parents[1]
ports = sorted(set(glob.glob('/dev/ttyUSB*') + glob.glob('/dev/ttyACM*')))
by_id = Path('/dev/serial/by-id')
report = {
    'project': str(root),
    'python': sys.version.split()[0],
    'g++': shutil.which('g++'),
    'platformio': str(root / '.venv/bin/pio') if (root / '.venv/bin/pio').exists() else None,
    'serial_ports': ports,
    'serial_by_id': [str(p) for p in sorted(by_id.iterdir())] if by_id.exists() else [],
    'firmware': str(root / '.pio/build/m5stickc-plus2/firmware.bin') if (root / '.pio/build/m5stickc-plus2/firmware.bin').exists() else None,
}
print(json.dumps(report, indent=2, ensure_ascii=False))
if not ports:
    print('\nNenhuma porta serial visivel. Conecte o PLUS2 com um cabo USB-C de dados.')
    print('Se continuar ausente, confira o cabo e as permissoes do usuario (grupo dialout).')
