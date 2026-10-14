# Arquitetura do primeiro protótipo

`App` recebe ações `Select`, `Next` e `Back`, altera um estado e fornece um `View`.
Ela não acessa flash, rede, USB ou criptografia. O firmware traduz botões M5Unified
nessas ações e desenha o resultado em um framebuffer RGB565 de 240 × 135 (~65 KB).
O simulador de terminal faz a mesma tradução para comandos digitados.

Não há heap na máquina de estados. Os textos têm até 37 caracteres por linha na
fonte padrão; testes verificam esse limite. A seleção circula por seis itens.
O retorno preserva o índice selecionado. Um botão B longo não gera um avanço
adicional ao ser solto. O boot exige reconhecimento do aviso com A.

O diagnóstico redesenha a cada segundo para atualizar bateria; demais telas só
redesenham ao navegar. O laço usa `M5.update()` e uma espera curta de 10 ms.
Não há entrada serial de seeds ou de comandos de assinatura. A única mensagem
serial própria descreve a versão e o caráter demonstrativo.

As futuras camadas de carteira, armazenamento e PSBT deverão ficar separadas da UI,
com modelos de revisão de transações que não dependam de strings fornecidas pelo host.
