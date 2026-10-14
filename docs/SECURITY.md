# Modelo de segurança e limites — 0.2.0

Esta versão implementa operações de carteira; não é uma carteira auditada para
custódia de fundos. Testes funcionais não demonstram resistência a invasão física,
side channels ou firmware malicioso.

## Chaves e entropia

BIP39 inglês (12/24 palavras), passphrase vazia, BIP32 e BIP84 são realizados pela
libwally-core fixada em `374df231d05410a31d605b31c1cc5256581381d7` e seu secp256k1-zkp.
A fonte de entropia não-RF é habilitada com `bootloader_random_enable`, usada por
`esp_fill_random` e desligada após a coleta. Não há operação concorrente de rádio,
microfone, alto-falante, I2S ou leitura de bateria ADC durante essa coleta. Wi-Fi e
Bluetooth são desligados no boot. Isso não remove fisicamente os rádios.

Os buffers próprios de entropia, chaves estendidas, PIN e frase de importação são
sobrescritos ao encerrar seu uso. Isso é uma medida de redução de exposição, não
uma prova de que não exista cópia transitória em stack, bibliotecas ou hardware.
Não há canal USB para importar/exportar seed ou PIN; recuperação é feita na tela.

## Cofre

Um único blob NVS `openvault/vault`, de 120 bytes, contém:

- versão `OVW1` e custo KDF fixado;
- salt aleatório de 16 bytes e IV aleatório de 16 bytes;
- rede, tamanho da entropia e entropia protegidos por AES-256-CBC/PKCS#7;
- HMAC-SHA256 de todo o cabeçalho e ciphertext (Encrypt-then-MAC).

PBKDF2-HMAC-SHA512, 100.000 iterações, produz 64 bytes divididos em chaves de
cifragem e autenticação independentes. O MAC é conferido antes de decifrar.
O PIN tem 8 dígitos e deve ser confirmado ao salvar. Erros de autenticação não
revelam se a falha veio de PIN ou adulteração. Há espera após cinco erros, com
contador NVS preservado após reinício.

Um atacante com acesso à flash pode contornar limites de tentativas de firmware
e testar PINs offline. Oito dígitos não substituem um segredo de alta entropia.
Secure boot, flash encryption, anti-rollback e bloqueio de JTAG **não estão ativados**.
Não há secure element. O backup integral original também não contém eFuses.
Remover o cofre não apaga seguramente cópias antigas deixadas pelo wear leveling
NVS; a seed de uma carteira apagada deve ser considerada possivelmente recuperável
por análise física da flash. Nenhum apagamento de eFuses é feito por este projeto.

## Assinatura

A política aceita apenas PSBT v0 com entradas P2WPKH, SIGHASH_ALL e caminhos BIP84
da própria conta 0. A transação anterior completa é exigida e seu txid é conferido;
o script deve coincidir com a chave derivada. Um witness_utxo divergente é rejeitado.
Quando ausente, o witness_utxo é reconstruído a partir da transação anterior validada
para que o cálculo de assinatura utilize BIP143.

Saídas desconhecidas são rejeitadas. Troco só é reconhecido após derivar a chave
e comparar o script localmente. A taxa é calculada por entradas menos saídas;
underflow, valores fora da faixa e taxas acima da política são rejeitados.
Todos os destinatários, valores, rede, taxa, versão e locktime passam pela revisão
na tela. A confirmação exige pressionar e segurar A em uma tela separada.
Não há assinatura automática por comando USB nem transmissão de transações.

O computador pode negar serviço, apresentar informações falsas ou enviar PSBTs
maliciosas; a proteção depende de conferir **todos os caracteres** dos endereços
na tela confiável. A revisão não prova inclusão dos UTXOs na cadeia nem saldo atual.
O arquivo em revisão é mantido separado e não pode ser substituído pela USB durante
a confirmação. Há limites de tamanho, entradas, saídas, caminhos e índices.

## Ainda fora do escopo implementado

Câmera/microSD, QR multipartes de PSBT, multisig, Taproot, PSBT v2, passphrase BIP39,
autenticidade verificável do firmware e auditoria independente. Não confiar fundos
com base somente no sucesso dos testes automatizados ou na descrição dos recursos.

## Painel local — 0.3.0

Servidor limitado a 127.0.0.1. A API exige token aleatório por execução e valida
Host/Origin/Sec-Fetch-Site; CSP local, sem dependências remotas, sem framing.
A lista de arquivos estáticos é fechada. Não existem rotas de seed, PIN, criação,
importação ou remoção do cofre. Dados públicos/PSBTs são mantidos em RAM até
reiniciar/desconectar; exportações só são gravadas quando o usuário baixa arquivos.
Endereço/xpub e mudanças de brilho exigem botões da placa. A assinatura mantém
as mesmas validações e revisão física do protocolo USB. Um processo malicioso no
mesmo computador ainda pode acessar a USB/painel; loopback não protege contra
comprometimento local. A xpub revela a conta e deve ser tratada como dado privado.

## Atualização 0.5: controles web e isolamento

A criação pode ser solicitada pelo HTTP, mas só gera entropia após autorização
física. Criação não substitui um cofre existente. Selecionar outra carteira exige
confirmação na placa, cancela a chave em memória e exige o PIN da carteira escolhida.
A ponte remove dados públicos/assinatura da carteira anterior. Pedidos durante
backup, PIN, criação ou revisão são rejeitados; não podem trocar o espaço usado
por uma operação em andamento. Cada espaço tem blob e contador de tentativas
independentes. O espaço original usa as mesmas chaves persistentes da versão 0.3.
A API continua sem rota que aceite ou revele seed/PIN. Backup iniciado no painel
é exibição das palavras no LCD, não exportação de segredo ao computador.
