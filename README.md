# Projeto de Graduação 2 - RFID UHF

Este repositório contém a monografia em LaTeX, o firmware do módulo YPD-R200,
os scripts de apoio, as tabelas e figuras consolidadas e a documentação técnica.
Os registros individuais da coleta foram mantidos pelo autor para organização e
conferência, mas não integram a versão pública do repositório.

## Coleta experimental

O roteiro de campo está em `PROTOCOLO_EXPERIMENTO_MINIMO.md`. Ele descreve a
montagem, as janelas de coleta e a ordem dos ensaios executados.

Os ensaios futuros com rotação entre tags e suportes, repetição em outro dia,
comparação real de antenas, verificação do adequador de nível e leitura posterior
da configuração de RF estão em `GUIA_CAMPANHA_COMPLEMENTAR_RFID.md`. Esses
protocolos ainda não integram os resultados apresentados na monografia.

Para criar, em uso local, um formulário tabular vazio com a estrutura da coleta,
use:

```sh
python scripts/criar_planilha_experimento_minimo.py --overwrite
```

As fotografias da montagem foram registradas como apoio documental, mas não foram inseridas na monografia. Elas podem ser mantidas separadamente, junto com o firmware e o log serial.

Durante a campanha, foram usadas tanto uma fonte externa regulada de 5 V/1 A
quanto a alimentação USB do Arduino. Após as medições, não foi observada
alteração relevante nos resultados em razão da forma de alimentação. Os
resultados consolidados, portanto, não são estratificados por fonte.

## Diagrama angular da antena

O arquivo `modelo_coleta_diagrama_radiacao.csv` é um modelo vazio para uma
eventual caracterização angular futura. O script
`scripts/plotar_diagrama_radiacao_antena.py` lê níveis logarítmicos em dB e pode
gravar uma figura polar em `04-figuras/diagrama_radiacao_antena.png`.

```sh
python scripts/plotar_diagrama_radiacao_antena.py --input modelo_coleta_diagrama_radiacao.csv --output 04-figuras/diagrama_radiacao_antena.png --title "Resposta angular relativa do conjunto RFID"
```

Esse ensaio opcional não foi empregado na análise da monografia. Sem câmara
anecoica e receptor calibrado, uma figura obtida com esse procedimento deve ser
tratada como resposta angular do conjunto no ambiente, e não como diagrama
oficial ou ganho absoluto da antena.

## Consolidação dos resultados

O script `scripts/gerar_resultados_planilha_rfid.py` lê, quando disponível
localmente, o registro tabular individual da campanha e confere as tabelas e
figuras usadas na monografia. O arquivo com as observações individuais não é
distribuído neste repositório.

Para validar sem escrever arquivos:

```sh
python -m pip install -r requirements-resultados.txt
python scripts/gerar_resultados_planilha_rfid.py
```

Para regenerar as tabelas e figuras:

```sh
python scripts/gerar_resultados_planilha_rfid.py --write
```

As saídas consolidadas incluem alcance por distância, tempo até a primeira leitura, orientação, material e inventário em sala.

## Antena

A APCA8090 de 5 dBi e polarização linear foi usada como referência nos ensaios. Os resultados mostraram boa leitura em curta distância, forte sensibilidade a metal direto e cobertura ainda limitada para varredura portátil ampla.

Uma antena omnidirecional pode ser uma alternativa interessante para ampliar a cobertura azimutal, mas a polarização continua sendo uma variável crítica. Para reduzir a dependência da orientação das tags, uma comparação futura com antena circular ou com diversidade de polarização também é recomendável.

## Compilação da monografia

O documento principal é `main.tex`. Em uma distribuição TeX com `latexmk`, use:

```sh
latexmk -pdf main.tex
```

Com MiKTeX sem `latexmk`, execute a sequência abaixo no diretório do projeto:

```sh
pdflatex -interaction=nonstopmode main.tex
bibtex main
pdflatex -interaction=nonstopmode main.tex
pdflatex -interaction=nonstopmode main.tex
```

Com Tectonic:

```sh
tectonic main.tex --keep-logs --keep-intermediates
```

O Tectonic executa automaticamente as passagens necessárias para bibliografia,
sumário e referências cruzadas. O PDF final é gravado como `main.pdf`.

## Estrutura principal

- `01-elementos-pre-textuais/`: capa, resumos e listas;
- `02-elementos-textuais/`: introdução, fundamentação, metodologia, resultados e conclusão;
- `04-figuras/` e `05-tabelas/`: saídas consolidadas da campanha experimental;
- `arduino_rfid/`: firmware e biblioteca do YPD-R200;
- `scripts/`: conferência local dos dados e geração das figuras;
- `Documentacao/`: datasheet e protocolo UART do módulo.

Arquivos legados de versões anteriores foram preservados apenas para histórico e não fazem parte do texto principal da monografia.
