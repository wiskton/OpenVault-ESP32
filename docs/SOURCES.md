# Referências consultadas

- Hardware, tela, pinagem e exemplo PlatformIO do PLUS2:
  https://docs.m5stack.com/en/core/StickC-Plus2
- M5Unified, suporte Arduino e ESP-IDF:
  https://github.com/m5stack/M5Unified
- Manifesto e versões da biblioteca:
  https://github.com/m5stack/M5Unified/blob/master/library.json
- API dos botões:
  https://docs.m5stack.com/en/arduino/m5stickc_plus2/button
- Manifesto base de placa PlatformIO:
  https://docs.platformio.org/en/latest/boards/espressif32/m5stick-c.html
- SeedSigner, referência conceitual indicada pelo usuário:
  https://github.com/SeedSigner/seedsigner

As bibliotecas são dependências externas; nenhum código do SeedSigner foi copiado
neste marco. As versões efetivas da compilação estão em `platformio.ini`.

## Base da carteira 0.2

- libwally-core upstream: https://github.com/ElementsProject/libwally-core
  Revisão fixada: 374df231d05410a31d605b31c1cc5256581381d7; licença BSD-MIT (conforme LICENSE upstream).
  O submódulo secp256k1-zkp fica fixado pela própria revisão upstream (licença MIT).
- Entropia ESP32/ESP-IDF 4.4:
  https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32/api-reference/system/random.html
- Protocolos Bitcoin:
  https://github.com/bitcoin/bips/blob/master/bip-0039.mediawiki
  https://github.com/bitcoin/bips/blob/master/bip-0084.mediawiki
  https://github.com/bitcoin/bips/blob/master/bip-0174.mediawiki

A biblioteca uBitcoin foi avaliada e não foi integrada: sua última atualização
identificada foi em 2023. Nenhum de seus fontes foi copiado para o firmware.
