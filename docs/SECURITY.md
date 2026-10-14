# Limites desta versão

Esta é uma base de interface e diagnóstico, sem material criptográfico. Os menus
não criam nem importam carteiras. Não há seed, PIN, assinatura ou cofre persistente.
A compilação inclui `OPENVAULT_DEMO_ONLY=1`; isso identifica o marco e não é uma
fronteira de segurança contra modificações futuras.

Wi-Fi e Bluetooth são desligados em software no boot. Isso não constitui isolamento
físico. USB continua disponível para desenvolvimento. A placa tem memória e portas
de depuração de um dispositivo de uso geral, sem proteção auditada para custódia.

Antes de qualquer persistência: definir ameaça de extração física, downgrade,
brute force offline de PIN, perda de energia durante escrita, firmware malicioso,
vazamento em logs/heap e recuperação após perda do aparelho. Um PIN curto e AEAD
sozinhos não impedem brute force após extração da memória. Flash encryption, secure
boot e eFuses precisam de plano de recuperação e teste em placa descartável.

Não gerar seeds com PRNG de demonstração. No ESP32, estudar os requisitos da fonte
de entropia do SDK com os rádios desativados antes de habilitar geração BIP39.
Toda futura assinatura deve validar rede e revisar valores, destinatários, troco e
taxa; testes de happy path não equivalem a auditoria.
