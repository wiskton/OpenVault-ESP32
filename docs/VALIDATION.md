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

## Identificação e backup físico — 2026-10-14

Após liberar acesso à porta USB, o chip foi identificado como ESP32-PICO-V3-02,
revisão v3.1, com 8 MB de flash. Backup integral lido de 0x0 até 0x800000 e salvo
em `backups/m5stickc-plus2-original-20261014/flash-original.bin` (pasta ignorada pelo Git).
`esptool verify_flash` confirmou **digest matched**. Checksum e instruções de restauração
estão junto da imagem. O backup não inclui eFuses ou RAM.

Não foi gravado firmware novo. O teste físico da interface OpenVault segue pendente.

## Instalação do protótipo — 2026-10-14

Upload autorizado pelo usuário e concluído com `pio run -e m5stickc-plus2 --target upload`
na porta serial do PLUS2. Bootloader, partições, boot_app0 e aplicação foram gravados;
o esptool confirmou os hashes de cada região e o PlatformIO retornou SUCCESS.
O backup original permanece intacto na pasta privada `backups/`.

Após reiniciar pela USB, o monitor serial confirmou:

```text
OpenVault 0.1.0: UI-only demo; no wallet or secrets.
```

A mensagem ocorre após a criação do framebuffer e a execução de `render()`; o boot
chegou ao final do setup. Não apareceram Guru Meditation, backtrace ou brownout
na janela de observação de 12 segundos. Log local em `artifacts/boot-device.log`.

Confirmação visual do LCD, navegação por A/B e autonomia sem USB ainda dependem
de teste humano. Não há funcionalidade de carteira implementada nesta versão.

## Carteira funcional e painel 0.3.0 — 2026-10-14

Os relatos acima descrevem marcos históricos. O firmware atual inclui carteira real.

- Testes C++: BIP39/BIP84, 12/24 palavras, PIN, autenticação/adulteração do cofre,
  menu sem Criar/Importar quando salvo e política PSBT aprovados.
- Assinatura do núcleo C++ conferida por libwally Python e OpenSSL, com nove
  PSBTs inválidas rejeitadas. Foram usados somente vetores públicos no host.
- Dois testes do companion e quatro testes do painel HTTP aprovados; incluem
  framing, token, Host/Origin, validação JSON e ausência de rotas seed/PIN/criação.
- JavaScript validado com node --check; git diff --check aprovado.
- Cross-compilação PlatformIO aprovada: RAM 60.508 bytes, flash 1.230.853 bytes.
- Primeiro upload perdeu comunicação; a repetição terminou SUCCESS, com hashes
  verificados. A partição NVS não foi apagada. Nenhuma carteira de teste foi criada.
- Reinício real confirmou `OPENVAULT 0.3.0 SELFTEST_OK READY` e
  `STATUS empty locked brightness:100`. Log local ignorado pelo Git.
- Serviço openvault-panel ativo; HTTP autenticado confirmou USB conectada,
  firmware 0.3.0 e ausência de cofre. Navegador mostrou visão geral e dispositivo
  com esses mesmos dados. A captura de imagem do navegador não estava disponível.

A persistência do formato cifrado foi exercitada no host. Criar uma seed real,
confirmar PIN, desligar/religar a placa e conferir a recuperação continuam sendo
operações físicas do usuário; o assistente não lê nem registra os segredos.

## Painel e cinco carteiras — 0.5.0

- Sete testes HTTP aprovados: autorização, validação, criação em espaço vazio,
  seleção 0..4, progresso público e limpeza do cache de outra carteira. Dois
  testes do companion aprovados. JavaScript passou em node --check.
- Testes C++/Python/OpenSSL aprovados novamente, incluindo criação/comandos
  estritos, preservação do nome NVS do espaço 0, nomes distintos dos cinco espaços,
  dois cofres com seeds/PINs diferentes, rejeição de PIN cruzado e restauração
  independente. A comparação signed/unsigned detectada por -Werror foi corrigida.
- Build final aprovado: RAM 60.516 bytes, flash 1.236.009 bytes. Upload final
  terminou SUCCESS com hashes verificados, sem apagamento NVS ou alteração de
  partições. A imagem instalada final está em artifacts/OpenVault-ESP32-v0.5.0.bin
  (ignorada no Git), digest em artifacts/SHA256SUMS.
- Boot 0.5 observado com SELFTEST_OK READY; log local privado/ignorado. Após a
  sincronização final, o painel confirmou USB, firmware 0.5.0, wallet:0,
  slots:00000, flow:idle. Não havia cofre salvo e nenhum foi gerado para teste.
- Nova aba exibiu os cinco espaços e formulário de criação com rede/12 ou 24
  palavras, conectada à placa. A aba antiga ficou na página de erro durante a
  parada temporária do serviço; nova aba confirmou o painel atualizado. Captura
  de imagem indisponível nesta sessão; não houve verificação visual de pixels.

Não foram automatizados os botões nem exibidas/anotadas seeds reais. Criação,
PIN, desligar/religar com cofres salvos e troca física entre carteiras reais ainda
precisam de teste humano. Não alegar validação física completa só com boot/host.

## Nomes, exportação e PIN — 0.6.0

- Nove testes HTTP aprovados, incluindo nomes válidos/injeção/tamanho, recebimento
  dos cinco rótulos, exportação pública sem segredo e rejeição de fingerprint
  divergente. Dois testes de transporte aprovados; node --check e py_compile passaram.
- Testes C++/Python/OpenSSL aprovados com a validação de nomes e as políticas de
  cofre/PSBT anteriores. Somente vetores públicos foram usados no host.
- Build final aprovado: RAM 60.540 bytes, flash 1.238.009 bytes. Upload SUCCESS com
  hashes verificados. A placa estava ociosa e sem cofre antes da instalação.
- Painel confirmou firmware 0.6.0, USB conectada, cinco rótulos recebidos e cinco
  espaços vazios. Navegador confirmou seletor com nomes e seção Exportar carteira
  para consulta, sem seed/PIN. Nenhum cofre foi criado para testar no dispositivo.

Salvar nome em cofre real, autorizar exportação de uma carteira real e completar
PIN continuam exigindo a operação física do usuário. As verificações no host e
consulta USB não substituem esse teste. Não houve criação nem exportação de seed.
