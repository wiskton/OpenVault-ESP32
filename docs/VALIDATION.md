# Registro de validação — 2026-10-14

## Host

- Pop!_OS 24.04 x86_64, Python 3.12.3.
- PlatformIO Core 6.1.18 instalado no `.venv` do projeto.
- `./scripts/dev.sh test`: passou com `g++`, C++11 e avisos tratados como erros.
- Simulador compilado e executado: aviso → menu → importar → retorno → sair.
- `bash -n` passou para setup, dev e flash; `py_compile` passou para doctor.

## Firmware

`pio run -e m5stickc-plus2 -j 4`: **SUCCESS**, incluindo uma compilação final
com o manifesto de placa ajustado para 8 MB. Código e bibliotecas linkados para ESP32.

- Espressif32 6.12.0 / Arduino-ESP32 2.0.17.
- M5Unified 0.2.26 / M5GFX 0.2.31.
- RAM estática: 51.948 bytes de 327.680 (15,9%); não inclui uso em runtime.
- Aplicação em flash: 1.057.805 bytes de 3.342.336 (31,6%).
- Arquivo de aplicação: `artifacts/OpenVault-ESP32-v0.1.0.bin`, 1064384 bytes.
- SHA-256: `bc53afcafa5ff31916674eee93d85d0601ce296f9f7d7207b60f7710a239aee5`.

O primeiro build identificou um nome de campo incorreto na configuração M5Unified;
foi corrigido para `fallback_board`, e a recompilação passou. Compilação não confirma
a detecção física da placa.

O `.bin` exportado é somente a aplicação. Para instalar com bootloader e partições
nos endereços corretos, use `scripts/flash.sh` conforme o README.

## Dispositivo físico

Nenhuma porta `/dev/ttyUSB*` ou `/dev/ttyACM*` estava visível no diagnóstico inicial.
Não houve upload, alteração de eFuses nem teste físico. Tela, botões, energia e leitura
de bateria precisam ser confirmados na placa. As mensagens do simulador não comprovam
esses itens.
