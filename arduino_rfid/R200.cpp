#include "R200.h"

#include <Arduino.h>
#include <string.h>

R200::R200()
    : uidLength(0),
      _serial(NULL),
      _receivedLength(0),
      _resultReady(false),
      _lastResult(RESULT_NONE),
      _lastRssiRaw(-1) {
  memset(uid, 0, sizeof(uid));
  memset(_buffer, 0, sizeof(_buffer));
}

bool R200::begin(Stream *serial) {
  _serial = serial;
  clearResult();
  return _serial != NULL;
}

bool R200::loop() {
  if (_serial == NULL || !dataAvailable()) {
    return false;
  }

  if (!receiveData()) {
    return false;
  }

  if (!dataIsValid()) {
    _lastResult = RESULT_INVALID_FRAME;
    _resultReady = true;
    return true;
  }

  processReceivedData();
  return true;
}

bool R200::dataAvailable() const {
  return _serial != NULL && _serial->available() > 0;
}

bool R200::hasResult() const { return _resultReady; }

R200::PollResult R200::result() const { return _lastResult; }

int16_t R200::rssiRaw() const { return _lastRssiRaw; }

void R200::clearResult() {
  _resultReady = false;
  _lastResult = RESULT_NONE;
  _lastRssiRaw = -1;
}

void R200::discardInput() {
  if (_serial == NULL) {
    return;
  }
  while (_serial->available() > 0) {
    _serial->read();
  }
}

bool R200::receiveData(unsigned long timeoutMs) {
  if (_serial == NULL) {
    return false;
  }

  memset(_buffer, 0, sizeof(_buffer));
  _receivedLength = 0;
  uint16_t expectedLength = 0;
  const unsigned long start = millis();

  while (millis() - start < timeoutMs) {
    while (_serial->available() > 0) {
      const uint8_t value = static_cast<uint8_t>(_serial->read());

      // Ignora lixo até o início inequívoco de um quadro.
      if (_receivedLength == 0 && value != FRAME_HEADER) {
        continue;
      }

      if (_receivedLength >= RX_BUFFER_LENGTH) {
        discardInput();
        return false;
      }

      _buffer[_receivedLength++] = value;

      // Cabeçalho(1) + tipo(1) + comando(1) + tamanho(2) + parâmetros
      // + checksum(1) + final(1) = tamanho dos parâmetros + 7 bytes.
      if (_receivedLength == 5) {
        const uint16_t parameterLength =
            arrayToUint16(&_buffer[PARAM_LENGTH_MSB_POS]);
        expectedLength = parameterLength + 7U;
        if (expectedLength > RX_BUFFER_LENGTH || expectedLength < 7U) {
          discardInput();
          return false;
        }
      }

      if (expectedLength > 0 && _receivedLength == expectedLength) {
        return _buffer[_receivedLength - 1] == FRAME_END;
      }
    }
  }

  return false;
}

bool R200::dataIsValid() const {
  if (_receivedLength < 7 || _buffer[HEADER_POS] != FRAME_HEADER ||
      _buffer[_receivedLength - 1] != FRAME_END) {
    return false;
  }

  const uint16_t parameterLength =
      arrayToUint16(&_buffer[PARAM_LENGTH_MSB_POS]);
  const uint16_t expectedLength = parameterLength + 7U;
  if (expectedLength != _receivedLength || expectedLength > RX_BUFFER_LENGTH) {
    return false;
  }

  const uint16_t checksumPosition = PARAM_POS + parameterLength;
  return checksumPosition < _receivedLength &&
         calculateCheckSum(_buffer) == _buffer[checksumPosition];
}

void R200::processReceivedData() {
  const uint8_t command = _buffer[COMMAND_POS];
  const uint16_t parameterLength =
      arrayToUint16(&_buffer[PARAM_LENGTH_MSB_POS]);

  if (command == CMD_SINGLE_POLL || command == CMD_MULTIPLE_POLL) {
    // Resposta: RSSI(1) + PC(2) + EPC(n) + CRC da tag(2).
    if (parameterLength < 5) {
      _lastResult = RESULT_INVALID_FRAME;
      _resultReady = true;
      return;
    }

    const uint16_t rawEpcLength = parameterLength - 5U;
    if (rawEpcLength == 0 || rawEpcLength > MAX_EPC_LENGTH ||
        PARAM_POS + 3U + rawEpcLength > _receivedLength) {
      _lastResult = RESULT_INVALID_FRAME;
      _resultReady = true;
      return;
    }

    memset(uid, 0, sizeof(uid));
    memcpy(uid, &_buffer[PARAM_POS + 3], rawEpcLength);
    uidLength = static_cast<uint8_t>(rawEpcLength);
    _lastRssiRaw = _buffer[PARAM_POS];
    _lastResult = RESULT_TAG;
    _resultReady = true;
    return;
  }

  if (command == CMD_EXECUTION_FAILURE && parameterLength >= 1) {
    const uint8_t error = _buffer[PARAM_POS];
    if (error == ERR_INVENTORY) {
      memset(uid, 0, sizeof(uid));
      uidLength = 0;
      _lastResult = RESULT_NO_TAG;
    } else {
      _lastResult = RESULT_COMMAND_ERROR;
    }
    _resultReady = true;
  }
}

void R200::dumpUIDToSerial() {
  for (uint8_t index = 0; index < uidLength; ++index) {
    if (uid[index] < 0x10) {
      Serial.print('0');
    }
    Serial.print(uid[index], HEX);
  }
}

void R200::poll() {
  if (_serial == NULL) {
    return;
  }
  uint8_t frame[7] = {FRAME_HEADER, FRAME_COMMAND, CMD_SINGLE_POLL,
                      0x00,         0x00,          CMD_SINGLE_POLL,
                      FRAME_END};
  _serial->write(frame, sizeof(frame));
}

void R200::setTransmitPower(uint16_t powerCentiDbm) {
  if (_serial == NULL) {
    return;
  }
  uint8_t frame[9] = {0};
  frame[0] = FRAME_HEADER;
  frame[1] = FRAME_COMMAND;
  frame[2] = CMD_SET_TRANSMIT_POWER;
  frame[3] = 0x00;
  frame[4] = 0x02;
  frame[5] = static_cast<uint8_t>((powerCentiDbm >> 8) & 0xFF);
  frame[6] = static_cast<uint8_t>(powerCentiDbm & 0xFF);
  frame[7] = calculateCheckSum(frame);
  frame[8] = FRAME_END;
  _serial->write(frame, sizeof(frame));
}

bool R200::getWorkArea(uint8_t &region) {
  uint8_t parameter = 0;
  if (!queryParameter(CMD_GET_WORK_AREA, &parameter, 1)) {
    return false;
  }
  region = parameter;
  return true;
}

bool R200::getWorkingChannel(uint8_t &channelIndex) {
  uint8_t parameter = 0;
  if (!queryParameter(CMD_GET_WORKING_CHANNEL, &parameter, 1)) {
    return false;
  }
  channelIndex = parameter;
  return true;
}

bool R200::getTransmitPower(uint16_t &powerCentiDbm) {
  uint8_t parameter[2] = {0};
  if (!queryParameter(CMD_GET_TRANSMIT_POWER, parameter, 2)) {
    return false;
  }
  powerCentiDbm = arrayToUint16(parameter);
  return true;
}

bool R200::queryParameter(uint8_t command, uint8_t *parameter,
                          uint16_t expectedLength,
                          unsigned long timeoutMs) {
  if (_serial == NULL || parameter == NULL) {
    return false;
  }

  discardInput();
  uint8_t frame[7] = {FRAME_HEADER, FRAME_COMMAND, command,
                      0x00,         0x00,          command,
                      FRAME_END};
  _serial->write(frame, sizeof(frame));

  const unsigned long start = millis();
  while (millis() - start < timeoutMs) {
    const unsigned long elapsed = millis() - start;
    const unsigned long remaining = timeoutMs - elapsed;
    if (!receiveData(remaining)) {
      return false;
    }
    if (!dataIsValid()) {
      continue;
    }

    const uint16_t parameterLength =
        arrayToUint16(&_buffer[PARAM_LENGTH_MSB_POS]);
    if (_buffer[TYPE_POS] == FRAME_RESPONSE &&
        _buffer[COMMAND_POS] == command) {
      if (parameterLength != expectedLength) {
        return false;
      }
      memcpy(parameter, &_buffer[PARAM_POS], expectedLength);
      return true;
    }

    // Uma notificação ou falha de inventário tardia pode preceder a resposta
    // da consulta. Como o protocolo não possui ID de transação, ignore quadros
    // de outro comando e continue aguardando o comando solicitado. Uma falha
    // real da consulta terminará em false ao esgotar o timeout.
  }
  return false;
}

void R200::dumpLastFrameTo(Print &output) const {
  for (uint16_t index = 0; index < _receivedLength; ++index) {
    if (_buffer[index] < 0x10) {
      output.print('0');
    }
    output.print(_buffer[index], HEX);
  }
}

uint16_t R200::lastFrameLength() const { return _receivedLength; }

void R200::setMultiplePollingMode(bool enable) {
  if (_serial == NULL) {
    return;
  }

  if (enable) {
    uint8_t frame[10] = {FRAME_HEADER, FRAME_COMMAND, CMD_MULTIPLE_POLL,
                         0x00,         0x03,          0x22,
                         0xFF,         0xFF,          0x4A,
                         FRAME_END};
    _serial->write(frame, sizeof(frame));
  } else {
    uint8_t frame[7] = {FRAME_HEADER, FRAME_COMMAND, CMD_STOP_MULTIPLE_POLL,
                        0x00,         0x00,          CMD_STOP_MULTIPLE_POLL,
                        FRAME_END};
    _serial->write(frame, sizeof(frame));
  }
}

void R200::dumpModuleInfo() {
  if (_serial == NULL) {
    return;
  }
  uint8_t frame[8] = {FRAME_HEADER, FRAME_COMMAND, CMD_GET_MODULE_INFO,
                      0x00,         0x01,          0x00,
                      0x04,         FRAME_END};
  _serial->write(frame, sizeof(frame));
}

uint8_t R200::calculateCheckSum(const uint8_t *buffer) const {
  const uint16_t parameterLength =
      arrayToUint16(&buffer[PARAM_LENGTH_MSB_POS]);
  uint16_t sum = 0;
  for (uint16_t index = 1; index < parameterLength + 5U; ++index) {
    sum += buffer[index];
  }
  return static_cast<uint8_t>(sum & 0xFF);
}

uint16_t R200::arrayToUint16(const uint8_t *array) const {
  return (static_cast<uint16_t>(array[0]) << 8) | array[1];
}
