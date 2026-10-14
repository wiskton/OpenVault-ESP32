# Working on OpenVault ESP32

Read PROJECT.md, docs/SECURITY.md and docs/VALIDATION.md before wallet changes.
The user expanded the scope beyond the original UI-only milestone: 0.2 implements
Bitcoin wallet operations. Keep exact supported policies and remaining limits clear.
Never use real wallet secrets in logs, terminals, source, tests or issues.

Run `./scripts/test_wallet.sh` for core/security changes, companion tests for transport,
and `./scripts/dev.sh build` for firmware changes. Record actual results and distinguish
host tests, cross-compilation, boot verification and manual button/screen testing.
Use maintained pinned libraries and published vectors; do not implement crypto primitives.

Do not commit toolchains, .deps, virtualenvs, binaries, seeds, PINs or device dumps.
Never enable irreversible eFuses as a routine build step. Firmware installation must
preserve the user's cofre and original backup; never create a known/test wallet in the
user's persistent namespace. The old terminal simulator is UI-only, not the current wallet.

0.5 supports five wallets. Never rename legacy slot 0 NVS keys (vault/receive/attempts)
or erase them during upgrades. Every operation uses the active slot; switching must
require physical confirmation, lock the key and clear previous public/PSBT UI state.
Web CREATE starts physical authorization only; seed must not enter serial/HTTP.
User requested web PIN controls in 0.7: PIN may transit authenticated loopback HTTP
and USB in volatile memory, without logging/storage/echo. Saving/unlocking with a web
PIN requires a separate physical authorization. Never inspect real user PIN fields.

User requires no sensitive wallet data in Git. Run python3 scripts/check_secrets.py
and --history before any push. Never bypass the local pre-commit/pre-push hooks.
Treat seeds, private keys, PINs, wallet exports/xpubs, PSBTs and flash/NVS backups as
private even when encrypted. Ignore rules do not protect already tracked files.
Only the explicitly known public test vectors belong in tests. Scanner is heuristic:
inspect staged paths as well. Never print matched values or read the user's secrets.
