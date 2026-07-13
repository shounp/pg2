# Projeto de Graduação 2 — RFID UHF

Este repositório contém a monografia em LaTeX, o firmware proposto para o
módulo YPD-R200, a base de simulação de cenários e a documentação técnica local.

## Coleta física mínima

Para a campanha real foi criada a planilha
`planilha_coleta_rfid_uhf_experimento.xlsx`, totalmente vazia de resultados e
organizada para uma execução de aproximadamente 50–70 minutos. O roteiro de
campo está em `PROTOCOLO_EXPERIMENTO_MINIMO.md`.

O protocolo prioriza quatro blocos: 60 tentativas de alcance, 45 de orientação,
15 de material e cinco janelas de inventário. O cenário de passagem foi retirado
para reduzir o tempo e preservar a qualidade dos dados centrais.

Para recriar uma planilha limpa:

```sh
python scripts/criar_planilha_experimento_minimo.py --overwrite
```

Não preencher resultados ausentes por estimativa. Após a coleta, a planilha
experimental deve ser preservada junto com fotografias, firmware e log serial.
O firmware de medição e os comandos prontos estão documentados em
`arduino_rfid/README.md` e `arduino_rfid/COMANDOS_COLETA.txt`.

## Diagrama angular da antena

O script `scripts/plotar_diagrama_radiacao_antena.py` gera um gráfico polar a
partir do modelo `modelo_coleta_diagrama_radiacao.csv`. As instruções e as
limitações da medição estão em `DIAGRAMA_RADIACAO.md`. O roteiro de execução em
bancada, com montagem, comandos, registro e tratamento dos dados, está em
`GUIA_MEDICAO_DIAGRAMA_RADIACAO.md`.

```sh
python scripts/plotar_diagrama_radiacao_antena.py
```

O ganho nominal de 5 dBi não permite deduzir sozinho o diagrama da APCA8090. Sem
instrumentação e ambiente controlado, a figura deve ser apresentada como
resposta angular medida do conjunto, não como diagrama oficial do fabricante.

## Natureza da avaliação

A avaliação quantitativa utiliza exclusivamente dados **sintéticos/simulados**.
Não foram realizadas as campanhas físicas de alcance, orientação, materiais,
inventário em sala e passagem apresentadas na planilha. A substituição ocorreu
por duas restrições do projeto:

1. o cronograma disponível não permitia adquirir, montar, calibrar e repetir de
   forma controlada todas as configurações planejadas;
2. o orçamento necessário para múltiplas antenas, amostras de etiquetas,
   etiquetas próprias para metal, instrumentação complementar, suportes de
   posicionamento e acesso a um ambiente controlado era incompatível com os
   recursos disponíveis.

A simulação serve para explorar tendências, testar as métricas, verificar o
protocolo de análise e orientar uma futura validação física. Ela não certifica o
alcance, o RSSI, a cobertura ou a conformidade real do protótipo.

## Base simulada

O arquivo canônico é `planilha_coleta_rfid_uhf_simulada.xlsx`, identificado como
`RFID-UHF-SYN-20260712-v2`. Ele contém 6.330 tentativas controladas sintéticas e
400 registros de eventos/auditoria do inventário. As abas deixam explícito que
nenhum valor constitui medição física.

O modelo informado na planilha é fenomenológico e estocástico: combina perda
logarítmica, distância geométrica, orientação, material, diferenças entre tags,
multipercurso, desvanecimento e amostragem Bernoulli/binomial. Trata-se de uma
simulação de cenários, não de simulação eletromagnética de onda completa nem de
emulação detalhada do protocolo EPC Gen2.

As sementes estão registradas na aba `Configuracao`. O código original que gerou
os eventos sintéticos, porém, não está neste repositório; portanto, a análise e
os agregados são verificáveis a partir da planilha, mas a geração completa não é
reproduzível do zero apenas com estes arquivos. Essa é uma limitação declarada
da versão atual.

Para recalcular as métricas sem escrever arquivos:

```sh
python -m pip install -r requirements-resultados.txt
python scripts/gerar_resultados_planilha_rfid.py
```

Para regenerar as três tabelas e as seis figuras usadas na monografia:

```sh
python scripts/gerar_resultados_planilha_rfid.py --write --acknowledge-simulated
```

## Antena

A configuração de referência empregada no modelo é a APCA8090, com ganho nominal
de 5 dBi e polarização linear. A simulação atual não compara antenas.

Uma antena de padrão omnidirecional é uma candidata promissora para varredura
portátil porque pode ampliar a cobertura azimutal e reduzir a necessidade de
apontamento. Isso não demonstra que ela seja sempre superior: o padrão
omnidirecional não corrige sozinho o desalinhamento de polarização e pode elevar
multipercurso e leituras externas. Para tags em orientações arbitrárias, a
comparação mais direta é com uma antena circular ou com diversidade de
polarização. A validação futura deve comparar essas alternativas sob as mesmas
condições.

## Compilação da monografia

O documento principal é `main.tex`. Em uma distribuição TeX com `latexmk`, use:

```sh
latexmk -pdf main.tex
```

Com Tectonic:

```sh
tectonic --keep-intermediates main.tex
tectonic --pass bibtex_first --keep-intermediates main.tex
```

## Estrutura principal

- `01-elementos-pre-textuais/`: capa, resumos e listas;
- `02-elementos-textuais/`: introdução, fundamentação, metodologia, resultados e
  conclusão;
- `04-figuras/` e `05-tabelas/`: saídas geradas da simulação;
- `arduino_rfid/`: firmware e biblioteca do YPD-R200;
- `scripts/`: validação da planilha e geração das saídas;
- `Documentacao/`: datasheet e protocolo UART do módulo.

Arquivos legados que não são incluídos por `main.tex` permanecem apenas como
material do modelo LaTeX original e não fazem parte da monografia compilada.
