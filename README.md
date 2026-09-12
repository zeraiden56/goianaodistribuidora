# Goianão Distribuidora

Jogo em C++17, SDL2 e OpenGL: gerencie uma distribuidora brasileira em primeira pessoa, com cenário de poucos polígonos e visual inspirado na era PS2. Modelos e fonte são gerados pelo código, sem downloads de assets.

## Compilar e executar no Linux

Requer compilador com suporte a C++17, CMake 3.16 ou superior, pkg-config e bibliotecas de desenvolvimento SDL2 e OpenGL.

Para obter o código:

```sh
git clone https://github.com/zeraiden56/goianaodistribuidora.git
cd goianaodistribuidora
```

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
./build/distribuidora
```

`Debug` inclui símbolos para usar breakpoints no GDB ou no editor. Para uma compilação otimizada, use `-DCMAKE_BUILD_TYPE=Release`.

## Menus e opções

O menu inicial permite **Continuar**, **Novo jogo**, **Opções** ou **Sair para o desktop**. Novo jogo pede confirmação antes de substituir um save existente. Use mouse e clique ou setas e Enter.

**Esc**, dentro do jogo, abre a pausa: retomar, salvar, opções, salvar e voltar ao menu ou salvar e sair. O tempo fica parado nos menus, inclusive nas opções. Perder o foco da janela também pausa o jogo.

As opções são aplicadas imediatamente e guardadas em arquivo:

- Tela cheia sem bordas na resolução do monitor; **F11** também alterna esse modo.
- Resolução em janela: 960×540, 1280×720, 1600×900 ou 1920×1080. Em tela cheia, essa escolha é usada ao voltar para janela.
- Visual nativo, retrô em metade ou em um terço da resolução. O cenário fica pixelado e a interface mantém a legibilidade.
- VSync, quando suportado pelo driver.
- Campo de visão entre 60 e 100 graus.
- Volume dos efeitos de 0 (mudo) a 100. Configurações antigas continuam válidas e começam com volume 70.

Nas opções, clique/Enter alterna os valores; setas esquerda/direita ajustam a opção selecionada. Se o driver recusar tela cheia ou VSync, uma mensagem informa a falha.

## Salvamento

Há um slot, `progress.save`, e um arquivo separado de opções, `options.cfg`. Eles ficam na pasta de dados do usuário determinada por SDL. No Linux, normalmente:

```text
~/.local/share/DistribuidoraSimulator/DistribuidoraSimulator/
```

O jogo salva ao iniciar uma nova partida, a cada 30 segundos de jogo ativo, pelo menu de pausa, ao voltar ao menu principal e ao sair normalmente, inclusive pelo botão de fechar da janela. Se a gravação falhar, a saída é interrompida e o erro aparece no menu.

São preservados caixa, estoque, melhorias, dia, reputação, pedido parcial, produto na mão, encomenda e seu tempo restante, posição e câmera, efeitos de consumo e sequência dos próximos clientes. Arquivos inválidos ou incompatíveis são recusados sem alterar a partida em memória. A gravação usa um arquivo temporário antes de substituir o save anterior. Não há múltiplos slots ou histórico de backups.

Para usar outra pasta de dados:

```sh
./build/distribuidora --data-dir /tmp/minha-distribuidora
```

## Jogar

- **WASD** anda; segure **Shift** (esquerdo ou direito) para correr; **mouse** olha; **E** interage; **R** consome; **Esc** abre a pausa; **F11** alterna tela cheia.
- Leia o pedido no painel à direita. Cerveja e destilado ficam em **geladeiras com portas de vidro**, no fundo. **Cigarros ficam no nicho embaixo do balcão**, à direita da abertura de atendimento; gelo continua no **FREEZER**, à esquerda. Aproxime-se, olhe para o produto e pressione **E** para pegar uma unidade. As portas das geladeiras abrem brevemente durante a retirada/devolução e fecham automaticamente.
- As placas e o HUD mostram **quantidade no estoque/capacidade máxima**: cerveja 48, cigarro 36, destilado 24 e gelo 36.
- Você carrega uma unidade por vez, visível na mão. Para devolver, olhe para o local de origem e pressione **E** novamente.
- Leve cada unidade à abertura retangular na grade, à esquerda do balcão, junto ao caixa. Olhe para a abertura e pressione **E** para entregar. O pagamento ocorre quando o pedido está completo.
- Produtos errados são recusados. Se o cliente desistir, as unidades do balcão voltam ao estoque; o produto na sua mão continua com você.
- No computador à direita, **E** abre/fecha o fornecedor. **1–4** encomendam o lote selecionado (inicialmente 12 unidades), pagas na hora e entregues automaticamente após 12 segundos. Há uma entrega por vez. Produtos na mão, no balcão e a caminho reservam espaço, para que devoluções não ultrapassem a capacidade.
- No computador, **U** melhora o nível da loja, aumenta os pedidos até cinco unidades e melhora o lucro. Após ampliar a loja, o botão **Grade + Caixa 2** abre outra passagem de atendimento. A compra não altera o estoque.
- O tempo continua no fornecedor. **Esc** fecha o fornecedor e abre a pausa.

Você começa com R$ 250 e estoque inicial. Clientes esperam 65 segundos. Vendas recuperam reputação e desistências a reduzem. Cada dia dura três minutos. A reputação é um indicador nesta versão. O espaço jogável é o interior da loja.

## Beber e fumar

Com cerveja ou destilado na mão, **R** bebe; com cigarro, **R** fuma. A unidade já retirada do estoque é consumida, sem receita de venda. Gelo não pode ser consumido.

Há uma animação simples de dois segundos. Nesse intervalo não é possível pegar outro produto. Bebidas causam oscilação da câmera, desfoque progressivo da visão e redução temporária da velocidade, inclusive ao correr. Quanto maior a embriaguez, maior o raio do desfoque; o efeito desaparece conforme o indicador diminui. Os textos e menus continuam nítidos. O desfoque também funciona nos modos gráficos retrô. Cigarros produzem um efeito visual de fumaça, sem bônus de desempenho. Esses efeitos e o total consumido entram no save.

## Computador e interface

O computador usa um desktop simplificado inspirado no Windows 98, com fundo verde-azulado, barra de título azul, botões em relevo e barra de tarefas. A tabela mostra os produtos, o estoque e o preço total do lote selecionado: 12, 24 ou 48 unidades.

- **Mouse:** clique na linha do produto para encomendar, ou nos botões de câmeras e melhorias. **X** e **Fechar** encerram o computador.
- **Teclado:** setas ou **Tab** mudam a seleção; **Enter** ou **Espaço** confirma. Os atalhos **1–4**, **C**, **U**, **B**, **H** e **E** continuam disponíveis.
- Ações indisponíveis ficam acinzentadas. Ao selecionar uma delas, a barra inferior explica se falta saldo, espaço ou se existe uma entrega em andamento. O clique não cobra nada nessas situações.
- O saldo, o tempo de entrega, o bônus de lucro e os quatro preços de venda (na ordem da tabela) ficam visíveis. O jogo segue rodando no computador; **Esc** abre a pausa.

O HUD possui painéis compactos: estoque com indicadores de capacidade e estoque baixo; pedido com nome do cliente, unidades entregues e barra de paciência; situação do auxiliar; produto na mão e dica contextual da ação. O tempo do cliente fica destacado quando está acabando.

Há **oito aparências de clientes**, com nomes, roupas, alturas, cabelos, tons de pele e acessórios diferentes. A seleção varia a cada chegada, evitando repetir a mesma aparência consecutivamente. A aparência do cliente atual é salva; saves das versões anteriores continuam compatíveis.

## Expansão, auxiliar e descanso

No computador:

- **B — Ampliar loja: R$ 600.** Abre uma passagem ao lado das geladeiras, à direita, para um novo cômodo nos fundos. A expansão inclui depósito, sofá e TV. A capacidade total dobra para cerveja 96, cigarro 72, destilado 48 e gelo 72. A compra **não repõe mercadorias**.
- **H — Contratar atendente: R$ 350 por funcionário.** O primeiro está disponível antes da expansão. Depois de abrir o segundo caixa, o mesmo botão contrata o segundo funcionário. Cada um atende seu próprio caixa. Não há salário diário.

O auxiliar anda até a geladeira, o nicho de cigarros ou o freezer, pega até a capacidade da sacola (e somente o que falta no pedido) e leva ao seu caixa. Ele completa pedidos e recebe o pagamento automaticamente. Se você terminar um pedido enquanto ele busca outra unidade, ou se o cliente desistir, ele devolve o produto. Sem estoque, ele espera reposição: as encomendas continuam sendo sua responsabilidade. Seu estado aparece no HUD e ele é visível no cenário e nas câmeras. Ele não bloqueia a passagem do jogador.

As prateleiras do depósito são interativas: aproxime-se, olhe para o produto e pressione **E** para pegar ou devolver. O depósito e a área de vendas compartilham o mesmo inventário; as caixas nos fundos representam mercadorias existentes, sem criar estoque extra.

Aproxime-se do sofá nos fundos e pressione **E** para sentar. Você pode olhar ao redor com o mouse; **E** levanta e **T** liga/desliga a TV. A televisão exibe uma animação original de futebol, sem áudio de programa. Também pode ser ligada/desligada com **E** ao se aproximar da tela. Enquanto você descansa, clientes, encomendas e auxiliar continuam ativos. **Esc** pausa tudo normalmente.

A expansão, os dois caixas e seus pedidos, os atendentes com as unidades nas sacolas, a encomenda com sua quantidade original, a TV e sua posição sentado entram no save versão 4. Saves das versões 1, 2 e 3 continuam carregando; o caminho de dados foi mantido para preservar partidas anteriores à mudança de nome.

## Atacado, lucro e segundo caixa

Todas as compras são feitas no computador, por mouse ou setas/Tab e Enter:

- **Lote:** após a ampliação, clique para alternar 12 → 24 → 48 unidades. Caixas de 24 têm desconto de 10%; engradados/caixas de 48 têm desconto de 20% sobre o custo normal. O total é arredondado para cima em reais inteiros. Por exemplo, cerveja custa R$ 48 por 12, R$ 87 por 24 e R$ 154 por 48. É preciso espaço para o lote inteiro, incluindo mercadorias reservadas. Trocar o seletor não muda a entrega já paga.
- **Grade + Caixa 2 — R$ 500:** exige loja ampliada. Abre outra janela retangular na grade, à direita, e instala o segundo caixa, sem repor produtos. O novo cliente tem pedido e paciência independentes. Você também pode entregar ali com **E**.
- **Segundo atendente — R$ 350:** use novamente o botão de contratação depois de abrir o caixa 2.
- **Sacolas:** exigem o primeiro atendente. Cada compra acrescenta uma unidade à capacidade dos dois funcionários: 2 por R$ 100, 3 por R$ 200, 4 por R$ 300 e 5 por R$ 400. A capacidade vale também para o segundo funcionário contratado depois. O jogador continua carregando uma unidade na mão.

O bônus sobre o lucro base aumenta **2 pontos percentuais por dia** e **5 por nível**, até 100%. O preço de venda é `preço base + arredondamento(lucro base × bônus / 100)`, com lucro base igual a preço base menos custo normal. Como os pagamentos usam reais inteiros, alguns aumentos aparecem no preço somente após acumular bônus suficiente. O desconto do atacado aumenta o lucro adicionalmente. O computador mostra os preços atuais; o pagamento usa o preço vigente ao concluir o pedido.

## Rua noturna e câmeras

A parte externa permanece à noite, com céu escuro, postes, janelas iluminadas, carros e motos passando em duas faixas. Faróis, lanternas e luzes no chão são efeitos estilizados, sem simulação física de iluminação. A loja permanece iluminada. O tráfego usa o relógio da partida, continua ao observar as câmeras e para quando o jogo é pausado.

No computador, pressione **C** para acessar o sistema de vigilância:

- **1**: interior da loja, visto do canto superior.
- **2**: fachada e rua, vista da frente da loja.
- **3**: rua e entrada, vista do outro lado da rua.
- **Setas esquerda/direita**: alternar entre as câmeras.
- **C** ou **E**: voltar ao fornecedor. **E** novamente fecha o computador.
- **Esc**: sair dos monitores e abrir o menu de pausa.

As imagens são renderizadas ao vivo, mostrando estoque, cliente, gerente e veículos conforme o ponto de vista. Enquanto as câmeras estiverem abertas, você fica parado no computador, mas os clientes continuam esperando e as encomendas continuam chegando. As teclas **1–3**, nos monitores, só mudam a câmera; não compram mercadorias. Ao carregar uma partida, você volta à visão em primeira pessoa na posição salva.

## Efeitos sonoros

Os efeitos são sintetizados em C++ e reproduzidos com SDL2, sem arquivos externos ou dependências adicionais. Há sons de:

- Passos suaves no piso, com volume reduzido, menos ruído agudo e cadência baseada na distância percorrida. A cadência aumenta naturalmente ao correr. Ficar parado ou andar contra uma parede não produz passos.
- Abrir a bebida e beber, sincronizado ao uso de **R**.
- Acionar o isqueiro, acender o cigarro, tragar e soltar a fumaça, em sequência ao usar **R** com cigarro.
- Dinheiro no caixa ou aprovação da máquina de cartão, sorteados ao concluir cada pedido. A mensagem informa a forma de pagamento; ambas acrescentam o mesmo valor ao saldo.
- Ligar/abrir o computador com **E**, pegar/devolver/entregar produtos e receber encomendas.

Os sons podem tocar simultaneamente e pausam com o jogo. Voltar ao menu ou carregar outra partida limpa os sons antigos. O áudio de uma ação não é recomeçado ao carregar um save durante sua animação. Em **Opções → Volume dos efeitos**, ajuste com esquerda/direita; zero silencia tudo. Se não houver dispositivo de áudio disponível, o jogo avisa e continua funcionando sem som.

## Organização do código

| Arquivo | Responsabilidade |
| --- | --- |
| `src/main.cpp` | Argumentos e entrada do programa |
| `src/app.cpp` | Janela, eventos, ciclo do jogo e ações dos menus |
| `src/game.cpp` / `.hpp` | Economia, estoque, clientes, melhorias e consumo |
| `src/helper.cpp` / `.hpp` | Auxiliar, rotas, coleta e atendimento automático |
| `src/interior.cpp` / `.hpp` | Depósito, sofá, TV animada e modelo do auxiliar |
| `src/player.cpp` / `.hpp` | Posição, caminhada/corrida, colisão, visão e seleção de objetos |
| `src/world.cpp` / `.hpp` | Rotas de tráfego e posições das câmeras |
| `src/world_render.cpp` / `.hpp` | Rua noturna, veículos e modelos de vigilância |
| `src/postprocess.cpp` / `.hpp` | Desfoque progressivo e apresentação do modo retrô |
| `src/render.cpp` / `.hpp` | Cenário, placas, fonte e composição de imagem |
| `src/hud.cpp` | Painéis, avisos e dicas de interação |
| `src/computer.cpp` / `.hpp`, `src/computer_render.cpp` | Ações, seleção, mouse e desktop retrô |
| `src/shop_layout.hpp`, `src/fixtures.cpp` / `.hpp` | Pontos de coleta, geladeiras de vidro e nicho do balcão |
| `src/customers.cpp` / `.hpp`, `src/customers_render.cpp` | Aparências e modelos dos clientes |
| `src/menu.cpp` / `.hpp` | Layout, navegação e textos dos menus |
| `src/persistence.cpp` / `.hpp` | Validação, leitura e gravação de save e opções |
| `src/settings.hpp` | Configurações gráficas, volume e resoluções |
| `src/audio.cpp` / `.hpp` | Síntese dos efeitos, mistura de sons e dispositivo SDL |
| `tests/audio_tests.cpp` | Sinais de áudio e reprodução com dispositivo simulado |
| `tests/tests.cpp` | Testes de regras e persistência, sem janela |

## Validação

```sh
ctest --test-dir build --output-on-failure
```

Os testes de áudio verificam sinais não silenciosos, limites de amplitude, sons distintos, reprodução simultânea, pausa, volume e encerramento seguro usando um dispositivo simulado. Os demais testes cobrem vendas parciais, devoluções, falta de saldo/estoque, capacidade com reservas, consumo, expiração dos efeitos, salvamento e retomada, saves truncados/inválidos e falhas de gravação.

O teste gráfico percorre menus, novo jogo, consumo, salvar/continuar, cancelamento de novo jogo, opções, resolução, tela cheia, câmeras, compras de melhorias, depósito, sofá, TV, carregamento sentado e saída. Também verifica que o antigo comando de grade não altera saldo/estoque e que trocar câmeras não faz encomendas. Os testes também cobrem lotes de 24/48 unidades, descontos, lucro por dia e nível, dois atendimentos simultâneos, sacolas, devolução de sacolas cheias e migração dos saves anteriores. Os testes de regras verificam corrida, colisão, progressão do desfoque, tráfego, expansão sem reposição gratuita, acesso aos fundos, sentar/levantar, atendimento automático dos quatro produtos, devoluções, cooperação com o jogador e salvamento/migração do auxiliar, melhorias e aparência do cliente. Também verifica pedidos por clique e Enter, ações bloqueadas, portas das geladeiras e as novas rotas do auxiliar. Renderiza os modos nativo e retrô, verifica erros OpenGL e grava capturas BMP das câmeras e da visão sóbria/embriagada na pasta temporária:

```sh
SDL_AUDIODRIVER=dummy SDL_VIDEODRIVER=offscreen ./build/distribuidora --smoke --data-dir "$(mktemp -d /tmp/distribuidora-smoke-XXXXXX)"
```

Esse modo exige suporte a OpenGL offscreen. O comportamento de tela cheia e VSync depende do driver; valide também na sessão gráfica real. Use sempre uma pasta descartável com `--smoke`, pois ele cria e substitui um save de teste.

## Escopo atual

Protótipo em desenvolvimento, atualmente validado no Linux. Ainda sem música, gravações realistas de áudio, assets finais ou NPCs com animações detalhadas. Há uma expansão física comprável, até dois caixas com clientes independentes e dois atendentes com sacolas de até cinco unidades. O jogador também pode atender manualmente. As encomendas chegam automaticamente, com lotes maiores e descontos após a expansão.
