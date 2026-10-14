#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
./scripts/install-panel.sh
if command -v pkexec >/dev/null; then
  pkexec /usr/bin/bash "$PWD/scripts/install-usb-system.sh"
else
  sudo /usr/bin/bash "$PWD/scripts/install-usb-system.sh"
fi
systemctl --user restart openvault-panel.service
printf 'Ao conectar a placa, o painel inicia em http://127.0.0.1:8765\n'
