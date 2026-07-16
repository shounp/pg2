#ifndef R200_H
#define R200_H

#include <Arduino.h>
#include <stdint.h>

#define RX_BUFFER_LENGTH 128
#define MAX_EPC_LENGTH 32

class R200 {
 public:
  enum PollResult : uint8_t {
    RESULT_NONE = 0,
    RESULT_TAG,
    RESULT_NO_TAG,
    RESULT_INVALID_FRAME,
    RESULT_COMMAND_ERROR
  };

  R200();

  uint8_t uid[MAX_EPC_LENGTH];
  uint8_t uidLength;

  bool begin(Stream *serial);
  bool loop();
  void poll();
  void setMultiplePollingMode(bool enable = true);
  void setTransmitPower(uint16_t powerCentiDbm);
  bool getWorkArea(uint8_t &region);
  bool getWorkingChannel(uint8_t &channelIndex);
  bool getTransmitPower(uint16_t &powerCentiDbm);
  void dumpLastFrameTo(Print &output) const;
  uint16_t lastFrameLength() const;
  void dumpModuleInfo();
  void dumpUIDToSerial();
  bool dataAvailable() const;
  void discardInput();

  bool hasResult() const;
  PollResult result() const;
  int16_t rssiRaw() const;
  void clearResult();

 private:
  Stream *_serial;
  uint8_t _buffer[RX_BUFFER_LENGTH];
  uint16_t _receivedLength;
  bool _resultReady;
  PollResult _lastResult;
  int16_t _lastRssiRaw;

  bool receiveData(unsigned long timeoutMs = 500);
  bool dataIsValid() const;
  bool queryParameter(uint8_t command, uint8_t *parameter,
                      uint16_t expectedLength,
                      unsigned long timeoutMs = 750);
  void processReceivedData();
  uint8_t calculateCheckSum(const uint8_t *buffer) const;
  uint16_t arrayToUint16(const uint8_t *array) const;

  enum FramePosition : uint8_t {
    HEADER_POS = 0,
    TYPE_POS = 1,
    COMMAND_POS = 2,
    PARAM_LENGTH_MSB_POS = 3,
    PARAM_LENGTH_LSB_POS = 4,
    PARAM_POS = 5
  };

  enum FrameControl : uint8_t {
    FRAME_HEADER = 0xAA,
    FRAME_END = 0xDD
  };

  enum FrameType : uint8_t {
    FRAME_COMMAND = 0x00,
    FRAME_RESPONSE = 0x01,
    FRAME_NOTIFICATION = 0x02
  };

  enum Command : uint8_t {
    CMD_GET_MODULE_INFO = 0x03,
    CMD_SINGLE_POLL = 0x22,
    CMD_MULTIPLE_POLL = 0x27,
    CMD_STOP_MULTIPLE_POLL = 0x28,
    CMD_GET_WORK_AREA = 0x08,
    CMD_GET_WORKING_CHANNEL = 0xAA,
    CMD_SET_TRANSMIT_POWER = 0xB6,
    CMD_GET_TRANSMIT_POWER = 0xB7,
    CMD_EXECUTION_FAILURE = 0xFF
  };

  enum ErrorCode : uint8_t {
    ERR_COMMAND = 0x17,
    ERR_FHSS = 0x20,
    ERR_INVENTORY = 0x15,
    ERR_ACCESS = 0x16,
    ERR_READ = 0x09,
    ERR_WRITE = 0x10
  };
};

#endif
