# OpenVault ESP32 — contexto de continuidade

## Intenção

Desenvolver um protótipo de dispositivo Bitcoin no M5StickC PLUS2 que futuramente
possa criar/importar BIP39, desbloquear por PIN, proteger uma seed persistente,
mostrar endereços por QR e assinar PSBTs após revisão humana.

Projeto iniciado em `~/Documents/GitHub/OpenVault-ESP32` em 14/10/2026.
Host: Pop!_OS 24.04, x86_64. O usuário tem o hardware, mas nenhuma porta serial
estava visível na inspeção inicial. Não foi gravado firmware pelo assistente.

## Marco 0.1: interface e diagnóstico

- [x] Repositório e estrutura C++.
- [x] Núcleo compartilhado entre firmware e simulador de terminal.
- [x] Aviso inicial e seis itens de menu em português (ASCII na fonte embarcada).
- [x] Botões A/B, retorno por B longo e diagnóstico de contadores/bateria.
- [x] Configuração PLUS2, tela, HOLD e desligamento de rádio no boot.
- [x] Scripts de setup, build, teste e upload com confirmação.
- [x] Firmware compilado e binário gerado; detalhes em docs/VALIDATION.md.
- [ ] Validar fisicamente LCD, botões, bateria e operação sem USB.

A compilação deve ser registrada em `docs/VALIDATION.md`. Os itens Criar/Importar/
Desbloquear/Assinar são telas de planejamento, deliberadamente indisponíveis.
Não chamar esta etapa de carteira funcional.

## Próximos marcos

1. **Hardware:** conectar a placa e validar a lista de teste do README.
2. **Criptografia em host:** selecionar bibliotecas mantidas para BIP39/BIP32,
   secp256k1, endereços e PSBT; executar vetores oficiais e casos inválidos.
   Não reimplementar primitivas criptográficas.
3. **Carteira volátil de teste:** criar/importar apenas dados de teste com checksum,
   derivar endereços testnet e apresentar QR. Estudar entropia no ESP32 com rádio off.
4. **PSBT testnet:** analisar entradas/saídas, rede, troco e taxa; revisão explícita
   no dispositivo antes de assinar; validar contra uma carteira de referência.
5. **Persistência experimental:** definir modelo de ameaça, PIN/KDF, AEAD e recuperação.
   Avaliar secure boot, flash encryption e anti-rollback sem habilitar eFuses cedo.
6. **Entrada offline:** escolher câmera ou microSD compatível e transporte multipartes.

SeedSigner é referência de experiência e protocolos; o firmware Raspberry Pi não
roda diretamente no ESP32. Reuso de código exige análise das licenças.

## Regras de desenvolvimento

Não adicionar seeds reais a arquivos, logs, testes, issues ou terminal. Não mostrar
endereços de recebimento com aparência operacional antes de validar derivação.
Não declarar proteção física ou isolamento de rádio com base apenas em configuração.
Documentar claramente quais funcionalidades são implementadas e quais são planejadas.
