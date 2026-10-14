#!/usr/bin/env bash
set -euo pipefail
if [[ $EUID -ne 0 ]]; then
  echo 'Execute este instalador com pkexec ou sudo; ele instala somente a regra USB.' >&2
  exit 1
fi
project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
/usr/bin/install -m 0644 -- "$project_dir/config/udev/70-openvault.rules" /etc/udev/rules.d/70-openvault.rules
/usr/bin/udevadm control --reload-rules
/usr/bin/udevadm trigger --action=change --subsystem-match=tty --property-match=ID_VENDOR_ID=1a86 --property-match=ID_MODEL_ID=55d4
/usr/bin/udevadm settle --timeout=10
printf 'Regra USB OpenVault instalada.\n'
