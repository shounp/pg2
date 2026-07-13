# Guia rápido para medir a resposta angular da antena

Este roteiro permite obter uma **resposta angular relativa do conjunto leitor,
antena e tag**. Sem câmara anecoica, posicionador calibrado e analisador de
espectro, o resultado não deve ser apresentado como diagrama oficial ou ganho
absoluto da antena.

## Materiais

- leitor YPD-R200, Arduino e firmware da pasta `arduino_rfid/`;
- antena APCA8090, cabo e fonte usados nos demais ensaios;
- uma única tag RFID de referência;
- trena, fita adesiva e suporte não metálico;
- transferidor impresso ou base marcada de 30° em 30°;
- computador para registrar o log serial.

## Configuração recomendada

Use somente o plano azimutal para a campanha rápida. Serão 12 ângulos, três
janelas por ângulo e 36 janelas no total.

| Parâmetro | Valor recomendado |
|---|---:|
| Distância entre antena e tag | 2,0 m |
| Altura dos centros | mesma altura |
| Passo angular | 30° |
| Repetições por ângulo | 3 |
| Duração de cada janela | 3 s |
| Potência do leitor | fixa durante todo o ensaio |

Se a tag não for lida de frente a 2,0 m, reduza a distância para 1,5 m e
registre essa alteração. Não mude a distância depois de iniciar a sequência.

## Montagem

1. Escolha o local mais aberto disponível e afaste objetos metálicos, paredes e
   pessoas da linha entre antena e tag.
2. Fixe a tag em suporte não metálico, na altura do centro da antena.
3. Marque no chão o centro de rotação da antena e a posição da tag.
4. Defina 0° quando a face frontal da antena aponta diretamente para a tag.
5. Mantenha a tag, o cabo, a potência, a altura e a distância imóveis.
6. Fotografe a montagem e anote data, local, distância, altura, potência, tag e
   obstáculos próximos.

Gire a antena em torno do próprio centro. Não caminhe com a antena ao redor da
tag e não gire a tag durante este ensaio.

## Sequência de medição

1. Ligue o sistema e aguarde aproximadamente dois minutos.
2. Configure a potência desejada com `POWER,2600` (26,00 dBm nominais enviados
   ao módulo) e confira a configuração com `INFO`.
3. Cadastre a tag de referência como tag 1 com `TAG,1,<EPC_HEX>`.
4. No ângulo 0°, execute `RUN,DIAGRAMA,0graus,1,3`. O último campo solicita três
   repetições; cada repetição usa a janela fixa de três segundos do firmware.
5. Salve as três linhas `RESULT` produzidas pelo monitor serial.
6. Gire somente a antena para 30° e execute
   `RUN,DIAGRAMA,30graus,1,3`.
7. Prossiga por 60°, 90°, ..., 330°.
8. Ao terminar, repita 0°. Uma diferença grande em relação ao valor inicial
   indica deslocamento da montagem ou mudança relevante no ambiente.

Afaste-se da montagem antes de cada execução. Se outra pessoa entrar no local,
aguarde e repita aquela janela.

## Registro e cálculo

Para cada ângulo, preserve os três resultados individuais. Calcule a média
somente das leituras válidas do indicador de RSSI. Registre também quantas
janelas não produziram leitura.

O byte de RSSI fornecido pelo YPD-R200 não deve receber a unidade dBm sem uma
equação de calibração documentada pelo fabricante. Se o firmware fornecer apenas
o valor bruto, use-o como `nivel_db` somente quando sua conversão para escala
logarítmica estiver comprovada. Caso contrário, adapte uma cópia do CSV para
registrar `nivel_relativo` e descreva o gráfico como resposta angular em unidades
arbitrárias. Não invente valores para ângulos sem leitura.

Se houver RSSI em dBm confiável, preencha `modelo_coleta_diagrama_radiacao.csv`:

```csv
plano,angulo_graus,nivel_db,observacoes
Azimutal,0,-42.3,média de 3 janelas
Azimutal,30,-44.1,média de 3 janelas
Azimutal,60,-51.8,2 válidas; 1 sem leitura
```

O script normaliza cada plano pelo maior nível medido. Assim, o máximo passa a
0 dB e os demais pontos representam a diferença relativa ao máximo.

## Gerar a figura

Instale as dependências e execute:

```powershell
python -m pip install -r requirements-resultados.txt
python scripts\plotar_diagrama_radiacao_antena.py `
  --input modelo_coleta_diagrama_radiacao.csv `
  --output 04-figuras\diagrama_radiacao_antena.png `
  --title "Resposta angular relativa medida do conjunto RFID"
```

Confira se 0° corresponde à frente da antena e 180° às costas. Preserve o CSV,
o log serial, a fotografia da montagem e a figura junto aos demais resultados.

## Texto sugerido para a monografia

> A resposta angular relativa do conjunto RFID foi caracterizada no plano
> azimutal, mantendo-se fixas a tag de referência, a distância, a altura, a
> potência e a configuração do leitor. A antena foi rotacionada em torno do seu
> centro em incrementos de 30°, com três janelas de três segundos por posição.
> Os níveis foram normalizados pelo maior valor observado. Como o ensaio foi
> realizado em ambiente não anecoico, os resultados incluem efeitos de
> multipercurso e caracterizam o conjunto experimental, não constituindo o
> diagrama calibrado fornecido pelo fabricante.

## Critérios para repetir uma posição

Repita apenas a posição afetada quando ocorrer:

- movimento da tag, antena ou cabo;
- pessoa ou objeto atravessando a área de medição;
- alteração de potência ou reinicialização do leitor;
- erro de comunicação serial;
- diferença anormal entre a verificação final e a inicial em 0°.

Registre a repetição e o motivo. Não descarte silenciosamente valores apenas por
serem diferentes do esperado.
