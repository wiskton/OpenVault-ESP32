#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
project_dir="$PWD"
service_dir="${XDG_CONFIG_HOME:-$HOME/.config}/systemd/user"
mkdir -p "$service_dir"
cat > "$service_dir/openvault-panel.service" <<SERVICE
[Unit]
Description=OpenVault local USB panel

[Service]
Type=simple
WorkingDirectory=$project_dir
ExecStart=$project_dir/scripts/panel.sh
Restart=on-failure
RestartSec=3
UMask=0077

[Install]
WantedBy=default.target
SERVICE
systemctl --user daemon-reload
systemctl --user enable --now openvault-panel.service
printf 'Painel: http://127.0.0.1:8765\n'
