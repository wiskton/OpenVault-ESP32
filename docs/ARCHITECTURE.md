# Arquitetura 0.2

`Wallet` é o núcleo portátil C++: usa libwally para BIP39/BIP32/BIP84, endereços,
AES/HMAC/PBKDF2 e PSBT. Entropia vem de fora e persistência não faz parte do núcleo.
A interface recebe somente operações específicas; não há método de exportar chave
privada ou frase inteira pela USB.

`src/main.cpp` adapta botões, framebuffer 240 × 135 RGB565, fonte padrão em escala 2,
fonte maior para palavras, NVS e geração de entropia na plataforma. As palavras
são exibidas individualmente. PIN é digitado com B (incrementar) e A (fixar).
Conteúdo longo é dividido em páginas; menus mostram três opções por vez.

O fluxo de criação mantém uma carteira volátil até percorrer as palavras e confirmar
o PIN duas vezes. O cofre é salvo em um único blob NVS com autenticação do cabeçalho.
Reinício inicia com a carteira bloqueada. Há somente uma carteira persistente por vez;
criação/importação não sobrescreve um cofre existente silenciosamente.

O parser libwally fornece a PSBT. A política própria confere propriedade dos inputs,
UTXOs completos, valores, scripts, derivação e taxa antes de montar as páginas. A PSBT
aceita permanece congelada em `pending_` até uma confirmação física; cancelar ou
bloquear a carteira a descarta. Os dados derivados do host não se tornam rótulos de
troco confiáveis sem verificar caminho e script.

Protocolo UART delimitado por newline:

- `PING` → versão pública;
- `STATUS` → estado saved/empty e unlocked/locked;
- `SIGN <base64>` → permitido apenas na tela Assinar PSBT;
- `REVIEW_READY`, `SIGNED <base64>`, `ERROR ...`, `CANCELED`, `LOCKED timeout`;
- `XPUB <fingerprint> <origem> <xpub>` só sai após autorização pelos botões.

Não há comandos PIN, seed, criação ou desbloqueio. O companion mantém framing entre
fragmentos de serial e nunca trata um pedaço de resposta como PSBT completa.

Build: upstream libwally CMake, Bitcoin-only, secp256k1 com tabelas reduzidas para
memória embarcada. Cabeçalhos públicos e biblioteca são da mesma revisão. Testes
nativos usam o mesmo src/wallet.cpp; Python libwally é usado como referência separada
para verificar a assinatura ECDSA e finalizar a transação de teste.

## Painel 0.3

companion/web_server.py é uma ponte HTTP loopback/USB com conexão serial exclusiva,
framing de linhas e estado em RAM. companion/web contém HTML/CSS/JS locais.
ADDRESS e REQUEST_XPUB iniciam confirmação física; BRIGHTNESS 40..200 também.
STATUS informa versão via PING, existência/bloqueio, rede/fingerprint quando
já desbloqueada e brilho. Nenhuma rota HTTP ou comando USB recebe seed/PIN.

## Controles e múltiplas carteiras 0.5

CREATE BTC|TEST 12|24 apenas inicia ConfirmCreate em espaço vazio/ocioso. Geração
requer hold A; seed e PIN passam pelo fluxo local existente. UNLOCK, LOCK, BACKUP
e CHANGE_PIN iniciam fluxos locais sem transportar segredos. SIGN aceita pedido
em tela ociosa desbloqueada e inicia revisão, mantendo as validações do núcleo.
SELECT 0..4 entra em ConfirmSwitch; hold A limpa a chave em RAM e carrega o espaço.

STATUS acrescenta wallet:0..4, slots:cinco bits e flow:etapa pública. A ponte limpa
endereço/xpub/PSBT ao trocar de espaço. Não inclui seed, PIN ou palavras nos estados.
WALLET_SAVED, WALLET_SELECTED, UNLOCKED, BACKUP_DONE e CANCELED concluem pedidos.

Espaço 0 preserva chaves NVS vault/receive/attempts; espaços 1..4 usam vaultN,
receiveN e attemptsN. active guarda só o identificador selecionado. Não há migração
nem apagamento dos cofres anteriores. Uma única Wallet permanece em RAM.
