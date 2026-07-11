#include <SoftwareSerial.h>
#include "R200.h"

// RX, TX do ponto de vista do Arduino
SoftwareSerial rfidSerial(10, 11);
// D10 recebe o TXD do R200
// D11 envia para o RXD do R200

R200 r200;

byte lastUid[MAX_EPC_LENGTH] = {0};
byte lastUidLength = 0;
unsigned long ultimoPoll = 0;

bool uidEhVazio(uint8_t *uid, uint8_t uidLength) {
  if (uidLength == 0) {
    return true;
  }

  for (int i = 0; i < uidLength; i++) {
    if (uid[i] != 0) {
      return false;
    }
  }
  return true;
}

void setup() {
  Serial.begin(9600);

  // Velocidade de comunicação com o R200
  rfidSerial.begin(115200);

  r200.begin(&rfidSerial);

  Serial.println("R200 iniciado");

  // Garante que o modo leitura continua esteja parado
  r200.setMultiplePollingMode(false);
  delay(200);

  // Configura potência para 26.00 dBm
  r200.setTransmitPower(2600);
  delay(200);

  Serial.println("Potencia configurada para 26 dBm");
  Serial.println("Modo leitura manual por poll iniciado");
}

void loop() {
  // Faz uma tentativa de leitura a cada 500 ms
  if (millis() - ultimoPoll >= 500) {
    ultimoPoll = millis();
    r200.poll();
  }

  // Processa resposta recebida do leitor
  r200.loop();

  // Se o EPC mudou, imprime
  if (lastUidLength != r200.uidLength || memcmp(lastUid, r200.uid, r200.uidLength) != 0) {
    memset(lastUid, 0, sizeof(lastUid));
    memcpy(lastUid, r200.uid, r200.uidLength);
    lastUidLength = r200.uidLength;

    if (!uidEhVazio(r200.uid, r200.uidLength)) {
      Serial.print("Tag detectada: ");
      r200.dumpUIDToSerial();
      Serial.println();
    }
  }
}
