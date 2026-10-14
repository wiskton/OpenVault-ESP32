#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build
.venv/bin/python scripts/build_wally.py host > build/wally-host.log 2>&1
g++ -std=c++11 -O1 -Wall -Wextra -Werror -Iinclude -I.deps/libwally/include tests/wallet_test.cpp src/wallet.cpp .pio/wally-host/src/libwallycore.a .pio/wally-host/src/secp256k1/lib/libsecp256k1.a -o build/wallet_test
.venv/bin/python tests/psbt_vectors.py
