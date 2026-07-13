# Diagrama de radiação da antena

O ganho nominal de 5 dBi e a polarização linear não são suficientes para
reconstruir o diagrama da APCA8090. O script apenas plota níveis angulares
medidos ou gera um exemplo explicitamente ilustrativo.

## Plotar dados medidos

Preencha `modelo_coleta_diagrama_radiacao.csv` com níveis em dB. Pode ser potência
recebida em dBm ou outro nível logarítmico calibrado, pois cada plano será
normalizado pelo próprio máximo.

```sh
python scripts/plotar_diagrama_radiacao_antena.py
```

Saída padrão:

```text
04-figuras/diagrama_radiacao_antena.png
```

Também é possível informar outros arquivos:

```sh
python scripts/plotar_diagrama_radiacao_antena.py \
  --input minha_medicao.csv \
  --output 04-figuras/diagrama_medido.png \
  --title "Resposta angular medida do conjunto"
```

O CSV aceita vírgula ou ponto e vírgula como separador e exige as colunas:

```text
plano,angulo_graus,nivel_db
```

Cada plano precisa de pelo menos três ângulos. Para uma curva rápida, o modelo
fornecido usa passos de 30°. Passos de 10° ou 15° produzem uma caracterização
mais detalhada, mas exigem mais tempo.

## Medição aproximada do conjunto

Sem câmara anecoica e receptor calibrado, o resultado deve ser chamado de
**resposta angular medida no ambiente**, não de diagrama oficial da antena.

1. Mantenha distância, altura, potência, cabo e posição do receptor constantes.
2. Use uma tag ou receptor de referência sem mudar sua orientação.
3. Gire somente a antena sob teste em torno do seu centro.
4. Registre várias leituras em cada ângulo e use a média.
5. Meça 0° como a direção frontal e prossiga até 330°.
6. Repita em outro plano somente se houver tempo e suporte mecânico adequado.
7. Registre ambiente, raio, altura, instrumento, quantidade de amostras e data.

Girar a tag em 0°, 45° e 90° mede principalmente a combinação entre polarização,
tag e ambiente; isso não é suficiente para obter o diagrama de radiação da
antena do leitor.

O byte de RSSI bruto do YPD-R200 não deve ser colocado na coluna `nivel_db` sem
uma conversão documentada. Caso não exista medição em dB, preserve os valores
brutos separadamente e apresente apenas uma resposta angular relativa, deixando
claro que a escala não foi calibrada.

## Testar o script sem medições

```sh
python scripts/plotar_diagrama_radiacao_antena.py \
  --example \
  --output exemplo_diagrama.png
```

O título da figura informa que as curvas são ilustrativas e não representam a
APCA8090. Não use essa figura como resultado experimental.
