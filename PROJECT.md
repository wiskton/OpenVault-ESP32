# OpenVault by wiskton — estado do projeto

Projeto em `~/Documents/GitHub/OpenVault-ESP32`, para M5StickC PLUS2
(ESP32-PICO-V3-02, 8 MB de flash). Host Pop!_OS 24.04.

O usuário pediu explicitamente uma carteira funcional, além do protótipo de UI.
A versão 0.2.0 implementa:

- geração e importação BIP39 inglês, 12/24 palavras, checksum;
- Bitcoin mainnet/testnet, BIP84 conta 0, passphrase BIP39 vazia;
- exibição e confirmação de backup físico das palavras;
- cofre NVS autenticado e cifrado, PIN de 8 dígitos e troca de PIN;
- bloqueio explícito/por inatividade, atraso após erros de PIN e remoção do cofre;
- endereços indexados, QR de recebimento e xpub watch-only autorizado na placa;
- PSBT v0/P2WPKH com revisão de todos os destinos, valores e taxa e confirmação física;
- companion USB para transporte/finalização, sem seed/PIN no host e sem broadcast.

A libwally-core é baixada em revisão fixa, compilada para host e ESP32. O núcleo
é testado com vetores públicos e casos negativos. Não é uma hardware wallet auditada:
consulte docs/SECURITY.md. Não chamar limites de escopo de recursos suportados.

A primeira interface 0.1/0.1.1 e seu simulador ficam preservados como histórico.
O firmware atual usa src/main.cpp e src/wallet.cpp. A implementação UI de 0.1 em
include/openvault/app.hpp não é usada pelo firmware atual.

## Evidências e pendências de hardware

Backup integral original de 8 MB em backups/m5stickc-plus2-original-20261014,
verificado por digest do esptool; não inclui seeds criadas depois. A pasta é privada
e ignorada pelo Git. Nunca publicar esse backup.

A porta é /dev/serial/by-id/usb-1a86_USB_Single_Serial_58FC011250-if00.
O usuário liberou acesso a /dev/ttyACM0 via ACL. Essa permissão pode precisar ser
reaplicada após desconectar. Logs e instalação atualizados em docs/VALIDATION.md.

Confirmação física de leitura/conforto, anotação de seed, digitação de PIN, operação
sem USB e revisão real de transação deve ser feita pelo usuário. Não automatizar
criação de carteira persistente com seed pública de teste no aparelho do usuário.

Ainda não implementados: câmera ou microSD, entrada PSBT por QR animado, multisig,
Taproot, PSBT v2, passphrase BIP39 e proteção auditada contra extração física.

Versão 0.3.0: menu condicionado à existência do cofre, painel web local via USB
em 127.0.0.1:8765, endereço/xpub mediante confirmação física e brilho persistente.
O painel não recebe seed/PIN nem consulta saldo. Serviço de usuário openvault-panel.
