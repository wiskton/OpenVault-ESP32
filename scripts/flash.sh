#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
if [[ $# -ne 1 || ! -c "$1" ]]; then
  echo "Uso: $0 /dev/ttyUSB0 (ou porta mostrada por scripts/doctor.py)" >&2
  exit 2
fi
printf 'Isto substituira o firmware do dispositivo em %s.\n' "$1"
read -r -p 'Digite GRAVAR para continuar: ' answer
[[ "$answer" == 'GRAVAR' ]] || { echo 'Gravacao cancelada.'; exit 1; }
exec .venv/bin/pio run -e m5stickc-plus2 --target upload --upload-port "$1"
