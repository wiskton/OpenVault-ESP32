# OpenVault by wiskton — carteira Bitcoin

Firmware C++ para **M5StickC PLUS2**, com textos grandes e controles físicos.
Versão atual 0.7.0 (fluxo web descrito ao final): criação/importação BIP39, cofre persistente com PIN, endereços Bitcoin
com QR e assinatura PSBT mediante revisão e confirmação na placa.

O software executa as operações descritas; não é uma hardware wallet auditada.
O ESP32 desta placa não oferece, nesta configuração, uma barreira validada contra
extração física. Use a testnet para validar o fluxo antes de considerar qualquer
uso que envolva fundos. Os limites estão em [docs/SECURITY.md](docs/SECURITY.md).

## Criar uma carteira no aparelho

1. Pressione **A** para entrar, depois abra **Criar carteira**.
2. **B** alterna entre **Testnet** e **Bitcoin real**; **A** confirma a rede.
3. Escolha **12 ou 24 palavras** com B e confirme com A.
4. Anote cada palavra exibida. **B** avança; o indicador mostra a posição.
5. Na última palavra, depois de percorrer todas, **segure A** para continuar.
6. Defina um **PIN de 8 dígitos**: B muda o dígito atual, A o fixa. Repita o PIN.
7. Aguarde a derivação da chave e a gravação do cofre criptografado.

As palavras só aparecem na tela da placa. Não são enviadas ao computador, impressas
na serial nem armazenadas em texto aberto. O PIN deve ser guardado por você; o backup
das palavras permite recuperação em outra carteira compatível. Não há recuperação
do PIN pelo companion.

## Importar uma seed

Abra **Importar seed**, escolha rede e 12/24 palavras. Para cada palavra, B alterna
letras e A confirma. Depois de três letras aparece uma lista de candidatas da lista
BIP39 inglesa: B percorre e A aceita a palavra. Segurar B retorna à digitação.
O checksum BIP39 é validado antes de permitir salvar o cofre.

A derivação usa **passphrase BIP39 vazia**; uma carteira que utiliza passphrase extra
não é recuperada por esta versão. Isso é informado também na escolha de palavras.

## Usar a carteira

- **Desbloquear:** digite o PIN na placa. Cinco erros impõem espera de 30 segundos.
- **Receber BTC:** veja o endereço; B percorre suas páginas e depois avança o índice.
  A alterna entre texto e QR. A rede escolhida é preservada no cofre.
- **Ver backup:** mostra novamente as palavras somente após desbloquear.
- **Exportar xpub:** segure A para autorizar a saída do xpub público pela USB.
- **Bloquear:** apaga as chaves e a entropia dos buffers próprios da carteira em RAM.
  Também ocorre automaticamente após 90 segundos sem interação nos botões.
- **Trocar PIN:** disponível após desbloquear; exige digitar e confirmar o novo PIN.
- **Apagar carteira:** disponível após desbloquear; exige segurar A. Remover o cofre
  NVS não equivale a sanitizar fisicamente todos os setores de flash.

**A** abre/confirma; **B** navega. Segurar B retorna/cancela; em telas de digitação
volta à edição ou apaga uma letra. O botão C continua sendo o botão de energia.

## Assinar uma transação

A placa não tem câmera integrada. A entrada de PSBT nesta versão usa USB; o QR de
recebimento é exibido na própria tela. A assinatura não é feita nem aprovada pelo PC.

No computador, a partir desta pasta:

```bash
.venv/bin/python companion/openvault.py status
.venv/bin/python companion/openvault.py xpub --output build/watch-only.txt
```

Para o xpub, selecione **Exportar xpub** na placa e segure A. A resposta contém
fingerprint, origem BIP84 e xpub/tpub da conta 0, adequados a uma carteira watch-only.
A exportação revela dados públicos que permitem acompanhar o histórico da conta.
Não existe comando USB para ler seed ou PIN salvo. Na versão 0.7, o painel pode
enviar um PIN para desbloquear, sempre com autorização física.

Prepare a PSBT na sua carteira watch-only. Na placa, desbloqueie e abra **Assinar PSBT**:

```bash
.venv/bin/python companion/openvault.py sign caminho/unsigned.psbt --output build/signed.psbt
```

O arquivo pode ser PSBT binário ou Base64. A política exige a transação anterior
completa (`non_witness_utxo`) para autenticar valores. Se o criador só incluiu
`witness_utxo`, obtenha as transações anteriores em hex/binário e acrescente:

```bash
.venv/bin/python companion/openvault.py sign caminho/unsigned.psbt \
  --utxo caminho/previous-tx.hex --output build/signed.psbt
```

`--utxo` pode ser repetido. Não há download automático nem envio de dados a serviços.

Percorra **todas** as páginas de revisão na placa: rede, cada endereço completo,
valor em satoshis, troco validado, taxa, versão e locktime. A última página libera
uma tela adicional: **segure A para assinar**. B cancela. O companion salva a PSBT
assinada, sem transmiti-la. Para extrair a transação final:

```bash
.venv/bin/python companion/openvault.py finalize build/signed.psbt --output build/signed.hex
```

Você pode então importar a PSBT/hex na carteira que preparou a transação.

### Política de assinatura suportada

- Bitcoin mainnet ou testnet, conforme o cofre; derivação BIP84 da conta 0.
- Entradas **native SegWit P2WPKH**, de uma única carteira; PSBT **v0**.
- Até 8 entradas e 8 saídas; Base64 até 24.000 caracteres.
- Caminhos completos `m/84'/coin'/0'/chain/index`, com chain 0/1 e índice < 100.000.
- Apenas `SIGHASH_ALL`; UTXOs completos, txids, pubkeys e scripts conferidos.
- Taxa limitada a 1.000.000 sat e 10% do total de entrada.
- Saídas com endereço reconhecido; troco só é marcado quando o caminho 1/index e
  o script forem derivados e conferidos localmente.

Taproot, multisig, PSBT v2, passphrase BIP39 e recepção por câmera/QR animado não
estão implementados e não são apresentados como suportados.

## Compilar e testar

```bash
./scripts/setup.sh
./scripts/dev.sh test                    # regressões da primeira UI
./scripts/test_wallet.sh                 # carteira, cofre e assinatura
.venv/bin/python -m unittest discover -s tests -p test_companion.py
./scripts/dev.sh build
python3 scripts/doctor.py
```

`setup.sh` prepara o ambiente Python. No primeiro build, `scripts/build_wally.py`
obtém a revisão fixada da **libwally-core** e seus submódulos. As primitivas
criptográficas e PSBT são da biblioteca upstream; o projeto implementa a política,
o formato de cofre e a interface. Downloads ficam em `.deps/`, ignorado pelo Git.

Para instalar deliberadamente no dispositivo:

```bash
./scripts/flash.sh /dev/ttyACM0
```

O upload substitui o firmware e usa os offsets de bootloader/partições corretos.
Não grave o arquivo de aplicação isolado no endereço zero. O backup original de
8 MB fica em `backups/`, privado e fora do Git; ele foi feito **antes da criação de
carteiras**, portanto não é backup das seeds criadas depois.

## Organização

- `src/main.cpp`: UI, botões, entropia ESP32, NVS e protocolo USB.
- `src/wallet.cpp` / `include/openvault/wallet.hpp`: BIP39/BIP84, cofre e política PSBT.
- `companion/openvault.py`: transporte local e finalização, sem transmissão à rede.
- `scripts/build_wally.py`: build upstream fixado para ESP32 e host.
- `tests/`: vetores públicos, casos negativos e protocolo serial fragmentado.
- `docs/VALIDATION.md`: evidências reais de build, testes e instalação.
- `simulator/` e `include/openvault/app.hpp`: simulador da primeira interface;
  não executam as operações da carteira atual.

## Painel web local — 0.3.0

Abra **http://127.0.0.1:8765** neste computador com a placa conectada por USB.
O serviço local `openvault-panel` mantém o painel disponível após entrar no sistema.
Para instalar o serviço em outro checkout: `./scripts/install-panel.sh`.
Para executar manualmente: `./scripts/panel.sh` (pare o serviço antes de usar a mesma porta).

- **Visão geral:** conexão USB e estado real do cofre.
- **Receber:** peça um endereço, percorra suas páginas na placa com B e segure A
  na última página. O painel recebe o endereço público e gera o QR localmente.
- **Transações:** selecione Assinar PSBT na placa, envie o arquivo no painel,
  confira cada saída/taxa e confirme fisicamente para baixar a PSBT assinada.
- **Dispositivo:** consulte firmware/cofre/rede, peça exportação xpub ou ajuste de
  brilho. Exportação e configuração exigem confirmação física.

O cofre fica na NVS da placa, cifrado pelo PIN, e persiste ao desligar. Depois de
salvo, Criar e Importar desaparecem do menu. Ao reiniciar, aparece Desbloquear.
Substituir a carteira exige primeiro desbloquear e confirmar sua remoção na placa.
Anote e guarde as palavras: elas são a recuperação se a placa falhar.

A seed não é enviada ao navegador. Na versão 0.7, você pode digitar o PIN no painel
local; ele passa em memória pelo computador e USB, exigindo autorização física. O painel escuta somente em loopback,
sem CDN ou servidor externo, e valida Host, origem e token da sessão. Não acompanha
saldo ou confirmações na blockchain. O CLI e o painel não podem usar a mesma USB
simultaneamente. Pare com `systemctl --user stop openvault-panel`; reinicie com
`systemctl --user start openvault-panel`. Remova o início automático com
`systemctl --user disable --now openvault-panel`.

Testes do painel: `.venv/bin/python tests/test_web.py` e
`.venv/bin/python tests/test_companion.py`.

## Fluxo pelo painel e várias carteiras — 0.5.0

O painel agora inicia as operações. Em **Suas carteiras**, selecione um dos cinco
espaços e clique em **Trocar carteira**. Confirme o número na placa segurando A.
A troca bloqueia a carteira anterior e limpa endereço, xpub e PSBT exibidos.

Para adicionar uma carteira, selecione um espaço vazio, escolha rede e 12/24
palavras e clique em **Criar carteira**. Segure A na placa para autorizar. Anote
as palavras (B avança; segure A na última), defina e repita o PIN de 8 dígitos.
O painel acompanha cada etapa e só informa salva após a gravação do cofre.
Cada espaço tem sua própria seed, PIN, índice de recebimento e tentativas de PIN.
A carteira antiga permanece no espaço 1; a atualização preserva as chaves NVS.

Desbloquear, bloquear, ver backup e trocar PIN são iniciados na visão geral.
Para assinar, basta enviar a PSBT pelo painel: a revisão abre na placa, sem
selecionar Assinar PSBT no menu. A seed continua exclusivamente na placa; o PIN pode ser digitado no formulário
local da versão 0.7, com autorização física.
O menu físico permanece como alternativa. Na tela inicial, A abre esse menu.

**Enviar:** a versão atual revisa/assina uma PSBT preparada por outra carteira e
permite baixá-la; não prepara pagamentos a partir de saldo nem faz broadcast.
Não há consulta de saldo/histórico. O endereço confirmado serve para recebimento
na rede escolhida. Cada carteira deve ser desbloqueada antes de receber/assinar.

## Nome, exportação e PIN — 0.6.0

Na visão geral, selecione e desbloqueie a carteira. Em **Nome da carteira ativa**,
preencha até 18 caracteres (letras sem acento, números, espaço, hífen ou sublinhado),
clique em **Salvar nome na placa** e confirme segurando A. O nome persiste na NVS
separado por carteira e aparece no seletor, mesmo após reiniciar o computador.
Ele é apenas um rótulo: confira também número, fingerprint e endereço na placa.

Em Dispositivo, use **Autorizar exportação na placa**, confirme segurando A e
clique em **Baixar carteira para consulta (.json)**. O arquivo inclui nome, rede,
xpub, fingerprint, derivação BIP84, descritores de recebimento/troco e endereço
confirmado, se existir. Os descritores são sem checksum. O formato JSON é próprio
do OpenVault, não um formato de importação universal. A exportação não guarda
seed/PIN/chaves privadas, não assina e não recupera os fundos se a placa for perdida.
Para recuperação, guarde as palavras anotadas na criação.

Cada cofre exige PIN de 8 dígitos criado e repetido na etapa final de criação.
O site indica se já existe PIN e oferece **Trocar PIN / senha** para carteira
salva e desbloqueada. Digitação e confirmação são feitas na placa (B muda o dígito,
A confirma), ou pelo formulário web guiado da versão 0.7. O PIN é numérico; senha alfanumérica
não é implementada. O PIN não aparece em nenhuma exportação.

## Configuração guiada e PIN pelo painel — 0.7.0

Esta versão substitui as instruções anteriores que limitavam o PIN aos botões.
Clique em Criar carteira e autorize na placa. O cartão Configuração guiada aparece
no alto do painel com a etapa atual. Use Próxima palavra na placa para percorrer
as palavras no LCD. Na última, clique em Anotei as palavras — definir PIN. Você
também pode percorrê-las com B e segurar A na última, como alternativa física.

O formulário de PIN aparece nessa etapa: informe 8 números, repita e clique em
Enviar PIN para autorização. Segure A na placa em SALVAR PIN; aguarde Carteira
salva. Para desbloquear, clique em Desbloquear, preencha o PIN e autorize com A.
Trocar PIN usa o mesmo formulário após desbloquear a carteira. A criação expira
após 5 minutos sem interação; carteiras já salvas continuam bloqueando após 90s.

O PIN agora passa pelo navegador, servidor local e USB **em texto claro no
transporte**, sem registro pelo aplicativo em arquivo, log ou resposta de estado.
Os campos são apagados ao enviar e ao mudar de etapa. Use um computador confiável;
malware local pode capturar o PIN. As palavras continuam somente no LCD. O PIN
cifra o cofre com o formato anterior; trocar o fluxo não altera as seeds existentes.

Testes da interface guiada: `node tests/test_panel.js`.

## Proteção contra publicação de segredos

Dados privados ficam em backups/, private/, secrets/ ou companion-output/, todos
ignorados. Nunca versionar seed/PIN/chaves privadas, dumps da placa, exportações
(inclusive xpub), PSBTs ou backup cifrado. .gitignore não remove dados já rastreados.

Os hooks locais pre-commit/pre-push executam scripts/check_secrets.py e bloqueiam
arquivos/padrões suspeitos sem imprimir seus valores. Neste checkout estão ativos.
Após clonar em outro local: `git config --local core.hooksPath .githooks`.
Antes de publicar, rode `python3 scripts/check_secrets.py --history` e revise os
arquivos staged. O detector é heurístico e os hooks podem ser desabilitados: não
há garantia de detectar todo segredo. Se houver exposição, apagar o arquivo do
commit atual não remove o histórico; seeds expostas devem ser consideradas perdidas.
O dicionário BIP39 em scripts/data é público, com hash fixado no detector; licença
libwally em docs/licenses/libwally-LICENSE.txt. Vetores públicos de testes não são
carteiras para uso real.

## Excluir carteira pelo painel — 0.8.0

Requer firmware 0.8.0 instalado. Selecione e desbloqueie a carteira, abra a visão
geral e clique em Excluir carteira. O modal identifica nome, espaço e rede; leia
o aviso sobre recuperação e marque que compreende a necessidade das palavras.
Continuar na placa envia somente um pedido. Confira nome/número no LCD e segure
A para remover o cofre; B cancela. Manter carteira, X e Escape fecham o modal sem
enviar o pedido. O pedido é rejeitado se outra carteira estiver ativa ou bloqueada.

Somente o espaço ativo é removido; os outros permanecem. Excluir não movimenta
Bitcoin nem invalida a seed. Recuperar acesso depende das palavras, não do PIN ou
JSON de consulta. Remoção NVS não é apagamento físico seguro contra análise da flash.

## Iniciar automaticamente ao conectar USB (Linux)

Execute `./scripts/install-usb.sh` na sua sessão de usuário. O instalador configura
o serviço de usuário e solicita autenticação administrativa uma vez para instalar
`config/udev/70-openvault.rules`. A regra concede acesso ao usuário da sessão local
ativa e aciona openvault-panel.service ao conectar a ponte USB 1a86:55d4 do PLUS2.
Não utiliza permissão global 0666 nem executa o painel como root.

O painel continua apenas em http://127.0.0.1:8765 e também inicia no login. Se o
servidor já estiver rodando sem placa, ele detecta a chegada da USB; se a porta
mudar ao reconectar, ele a redescobre. Com várias placas compatíveis, selecione a
porta manualmente. Desconectar USB na interface desabilita a reconexão automática
até clicar em Conectar novamente. Desconectar o cabo não encerra o painel.
A concessão uaccess exige sessão local ativa; não cobre login SSH/headless.

Para remover a regra: `sudo rm /etc/udev/rules.d/70-openvault.rules` e
`sudo udevadm control --reload-rules`. O serviço pode ser desabilitado conforme
as instruções anteriores. Não coloque seeds/PINs em regras, argumentos ou unidades.
