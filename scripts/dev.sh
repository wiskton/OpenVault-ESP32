#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build
case "${1:-test}" in
  test)
    g++ -std=c++11 -Wall -Wextra -Werror -pedantic -Iinclude tests/app_test.cpp -o build/app_test
    ./build/app_test
    ;;
  simulate)
    g++ -std=c++11 -Wall -Wextra -Werror -pedantic -Iinclude simulator/main.cpp -o build/openvault-sim
    exec ./build/openvault-sim
    ;;
  build)
    exec .venv/bin/pio run -e m5stickc-plus2
    ;;
  *) echo "Uso: $0 {test|simulate|build}" >&2; exit 2 ;;
esac
