# OpenVault ESP32

Protótipo de interface para **M5StickC PLUS2**, inspirado no fluxo do SeedSigner.
Primeiro marco: ligar a tela, navegar com os botões e verificar a placa. Esta versão
**não cria carteiras, não recebe seeds, não salva PINs e não assina transações**.
Os itens de carteira mostram o que falta implementar; não simulam sucesso.

## Usar agora no computador

```bash
cd ~/Documents/GitHub/OpenVault-ESP32
./scripts/dev.sh test
./scripts/dev.sh simulate
```

O simulador de terminal e o firmware usam a mesma lógica de navegação em C++.
No simulador: `a` seleciona, `b` avança, `v` volta ao menu e `q` sai.
Aperte Enter depois de cada comando. O simulador não emula eletrônica nem bateria.

## Compilar para o dispositivo

```bash
./scripts/setup.sh
./scripts/dev.sh build
python3 scripts/doctor.py
```

O ambiente virtual fica no projeto. O PlatformIO baixa a plataforma Espressif,
o compilador e as bibliotecas M5Unified/M5GFX. As versões estão fixadas em
`platformio.ini` e `requirements-dev.lock` (snapshot das dependências Python). O primeiro build precisa de internet.

A saída principal é `.pio/build/m5stickc-plus2/firmware.bin`. Esse arquivo é apenas
a aplicação; para gravar bootloader, tabela de partições e aplicação nos endereços
corretos, use o comando de upload, não grave o `.bin` isoladamente no endereço zero.

## Primeiro teste no M5StickC PLUS2

1. Conecte com um cabo USB-C **de dados**.
2. Rode `python3 scripts/doctor.py` e confira a porta do dispositivo.
3. Para instalar deliberadamente: `./scripts/flash.sh /dev/ttyUSB0`.
   O script exige confirmação e substitui o firmware que estiver na placa.
4. A tela deve mostrar `OPENVAULT`, `DEMO` e o aviso inicial.
5. Pressione **A** para entrar. **B** avança no menu; **A** abre/volta.
6. Segure **B por 0,7 s** para voltar ao menu. O botão **C** mantém sua função de energia.
7. Em **Diagnóstico**, confira a imagem, contadores A/B e a leitura de bateria.
8. Teste também desconectar USB e operar com a bateria.

Monitor serial, se necessário:

```bash
.venv/bin/pio device monitor --port /dev/ttyUSB0 --baud 115200
```

Não há descoberta automática seguida de gravação. Se não houver porta, confira o
cabo e o acesso ao grupo `dialout`; não é necessário executar o build como root.

## Escolha de implementação

O primeiro firmware usa **PlatformIO + Arduino-ESP32 + M5Unified**, conforme o caminho
suportado pela M5Stack. Arduino-ESP32 usa ESP-IDF como base. Não é ainda um projeto
ESP-IDF nativo com `idf.py`: essa migração poderá ser avaliada no marco de segurança.
O núcleo da interface não depende de Arduino e tem testes de host.

O manifesto `m5stick-c` recebe configuração explícita para **8 MB de flash e PSRAM**;
a M5Unified detecta a placa e usa `board_M5StickCPlus2` como fallback no código. HOLD
(GPIO4) fica alto antes de iniciar a biblioteca. Tela em paisagem, 240 × 135.
Wi-Fi e Bluetooth são desligados no boot; isso não remove fisicamente os rádios.

## Estrutura

- `src/main.cpp`: adaptação M5Unified, tela, energia e eventos dos botões.
- `include/openvault/app.hpp`: estado, navegação e conteúdo das telas.
- `simulator/main.cpp`: simulador de terminal usando o mesmo núcleo.
- `tests/app_test.cpp`: navegação e limites de texto da tela.
- `scripts/`: preparar ambiente, testar, compilar, diagnosticar e gravar.
- `PROJECT.md`: escopo, marcos e ponto de retomada.
- `docs/`: arquitetura, limites de segurança e fontes oficiais.

O projeto foi iniciado localmente. Não há publicação automática no GitHub nem gravação
no hardware durante testes ou compilação.
