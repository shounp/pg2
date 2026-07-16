# Firmware de medição RFID UHF

O sketch `arduino_rfid.ino` executa as janelas descritas em
`PROTOCOLO_EXPERIMENTO_MINIMO.md` e imprime os resultados em CSV.

## Ligações

| YPD-R200 | Arduino Uno/Nano | Observação |
|---|---|---|
| TXD | D10 | RX do `SoftwareSerial` |
| RXD | D11 | Usar adequador de nível identificado e eletricamente verificado |
| GND | GND comum | Fonte, leitor e Arduino no mesmo GND |
| 5 V | Fonte externa de 5 V/1 A ou pino de 5 V do Arduino alimentado por USB | As duas formas foram usadas; a fonte deve suportar os picos de corrente do módulo |

Abra `arduino_rfid.ino` na Arduino IDE, selecione a placa correta e carregue o
sketch. O monitor serial deve usar **115200 bit/s** e final de linha `Nova linha`.

O uso de `SoftwareSerial` a 115200 bit/s é uma limitação conhecida. Se aparecerem
muitos timeouts ou quadros inválidos, interrompa a condição e registre a
ocorrência; não complete os valores por estimativa.

Na campanha de 13/07/2026, foram usadas a fonte externa regulada de 5 V/1 A e a
alimentação USB do Arduino. Após as medições, não foi observada alteração
relevante nos resultados em razão da forma de alimentação; por isso, os valores
consolidados não são separados por fonte.

## Primeiro uso

Após reiniciar, o monitor mostra `RFID_MEASUREMENT_READY` e a ajuda.

1. Isole uma tag na zona de leitura e envie:

   ```text
   POLL
   ```

2. Copie o EPC retornado e cadastre-o, sem `0x`:

   ```text
   TAG,1,300833B2DDD9014000000001
   ```

3. Repita para as cinco tags e confira:

   ```text
   TAGS
   ```

Os cadastros são mantidos apenas na RAM e precisam ser refeitos depois de
reiniciar o Arduino.

## Ensaios controlados

Formato:

```text
RUN,<ENSAIO>,<CONDICAO>,<NUMERO_DA_TAG>,<REPETICOES>
```

Exemplos:

```text
RUN,ALCANCE,0.5m,1,5
RUN,ALCANCE,1.5m,3,5
RUN,ORIENTACAO,45graus,2,5
RUN,MATERIAL,METAL_DIRETO,1,5
RUN,MATERIAL,METAL_ESPACADOR_10mm,1,5
```

Antes de cada tentativa o firmware imprime `PREPARE`. Retire a tag da zona,
reposicione-a e aguarde. A janela de leitura começa automaticamente e dura 3 s.

Cada linha `RESULT` possui:

```text
RESULT,millis,ensaio,condicao,tag,tentativa,janela_ms,polls,
leituras_tag,detectou,primeira_ms,epc,rssi_raw,externas,timeouts,
quadros_invalidos,erros_comando
```

Para o registro tabular:

- `detectou` → coluna `detectou_0_1`;
- `epc` → `epc_observado`;
- `primeira_ms / 1000` → `tempo_primeira_leitura_s`;
- anote timeouts, leituras externas e quadros inválidos em `observacoes`.

O `rssi_raw` é o byte bruto recebido do módulo. Não o chame de dBm e não aplique
conversão sem documentação específica.

## Inventário

Depois de cadastrar as cinco tags, posicione-as e envie uma janela por vez:

```text
INV,1,25,5
INV,2,25,5
INV,3,25,5
INV,4,25,5
INV,5,25,5
```

As linhas `INV_TAG` listam EPC, se ele estava cadastrado, quantidade de leituras,
instante da primeira detecção e RSSI bruto. A linha `INV_RESULT` contém:

```text
INV_RESULT,rodada,janela_ms,polls,leituras_validas,unicas_esperadas,
esperadas,completa,duplicatas,externas,tempo_todas_ms,timeouts,
invalidos,overflow
```

Copie os EPCs cadastrados das linhas `INV_TAG` para a coluna de EPCs detectados e
use `unicas_esperadas` como quantidade de tags detectadas. Use `externas` na
coluna correspondente.

## Outros comandos

```text
HELP
INFO
POLL
POWER,2600
VERIFY_RF,20
```

`POWER` aceita de 500 a 2600 centésimos de dBm, em passos de 100, faixa da
variante RPEUM-26 descrita no datasheet. Confirme a variante física antes de
usar o limite superior. O comando compara o valor solicitado com a leitura
posterior da configuração interna do módulo.
Esse retorno não mede a potência efetiva de RF. `VERIFY_RF` lê a região, o valor
configurado de potência e amostras do canal atual, preservando os quadros recebidos em
hexadecimal. O protocolo do fabricante não oferece leitura posterior do estado
do FHSS nem da lista completa de canais, de modo que amostras de canal não
comprovam sozinhas toda a sequência de salto.

O procedimento completo para conferir o adequador de nível, interpretar o
readback e executar os novos ensaios está em
`GUIA_CAMPANHA_COMPLEMENTAR_RFID.md`.

Ao finalizar, copie todo o conteúdo do monitor serial para um arquivo de texto e
preserve-o junto com os demais registros e as fotografias da montagem.
