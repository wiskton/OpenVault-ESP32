# Firmware do protótipo 0.1.0

`OpenVault-ESP32-v0.1.0.bin` é uma cópia local da aplicação compilada; o binário
não é versionado. `SHA256SUMS` registra a imagem validada. Recompile pelo README
para gerar novamente o firmware.

Este arquivo não inclui bootloader ou tabela de partições e não deve ser gravado
isoladamente no endereço zero. Use `scripts/flash.sh` para o upload completo.
É apenas uma interface demonstrativa, sem funções de carteira.
