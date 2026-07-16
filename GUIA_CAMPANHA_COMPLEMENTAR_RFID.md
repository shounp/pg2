# Guia da campanha complementar RFID UHF

Este guia descreve ensaios futuros. As linhas vazias adicionadas à planilha não
representam resultados já obtidos e não devem ser incorporadas ao PG2 antes da
execução e da conferência da coleta.

## Ordem recomendada

Execute os blocos em dias separados, se necessário:

1. `Fatorial_Tag_Suporte`: aproximadamente 35 a 45 minutos.
2. `Repeticao_Sessao2`: aproximadamente 50 a 65 minutos em outro dia.
3. `Comparacao_Antenas`: aproximadamente 30 a 40 minutos, quando a segunda
   antena estiver disponível.

Não é necessário fazer os três blocos no mesmo dia. É preferível concluir um
bloco com todas as condições controladas a deixar vários blocos incompletos.

## 1. Rotação entre etiquetas e suportes

A aba `Fatorial_Tag_Suporte` contém 162 janelas. O desenho cruza:

- três exemplares, `TAG1`, `TAG2` e `TAG3`;
- três suportes, papelão, madeira e plástico;
- duas distâncias, 1,0 e 2,0 m;
- três orientações, 0, 45 e 90 graus;
- três blocos independentes de remontagem.

Cada etiqueta passa por todos os suportes. Assim, a comparação não mantém uma
tag permanentemente associada a um único material. O desenho permite separar,
de forma exploratória, o efeito do exemplar da condição de suporte ensaiada.
Ele não permite generalizar o resultado para todos os tipos de papelão, madeira
ou plástico, porque há somente um cupom físico de cada material.

Procedimento:

1. Produza três cupons de dimensões semelhantes e identifique-os como `PAP1`,
   `MAD1` e `PLA1`.
2. Marque o centro e a orientação de referência em cada cupom.
3. Em cada bloco, remova e remonte cada tag no suporte indicado. Não mantenha a
   mesma montagem entre os blocos.
4. Use sempre o mesmo método de fixação, altura de 150 cm, janela de 3 s e
   intervalo de aproximadamente 2 s.
5. Siga `ordem_execucao`; ela foi preparada para distribuir as combinações no
   tempo. Envie o conteúdo de `comando_serial` uma linha por vez.
6. Preencha `detectou_0_1` com 1 somente se o EPC esperado aparecer na janela.
   Caso contrário, preencha 0. Não estime células esquecidas.

Três repetições por célula ainda produzem intervalos amplos. A finalidade deste
bloco é corrigir o confundimento entre exemplar e suporte com uma carga de
coleta executável, não estabelecer uma comparação universal entre materiais.

## 2. Estabilidade em outro dia e em outro ambiente

A aba `Repeticao_Sessao2` contém 140 linhas que reproduzem a parte
individualizada da campanha principal: 60 janelas de alcance, 45 de orientação,
30 de material e cinco rodadas de inventário.

Para avaliar estabilidade temporal, repita primeiro no mesmo Laboratório de
Práticas Digitais em um dia diferente. Mantenha montagem, posições, percurso,
potência, antena e etiquetas tão próximos quanto possível da primeira sessão.
Registre data, horário, ocupação, objetos próximos e qualquer diferença.
Execute as linhas na ordem apresentada e envie o `comando_serial` correspondente
a cada tentativa ou rodada.

Se houver tempo para um segundo ambiente, faça uma terceira sessão. Não troque
simultaneamente o dia e a sala quando a intenção for separar os dois efeitos:
uma diferença observada ficaria atribuível tanto ao tempo quanto ao ambiente.
O campo `sessao_id` permite copiar o mesmo desenho para uma terceira sessão sem
alterar o protocolo.

## 3. Comparação entre antenas

A aba `Comparacao_Antenas` contém 90 janelas: duas antenas, três tags, três
orientações e cinco blocos. A distância é 1,5 m e o suporte deve permanecer
igual em todas as condições.

Use a APCA8090 linear como referência. A alternativa mais simples é uma antena
circular de porta única adequada à faixa e ao leitor. Uma antena de dupla
polarização com duas portas não produz diversidade quando somente uma porta é
ligada ao único conector SMA do YPD-R200. Nesse caso, seria necessário testar
cada porta como condição separada ou usar chaveamento de RF apropriado.

Procedimento:

1. Registre modelo, polarização, ganho nominal, cabo e perda informada para cada
   antena.
2. Marque no piso a posição da tag e o eixo de apontamento da antena.
3. Mantenha distância, altura, ambiente, cabo e janela de leitura constantes.
4. Alterne qual antena é testada primeiro conforme a coluna
   `sequencia_antenas`, para reduzir efeito de deriva temporal.
5. Envie o `comando_serial` de cada linha e confira a antena, a tag e o ângulo
   antes de iniciar a janela.
6. Leia de volta o valor configurado de potência após cada troca. Não copie o
   valor comandado para o campo reservado à leitura posterior.
7. Calcule a EIRP nominal a partir do valor configurado, do ganho nominal e da
   perda de cabo. Esse cálculo não substitui uma medição de RF no conector ou no
   campo. Compare preferencialmente os sistemas sob EIRP nominal equivalente, dentro
   das condições permitidas para o equipamento. Se os ganhos forem diferentes
   e a potência não for ajustada, declare que a comparação é entre os dois
   sistemas completos, e não apenas entre polarizações.
8. Desligue o módulo antes de trocar a antena e nunca transmita sem antena de
   impedância adequada ou carga de RF compatível.

O resultado será preliminar. Para transformar a recomendação em evidência, as
duas antenas devem ser medidas fisicamente sob o mesmo protocolo; preencher a
aba sem a execução real não é válido.

## 4. Confirmação elétrica do adequador de nível

O datasheet local do YPD-R200 informa alimentação de 3,6 a 5,5 V e interface
UART TTL, mas não apresenta de forma inequívoca os limites absolutos dos pinos
RXD e TXD. Portanto, a tensão de alimentação do módulo não prova que seu RXD
tolera diretamente 5 V. Identifique o conversor usado e consulte também a
documentação específica desse componente.

### Verificação com o circuito desligado

1. Desconecte USB, fonte e módulo.
2. Fotografe o caminho completo entre D11 do Arduino e RXD do YPD-R200.
3. Identifique o código do conversor ou os valores dos dois resistores.
4. Em continuidade ou resistência, confirme o caminho D11, entrada do
   adequador, saída do adequador e RXD, além do GND comum.
5. Nunca use continuidade ou resistência em circuito energizado.

Se for divisor resistivo, registre os valores e confira:

```text
Vsaida = 5 V × Rbaixo / (Rcima + Rbaixo)
```

O valor calculado deve coincidir com a tensão-alvo do projeto do adaptador. Um
divisor projetado para 5 V a 3,3 V deve produzir valor próximo de 3,3 V.

### Verificação energizada

Um multímetro permite verificar as alimentações e o nível de repouso, mas não
prova a integridade de pulsos UART a 115200 bit/s. Para essa evidência, use
osciloscópio ou analisador lógico:

1. Confirme antes a tensão máxima aceita pelas entradas do instrumento.
2. Ligue todos os terras somente ao GND comum do circuito.
3. Meça simultaneamente D11 e a saída do adequador ligada ao RXD.
4. Execute várias vezes `VERIFY_RF,20` para produzir tráfego UART.
5. Em 115200 bit/s, cada bit dura aproximadamente 8,68 microssegundos.
6. Confirme o nível alto, o nível baixo próximo de 0 V e bordas legíveis na
   entrada e na saída do adequador.
7. Repita sob carga, com o RXD do módulo conectado.
8. Meça também o caminho TXD do R200 para D10 e decodifique como UART 8N1.
9. Durante inventário a 26 dBm, observe se a alimentação de 5 V permanece
   estável; o datasheet informa pico de corrente próximo de 380 mA nessa
   variante.

Não ligue os instrumentos ao conector SMA. Uma medição conduzida de RF exige
atenuação e instrumentos próprios; 26 dBm não devem ser aplicados diretamente a
uma entrada de analisador sem conferir seu limite.

Evidência mínima a preservar:

- foto identificada da montagem;
- modelo do conversor ou valores dos resistores;
- tensões de entrada, saída e alimentação;
- captura das formas de onda e decodificação 115200 8N1;
- data, instrumento e revisão do firmware.

## 5. Leitura posterior de região, canal e potência

O firmware dispõe do comando:

```text
VERIFY_RF,20
```

Ele consulta a região com `0x08`, o valor configurado de potência com `0xB7` e o canal atual com
`0xAA`. Entre as amostras de canal são executadas consultas de inventário para
observar mais de um estado possível. O log contém os quadros recebidos em
hexadecimal e os valores decodificados.

O retorno de `0xB7` confirma o parâmetro armazenado no módulo. Ele não mede a
potência efetivamente entregue ao conector nem a potência irradiada pela antena.
Essas grandezas exigem instrumentação de RF e um arranjo de medição adequado.
A faixa de 5 a 26 dBm usada pelo firmware corresponde à variante RPEUM-26;
confirme a identificação física do módulo antes de usar 26 dBm.

Para a região `0x02`, chamada de `US` pelo fabricante, o protocolo usa:

```text
frequência em MHz = 902,25 + 0,5 × índice do canal
```

Os índices 0 a 10 correspondem a 902,25 a 907,25 MHz e os índices 26 a 51, a
915,25 a 927,75 MHz. Os índices 11 a 25 ficam na lacuna de 907,5 a 915 MHz e não
devem ser tratados como pertencentes às duas subfaixas RFID citadas no Ato
Anatel nº 14.448/2017 em sua redação consolidada.

O rótulo `US` é uma denominação do fabricante e, sozinho, não comprova
conformidade regulatória. Além da frequência central, homologação, largura de
canal, emissões, potência e mecanismo de seleção também importam. O protocolo
do módulo não oferece comando para ler de volta o estado do FHSS nem a lista
completa de canais. Consequentemente:

- uma resposta positiva ao comando de ativação confirmaria apenas sua execução;
- várias leituras de `GET_CHANNEL` mostram canais observados, mas não provam
  quais são todos os canais possíveis;
- a verificação completa do salto exige captura prolongada com analisador de
  espectro configurado adequadamente.

Salve todo o bloco entre `RF_VERIFY_START` e `RF_VERIFY_END`, junto com data,
hora, revisão do firmware e fotos. Se `outside_target_subbands_observed` ou
`gap_907_5_915_observed` for 1, interrompa a transmissão e revise a configuração.
Mesmo quando ambos forem 0, não conclua que a lista inteira foi comprovada
apenas por uma amostra finita.

## Referências técnicas usadas no procedimento

- `Documentacao/YPD-R200 User Protocol V2.3.3.pdf`, comandos de região, canal,
  FHSS e potência.
- `Documentacao/YPD-R200 Product Datasheet V0.2.pdf`, alimentação, potência e
  corrente de pico.
- Ato Anatel nº 14.448, de 4 de dezembro de 2017, texto consolidado:
  <https://informacoes.anatel.gov.br/legislacao/atos-de-certificacao-de-produtos/2017/1139-ato-14451>.
