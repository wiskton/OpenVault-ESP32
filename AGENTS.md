# Working on OpenVault ESP32

Read PROJECT.md, docs/SECURITY.md and docs/VALIDATION.md before extending the wallet.
The current milestone is a UI-only hardware prototype. Keep future wallet features
clearly labeled until implemented and validated; never use real wallet secrets.

Run `./scripts/dev.sh test` for shared UI changes, and `./scripts/dev.sh build` for
firmware changes. Update the validation record with actual results and distinguish
host tests, cross-compilation and physical device tests.

Keep cryptographic functionality separate from UI and use maintained libraries
with published test vectors. Do not commit toolchains, virtualenvs, build outputs,
seeds, PINs or device dumps. Never enable irreversible eFuses as a routine build step.
