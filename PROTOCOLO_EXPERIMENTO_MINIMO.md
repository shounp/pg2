# Protocolo experimental mínimo — RFID UHF

Tempo previsto: 50–70 minutos. O objetivo é obter uma base pequena, real e
defensável, priorizando o inventário portátil. O ensaio de passagem foi retirado.

## Antes de começar — 10 minutos

1. Use fonte externa de 5 V, GND comum e confira as conexões.
2. Posicione o centro da antena a 150 cm do piso.
3. Preencha a aba `Configuracao`, inclusive local, placa, firmware, potência,
   região/canal ou a expressão `NÃO VERIFICADO`.
4. Cadastre cinco EPCs na aba `Tags`.
5. Tire pelo menos três fotos: montagem elétrica, vista geral da sala e régua ou
   trena mostrando a geometria.
6. Confirme que `TAG1` aparece no monitor serial antes de iniciar.

## Definição de tentativa

Cada tentativa é uma janela de 3 segundos. Antes de iniciá-la, retire a tag da
zona de leitura por aproximadamente 2 segundos, reposicione-a e aguarde o tempo
de estabilização registrado na configuração.

- Preencha `1` se o EPC esperado aparecer ao menos uma vez na janela.
- Preencha `0` se ele não aparecer.
- Não complete posteriormente uma tentativa esquecida por estimativa.
- Tempo para primeira leitura e log serial são opcionais.
- Não use RSSI convertido se a fórmula do módulo não estiver documentada.

## 1. Alcance — aproximadamente 20 minutos

- Use `TAG1`, `TAG2` e `TAG3`.
- Antena e tag a 150 cm, alinhadas a 0°.
- Use o mesmo suporte de papelão em todas as condições.
- Distâncias horizontais: 0,5; 1,0; 1,5 e 2,0 m.
- Faça cinco tentativas por tag em cada distância: 60 tentativas no total.
- Agrupe por distância para evitar movimentação desnecessária da montagem.

## 2. Orientação — aproximadamente 12 minutos

- Mantenha distância horizontal de 1,5 m e alturas de 150 cm.
- Use as mesmas três tags e o mesmo suporte do alcance.
- Teste 0°, 45° e 90° em relação à polarização linear da antena.
- Faça cinco tentativas por tag em cada ângulo: 45 tentativas no total.
- Fotografe ou marque fisicamente como cada ângulo foi definido.

## 3. Material — aproximadamente 8 minutos

- Use sempre `TAG1`, a 1,5 m, 150 cm e 0°.
- Teste papelão, metal direto e metal com espaçador de 10 mm.
- Faça cinco tentativas por condição: 15 tentativas no total.
- Use a mesma tag para não confundir material com diferença entre exemplares.

## 4. Inventário — aproximadamente 10 minutos

- Posicione as cinco tags cadastradas em locais fixos e fotografe as posições.
- Faça sempre o mesmo percurso curto.
- Execute cinco janelas de 10 segundos.
- Em cada janela, limpe a lista anterior e registre os EPCs distintos detectados.
- Preencha quantidade detectada, leituras externas e observações.
- Não conte leituras repetidas do mesmo EPC como bens adicionais.

## Encerramento — 5 minutos

1. Confira a aba `Resumo`; todas as condições devem aparecer como `COMPLETO`.
2. Explique qualquer célula vazia ou condição interrompida.
3. Salve uma cópia com data e hora no nome.
4. Preserve a planilha, fotografias, firmware utilizado e qualquer log serial.
5. Não altere os resultados depois da coleta, exceto para corrigir erro de
   digitação claramente documentado.

Se o tempo acabar, complete primeiro alcance, orientação e inventário. É melhor
ter três blocos íntegros do que preencher cinco blocos sem controle.
