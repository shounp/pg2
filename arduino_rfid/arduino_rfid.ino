#include <SoftwareSerial.h>
#include <string.h>
#include <stdlib.h>

#include "R200.h"

// D10 recebe TXD do R200; D11 envia comandos ao RXD do R200.
// Use adequação de nível de 5 V para 3,3 V no caminho D11 -> RXD.
SoftwareSerial rfidSerial(10, 11);
R200 r200;

const unsigned long PC_BAUD = 115200;
const unsigned long RFID_BAUD = 115200;
const unsigned long POLL_RESPONSE_TIMEOUT_MS = 750;
const unsigned long POLL_INTERVAL_MS = 500;
const unsigned long CONTROLLED_WINDOW_MS = 3000;
const unsigned long FIRST_PREPARE_MS = 3000;
const unsigned long BETWEEN_ATTEMPTS_MS = 2000;
const uint8_t MAX_REGISTERED_TAGS = 5;
const uint8_t MAX_INVENTORY_TAGS = 10;
const uint8_t COMMAND_BUFFER_LENGTH = 120;

uint16_t requestedPowerCentiDbm = 2600;
char commandBuffer[COMMAND_BUFFER_LENGTH];
uint8_t commandLength = 0;

struct RegisteredTag {
  bool used;
  uint8_t length;
  uint8_t epc[MAX_EPC_LENGTH];
};

struct InventoryTag {
  bool used;
  bool registered;
  uint8_t length;
  uint8_t epc[MAX_EPC_LENGTH];
  uint16_t reads;
  unsigned long firstSeenMs;
  int16_t lastRssiRaw;
};

RegisteredTag registeredTags[MAX_REGISTERED_TAGS];
InventoryTag inventoryTags[MAX_INVENTORY_TAGS];

enum MeasurementStatus : uint8_t {
  MEASURE_TAG = 0,
  MEASURE_NO_TAG,
  MEASURE_TIMEOUT,
  MEASURE_INVALID,
  MEASURE_COMMAND_ERROR
};

struct PollMeasurement {
  MeasurementStatus status;
  int16_t rssiRaw;
};

void printHex(const uint8_t *data, uint8_t length) {
  for (uint8_t index = 0; index < length; ++index) {
    if (data[index] < 0x10) {
      Serial.print('0');
    }
    Serial.print(data[index], HEX);
  }
}

bool epcEquals(const uint8_t *left, uint8_t leftLength,
               const uint8_t *right, uint8_t rightLength) {
  return leftLength == rightLength &&
         memcmp(left, right, leftLength) == 0;
}

int8_t hexNibble(char value) {
  if (value >= '0' && value <= '9') {
    return value - '0';
  }
  if (value >= 'A' && value <= 'F') {
    return value - 'A' + 10;
  }
  if (value >= 'a' && value <= 'f') {
    return value - 'a' + 10;
  }
  return -1;
}

bool parseHexEpc(const char *text, uint8_t *destination, uint8_t &length) {
  if (text == NULL) {
    return false;
  }
  if (text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
    text += 2;
  }

  const size_t characters = strlen(text);
  if (characters == 0 || characters % 2 != 0 ||
      characters > MAX_EPC_LENGTH * 2U) {
    return false;
  }

  length = static_cast<uint8_t>(characters / 2U);
  for (uint8_t index = 0; index < length; ++index) {
    const int8_t high = hexNibble(text[index * 2U]);
    const int8_t low = hexNibble(text[index * 2U + 1U]);
    if (high < 0 || low < 0) {
      length = 0;
      return false;
    }
    destination[index] = static_cast<uint8_t>((high << 4) | low);
  }
  return true;
}

bool parseUnsigned(const char *text, unsigned long &value) {
  if (text == NULL || *text == '\0') {
    return false;
  }
  char *end = NULL;
  value = strtoul(text, &end, 10);
  return end != text && *end == '\0';
}

uint8_t registeredCount() {
  uint8_t count = 0;
  for (uint8_t index = 0; index < MAX_REGISTERED_TAGS; ++index) {
    if (registeredTags[index].used) {
      ++count;
    }
  }
  return count;
}

int8_t registeredIndexForEpc(const uint8_t *epc, uint8_t length) {
  for (uint8_t index = 0; index < MAX_REGISTERED_TAGS; ++index) {
    if (registeredTags[index].used &&
        epcEquals(epc, length, registeredTags[index].epc,
                  registeredTags[index].length)) {
      return index;
    }
  }
  return -1;
}

PollMeasurement performSinglePoll() {
  PollMeasurement measurement;
  measurement.status = MEASURE_TIMEOUT;
  measurement.rssiRaw = -1;

  r200.clearResult();
  r200.poll();
  const unsigned long start = millis();

  while (millis() - start < POLL_RESPONSE_TIMEOUT_MS) {
    r200.loop();
    if (!r200.hasResult()) {
      continue;
    }

    measurement.rssiRaw = r200.rssiRaw();
    switch (r200.result()) {
      case R200::RESULT_TAG:
        measurement.status = MEASURE_TAG;
        break;
      case R200::RESULT_NO_TAG:
        measurement.status = MEASURE_NO_TAG;
        break;
      case R200::RESULT_INVALID_FRAME:
        measurement.status = MEASURE_INVALID;
        break;
      case R200::RESULT_COMMAND_ERROR:
        measurement.status = MEASURE_COMMAND_ERROR;
        break;
      default:
        measurement.status = MEASURE_TIMEOUT;
        break;
    }
    return measurement;
  }
  return measurement;
}

void waitPollInterval(unsigned long pollStart) {
  const unsigned long elapsed = millis() - pollStart;
  if (elapsed < POLL_INTERVAL_MS) {
    delay(POLL_INTERVAL_MS - elapsed);
  }
}

void printHelp() {
  Serial.println(F("# COMANDOS"));
  Serial.println(F("# TAG,<1-5>,<EPC_HEX>"));
  Serial.println(F("# TAGS"));
  Serial.println(F("# RUN,<ENSAIO>,<CONDICAO>,<TAG_1-5>,<REPETICOES>"));
  Serial.println(F("# INV,<RODADA>,<DURACAO_S>,<TAGS_ESPERADAS>"));
  Serial.println(F("# POLL"));
  Serial.println(F("# POWER,<CENTIDBM>   exemplo: POWER,2600"));
  Serial.println(F("# VERIFY_RF,<AMOSTRAS_1_A_20>   exemplo: VERIFY_RF,10"));
  Serial.println(F("# INFO"));
  Serial.println(F("# HELP"));
  Serial.println(F("# Exemplos:"));
  Serial.println(F("# TAG,1,300833B2DDD9014000000001"));
  Serial.println(F("# RUN,ALCANCE,0.5m,1,5"));
  Serial.println(F("# RUN,ORIENTACAO,45graus,2,5"));
  Serial.println(F("# INV,1,25,5"));
}

void printInfo() {
  Serial.print(F("INFO,power_setpoint_requested_centidbm,"));
  Serial.println(requestedPowerCentiDbm);
  Serial.println(F("INFO,power_range_assumes,RPEUM_26_CONFIRM_PHYSICAL_VARIANT"));
  Serial.println(F("INFO,rfid_uart_baud,115200"));
  Serial.println(F("INFO,pc_serial_baud,115200"));
  Serial.println(F("INFO,controlled_window_ms,3000"));
  Serial.println(F("INFO,poll_interval_ms,500"));
  Serial.println(F("INFO,rssi,RAW_NAO_CONVERTIDO"));
  Serial.println(F("INFO,rf_readback,USE_VERIFY_RF"));
  Serial.println(F("INFO,fhss_readback,NAO_EXISTE_NO_PROTOCOLO_V2_3_3"));
  Serial.println(F("INFO,software_serial_115200,VALIDAR_PERDAS"));
}

void printRegionName(uint8_t region) {
  switch (region) {
    case 0x01:
      Serial.print(F("CHINA_900"));
      break;
    case 0x02:
      Serial.print(F("US_FABRICANTE"));
      break;
    case 0x03:
      Serial.print(F("EUROPA"));
      break;
    case 0x04:
      Serial.print(F("CHINA_800"));
      break;
    case 0x06:
      Serial.print(F("COREIA"));
      break;
    default:
      Serial.print(F("DESCONHECIDA"));
      break;
  }
}

bool isKnownRegion(uint8_t region) {
  return region == 0x01 || region == 0x02 || region == 0x03 ||
         region == 0x04 || region == 0x06;
}

void printUsFrequencyMHz(uint8_t channelIndex) {
  // Grade definida pelo protocolo para a região 0x02:
  // f_MHz = 902,25 + 0,5 * índice.
  const uint16_t quarterMHz = 3609U + 2U * channelIndex;
  Serial.print(quarterMHz / 4U);
  Serial.print('.');
  const uint8_t hundredths = (quarterMHz % 4U) * 25U;
  if (hundredths < 10) {
    Serial.print('0');
  }
  Serial.print(hundredths);
}

void printLastRfFrame() {
  if (r200.lastFrameLength() == 0) {
    Serial.print(F("NA"));
  } else {
    r200.dumpLastFrameTo(Serial);
  }
}

void verifyRfConfiguration(uint8_t samples) {
  Serial.println(F("# RF_VERIFY_START"));
  Serial.println(F("RF_TX,GET_REGION,AA0008000008DD"));

  uint8_t region = 0;
  const bool regionOk = r200.getWorkArea(region);
  Serial.print(F("RF_RX,GET_REGION,"));
  Serial.print(regionOk ? F("OK,") : F("FALHA,"));
  printLastRfFrame();
  Serial.println();
  Serial.print(F("RF_VERIFY,region,"));
  if (regionOk) {
    Serial.print(region);
    Serial.print(',');
    printRegionName(region);
  } else {
    Serial.print(F("NA,NA"));
  }
  Serial.println();

  Serial.println(F("RF_TX,GET_POWER,AA00B70000B7DD"));
  uint16_t readPower = 0;
  const bool powerOk = r200.getTransmitPower(readPower);
  Serial.print(F("RF_RX,GET_POWER,"));
  Serial.print(powerOk ? F("OK,") : F("FALHA,"));
  printLastRfFrame();
  Serial.println();
  Serial.print(F("RF_VERIFY,power_setpoint_centidbm,"));
  if (powerOk) {
    Serial.println(readPower);
  } else {
    Serial.println(F("NA"));
  }

  uint8_t observedChannels[20] = {0};
  uint8_t observedCount = 0;
  uint8_t successfulReads = 0;
  uint8_t validPollTriggers = 0;
  bool observedBrazilGap = false;
  bool observedOutsideTargetSubbands = false;

  for (uint8_t sample = 1; sample <= samples; ++sample) {
    const PollMeasurement pollMeasurement = performSinglePoll();
    if (pollMeasurement.status == MEASURE_TAG ||
        pollMeasurement.status == MEASURE_NO_TAG) {
      ++validPollTriggers;
    }
    Serial.print(F("RF_TX,GET_CHANNEL,"));
    Serial.print(sample);
    Serial.println(F(",AA00AA0000AADD"));
    uint8_t channel = 0;
    const bool channelOk = r200.getWorkingChannel(channel);

    Serial.print(F("RF_RX,GET_CHANNEL,"));
    Serial.print(sample);
    Serial.print(',');
    Serial.print(channelOk ? F("OK,") : F("FALHA,"));
    printLastRfFrame();
    Serial.println();

    Serial.print(F("RF_VERIFY,channel_sample,"));
    Serial.print(sample);
    Serial.print(',');
    if (channelOk) {
      ++successfulReads;
      Serial.print(channel);
      Serial.print(',');
      if (regionOk && region == 0x02) {
        printUsFrequencyMHz(channel);
        Serial.print(F(",MHz,"));
        const bool inDocumentedSubbands =
            channel <= 10U || (channel >= 26U && channel <= 51U);
        Serial.print(inDocumentedSubbands ? 1 : 0);
        if (!inDocumentedSubbands) {
          observedOutsideTargetSubbands = true;
        }
        if (channel >= 11U && channel <= 25U) {
          observedBrazilGap = true;
        }
      } else {
        Serial.print(F("NA,NA,NA"));
      }

      bool alreadyObserved = false;
      for (uint8_t index = 0; index < observedCount; ++index) {
        if (observedChannels[index] == channel) {
          alreadyObserved = true;
          break;
        }
      }
      if (!alreadyObserved && observedCount < sizeof(observedChannels)) {
        observedChannels[observedCount++] = channel;
      }
    } else {
      Serial.print(F("NA,NA,NA,NA"));
    }
    Serial.print(F(",poll_status,"));
    Serial.println(static_cast<uint8_t>(pollMeasurement.status));
    delay(100);
  }

  Serial.print(F("RF_VERIFY_SUMMARY,region_read_ok,"));
  Serial.print(regionOk ? 1 : 0);
  Serial.print(F(",region_known,"));
  Serial.print(regionOk && isKnownRegion(region) ? 1 : 0);
  Serial.print(F(",power_setpoint_read_ok,"));
  Serial.print(powerOk ? 1 : 0);
  Serial.print(F(",power_in_rpeum26_range,"));
  Serial.print(powerOk && readPower >= 500U && readPower <= 2600U &&
                       readPower % 100U == 0
                   ? 1
                   : 0);
  Serial.print(F(",poll_trigger_valid_count,"));
  Serial.print(validPollTriggers);
  Serial.print(F(",channel_snapshot_read_ok,"));
  Serial.print(successfulReads);
  Serial.print(F(",channels_distinct,"));
  Serial.print(observedCount);
  Serial.print(F(",gap_907_5_915_observed,"));
  if (regionOk && region == 0x02 && successfulReads > 0) {
    Serial.println(observedBrazilGap ? 1 : 0);
  } else {
    Serial.println(F("NA"));
  }
  Serial.print(F("RF_VERIFY_SUMMARY,outside_target_subbands_observed,"));
  if (regionOk && region == 0x02 && successfulReads > 0) {
    Serial.println(observedOutsideTargetSubbands ? 1 : 0);
  } else {
    Serial.println(F("NA"));
  }
  Serial.println(F("RF_VERIFY,fhss_readback,NAO_DISPONIVEL_NO_PROTOCOLO_V2_3_3"));
  Serial.println(F("RF_VERIFY,regulatory_status,NAO_INFERIR_CONFORMIDADE_SO_PELO_PERFIL"));
  Serial.println(F("# RF_VERIFY_END"));
}

void listTags() {
  for (uint8_t index = 0; index < MAX_REGISTERED_TAGS; ++index) {
    Serial.print(F("TAG_REGISTERED,"));
    Serial.print(index + 1);
    Serial.print(',');
    if (registeredTags[index].used) {
      printHex(registeredTags[index].epc, registeredTags[index].length);
    } else {
      Serial.print(F("NA"));
    }
    Serial.println();
  }
}

void handleTagCommand(char *indexText, char *epcText) {
  unsigned long requestedIndex = 0;
  if (!parseUnsigned(indexText, requestedIndex) || requestedIndex < 1 ||
      requestedIndex > MAX_REGISTERED_TAGS) {
    Serial.println(F("ERROR,TAG,INDICE_DEVE_SER_1_A_5"));
    return;
  }

  RegisteredTag &tag = registeredTags[requestedIndex - 1];
  uint8_t length = 0;
  uint8_t parsed[MAX_EPC_LENGTH] = {0};
  if (!parseHexEpc(epcText, parsed, length)) {
    Serial.println(F("ERROR,TAG,EPC_HEX_INVALIDO"));
    return;
  }

  memset(tag.epc, 0, sizeof(tag.epc));
  memcpy(tag.epc, parsed, length);
  tag.length = length;
  tag.used = true;

  Serial.print(F("OK,TAG,"));
  Serial.print(requestedIndex);
  Serial.print(',');
  printHex(tag.epc, tag.length);
  Serial.println();
}

void runControlled(const char *experiment, const char *condition,
                   uint8_t tagIndex, uint8_t repetitions) {
  if (tagIndex >= MAX_REGISTERED_TAGS ||
      !registeredTags[tagIndex].used) {
    Serial.println(F("ERROR,RUN,TAG_NAO_CADASTRADA"));
    return;
  }
  if (repetitions == 0 || repetitions > 20) {
    Serial.println(F("ERROR,RUN,REPETICOES_DEVE_SER_1_A_20"));
    return;
  }

  const RegisteredTag &expected = registeredTags[tagIndex];
  Serial.print(F("# RUN_START,"));
  Serial.print(experiment);
  Serial.print(',');
  Serial.print(condition);
  Serial.print(F(",TAG"));
  Serial.print(tagIndex + 1);
  Serial.print(',');
  Serial.println(repetitions);

  r200.discardInput();

  for (uint8_t attempt = 1; attempt <= repetitions; ++attempt) {
    Serial.print(F("# PREPARE,TAG"));
    Serial.print(tagIndex + 1);
    Serial.print(F(",attempt,"));
    Serial.print(attempt);
    Serial.println(F(",retire_e_reposicione"));
    delay(attempt == 1 ? FIRST_PREPARE_MS : BETWEEN_ATTEMPTS_MS);

    const unsigned long windowStart = millis();
    unsigned long firstExpectedMs = 0;
    uint16_t polls = 0;
    uint16_t expectedReads = 0;
    uint16_t externalReads = 0;
    uint16_t timeouts = 0;
    uint16_t invalidFrames = 0;
    uint16_t commandErrors = 0;
    int16_t lastExpectedRssi = -1;

    while (millis() - windowStart < CONTROLLED_WINDOW_MS) {
      const unsigned long pollStart = millis();
      const PollMeasurement measurement = performSinglePoll();
      ++polls;

      if (measurement.status == MEASURE_TAG) {
        if (epcEquals(r200.uid, r200.uidLength, expected.epc,
                      expected.length)) {
          ++expectedReads;
          lastExpectedRssi = measurement.rssiRaw;
          if (firstExpectedMs == 0) {
            firstExpectedMs = millis() - windowStart;
          }
        } else {
          ++externalReads;
        }
      } else if (measurement.status == MEASURE_TIMEOUT) {
        ++timeouts;
      } else if (measurement.status == MEASURE_INVALID) {
        ++invalidFrames;
      } else if (measurement.status == MEASURE_COMMAND_ERROR) {
        ++commandErrors;
      }

      waitPollInterval(pollStart);
    }

    Serial.print(F("RESULT,"));
    Serial.print(millis());
    Serial.print(',');
    Serial.print(experiment);
    Serial.print(',');
    Serial.print(condition);
    Serial.print(F(",TAG"));
    Serial.print(tagIndex + 1);
    Serial.print(',');
    Serial.print(attempt);
    Serial.print(',');
    Serial.print(CONTROLLED_WINDOW_MS);
    Serial.print(',');
    Serial.print(polls);
    Serial.print(',');
    Serial.print(expectedReads);
    Serial.print(',');
    Serial.print(expectedReads > 0 ? 1 : 0);
    Serial.print(',');
    if (firstExpectedMs > 0) {
      Serial.print(firstExpectedMs);
    } else {
      Serial.print(F("NA"));
    }
    Serial.print(',');
    if (expectedReads > 0) {
      printHex(expected.epc, expected.length);
    } else {
      Serial.print(F("NA"));
    }
    Serial.print(',');
    if (lastExpectedRssi >= 0) {
      Serial.print(lastExpectedRssi);
    } else {
      Serial.print(F("NA"));
    }
    Serial.print(',');
    Serial.print(externalReads);
    Serial.print(',');
    Serial.print(timeouts);
    Serial.print(',');
    Serial.print(invalidFrames);
    Serial.print(',');
    Serial.println(commandErrors);
  }

  Serial.println(F("# RUN_END"));
}

void clearInventory() {
  memset(inventoryTags, 0, sizeof(inventoryTags));
}

int8_t inventoryIndexForEpc(const uint8_t *epc, uint8_t length) {
  for (uint8_t index = 0; index < MAX_INVENTORY_TAGS; ++index) {
    if (inventoryTags[index].used &&
        epcEquals(epc, length, inventoryTags[index].epc,
                  inventoryTags[index].length)) {
      return index;
    }
  }
  return -1;
}

int8_t addInventoryEpc(const uint8_t *epc, uint8_t length,
                       unsigned long firstSeen, int16_t rssiRaw) {
  int8_t existing = inventoryIndexForEpc(epc, length);
  if (existing >= 0) {
    ++inventoryTags[existing].reads;
    inventoryTags[existing].lastRssiRaw = rssiRaw;
    return existing;
  }

  for (uint8_t index = 0; index < MAX_INVENTORY_TAGS; ++index) {
    if (!inventoryTags[index].used) {
      inventoryTags[index].used = true;
      inventoryTags[index].registered = registeredIndexForEpc(epc, length) >= 0;
      inventoryTags[index].length = length;
      memcpy(inventoryTags[index].epc, epc, length);
      inventoryTags[index].reads = 1;
      inventoryTags[index].firstSeenMs = firstSeen;
      inventoryTags[index].lastRssiRaw = rssiRaw;
      return index;
    }
  }
  return -1;
}

uint8_t uniqueRegisteredInventoryTags() {
  uint8_t count = 0;
  for (uint8_t index = 0; index < MAX_INVENTORY_TAGS; ++index) {
    if (inventoryTags[index].used && inventoryTags[index].registered) {
      ++count;
    }
  }
  return count;
}

uint8_t uniqueInventoryTags() {
  uint8_t count = 0;
  for (uint8_t index = 0; index < MAX_INVENTORY_TAGS; ++index) {
    if (inventoryTags[index].used) {
      ++count;
    }
  }
  return count;
}

void runInventory(uint16_t roundNumber, uint16_t durationSeconds,
                  uint8_t expectedTags) {
  if (durationSeconds == 0 || durationSeconds > 60 || expectedTags == 0 ||
      expectedTags > MAX_REGISTERED_TAGS) {
    Serial.println(F("ERROR,INV,PARAMETROS_INVALIDOS"));
    return;
  }
  if (registeredCount() < expectedTags) {
    Serial.println(F("ERROR,INV,CADASTRE_TODAS_AS_TAGS_ESPERADAS"));
    return;
  }

  clearInventory();
  r200.discardInput();
  const unsigned long durationMs = durationSeconds * 1000UL;
  const unsigned long windowStart = millis();
  unsigned long timeToAllMs = 0;
  uint16_t polls = 0;
  uint16_t validReads = 0;
  uint16_t externalReads = 0;
  uint16_t timeouts = 0;
  uint16_t invalidFrames = 0;
  uint16_t overflowReads = 0;

  Serial.print(F("# INV_START,"));
  Serial.print(roundNumber);
  Serial.print(',');
  Serial.print(durationSeconds);
  Serial.print(',');
  Serial.println(expectedTags);

  while (millis() - windowStart < durationMs) {
    const unsigned long pollStart = millis();
    const PollMeasurement measurement = performSinglePoll();
    ++polls;

    if (measurement.status == MEASURE_TAG) {
      ++validReads;
      const bool isRegistered =
          registeredIndexForEpc(r200.uid, r200.uidLength) >= 0;
      if (!isRegistered) {
        ++externalReads;
      }
      if (addInventoryEpc(r200.uid, r200.uidLength,
                          millis() - windowStart, measurement.rssiRaw) < 0) {
        ++overflowReads;
      }
      if (timeToAllMs == 0 &&
          uniqueRegisteredInventoryTags() >= expectedTags) {
        timeToAllMs = millis() - windowStart;
      }
    } else if (measurement.status == MEASURE_TIMEOUT) {
      ++timeouts;
    } else if (measurement.status == MEASURE_INVALID ||
               measurement.status == MEASURE_COMMAND_ERROR) {
      ++invalidFrames;
    }

    waitPollInterval(pollStart);
  }

  for (uint8_t index = 0; index < MAX_INVENTORY_TAGS; ++index) {
    if (!inventoryTags[index].used) {
      continue;
    }
    Serial.print(F("INV_TAG,"));
    Serial.print(roundNumber);
    Serial.print(',');
    printHex(inventoryTags[index].epc, inventoryTags[index].length);
    Serial.print(',');
    Serial.print(inventoryTags[index].registered ? 1 : 0);
    Serial.print(',');
    Serial.print(inventoryTags[index].reads);
    Serial.print(',');
    Serial.print(inventoryTags[index].firstSeenMs);
    Serial.print(',');
    Serial.println(inventoryTags[index].lastRssiRaw);
  }

  const uint8_t uniqueRegistered = uniqueRegisteredInventoryTags();
  const uint8_t uniqueAll = uniqueInventoryTags();
  const uint16_t duplicates =
      validReads >= uniqueAll ? validReads - uniqueAll : 0;

  Serial.print(F("INV_RESULT,"));
  Serial.print(roundNumber);
  Serial.print(',');
  Serial.print(durationMs);
  Serial.print(',');
  Serial.print(polls);
  Serial.print(',');
  Serial.print(validReads);
  Serial.print(',');
  Serial.print(uniqueRegistered);
  Serial.print(',');
  Serial.print(expectedTags);
  Serial.print(',');
  Serial.print(uniqueRegistered >= expectedTags ? 1 : 0);
  Serial.print(',');
  Serial.print(duplicates);
  Serial.print(',');
  Serial.print(externalReads);
  Serial.print(',');
  if (timeToAllMs > 0) {
    Serial.print(timeToAllMs);
  } else {
    Serial.print(F("NA"));
  }
  Serial.print(',');
  Serial.print(timeouts);
  Serial.print(',');
  Serial.print(invalidFrames);
  Serial.print(',');
  Serial.println(overflowReads);
  Serial.println(F("# INV_END"));
}

void diagnosticPoll() {
  const PollMeasurement measurement = performSinglePoll();
  Serial.print(F("POLL_RESULT,"));
  Serial.print(millis());
  Serial.print(',');
  Serial.print(static_cast<uint8_t>(measurement.status));
  Serial.print(',');
  if (measurement.status == MEASURE_TAG) {
    printHex(r200.uid, r200.uidLength);
  } else {
    Serial.print(F("NA"));
  }
  Serial.print(',');
  Serial.println(measurement.rssiRaw);
}

void handleRunCommand(char *experiment, char *condition, char *tagText,
                      char *repetitionsText) {
  unsigned long tagNumber = 0;
  unsigned long repetitions = 0;
  if (experiment == NULL || condition == NULL ||
      !parseUnsigned(tagText, tagNumber) ||
      !parseUnsigned(repetitionsText, repetitions) || tagNumber < 1 ||
      tagNumber > MAX_REGISTERED_TAGS || repetitions < 1 ||
      repetitions > 20) {
    Serial.println(F("ERROR,RUN,FORMATO_INVALIDO"));
    return;
  }
  runControlled(experiment, condition, tagNumber - 1,
                static_cast<uint8_t>(repetitions));
}

void handleInventoryCommand(char *roundText, char *durationText,
                            char *expectedText) {
  unsigned long roundNumber = 0;
  unsigned long duration = 0;
  unsigned long expected = 0;
  if (!parseUnsigned(roundText, roundNumber) ||
      !parseUnsigned(durationText, duration) ||
      !parseUnsigned(expectedText, expected) || roundNumber > 65535UL ||
      duration > 60UL || expected > MAX_REGISTERED_TAGS) {
    Serial.println(F("ERROR,INV,FORMATO_INVALIDO"));
    return;
  }
  runInventory(static_cast<uint16_t>(roundNumber),
               static_cast<uint16_t>(duration),
               static_cast<uint8_t>(expected));
}

void handlePowerCommand(char *powerText) {
  unsigned long power = 0;
  if (!parseUnsigned(powerText, power) || power < 500UL || power > 2600UL ||
      power % 100UL != 0) {
    Serial.println(F("ERROR,POWER,USE_500_A_2600_EM_PASSOS_DE_100"));
    return;
  }
  requestedPowerCentiDbm = static_cast<uint16_t>(power);
  r200.setTransmitPower(requestedPowerCentiDbm);
  delay(250);
  uint16_t readPower = 0;
  const bool readbackOk = r200.getTransmitPower(readPower);
  Serial.print(F("POWER_RESULT,requested_setpoint_centidbm,"));
  Serial.print(requestedPowerCentiDbm);
  Serial.print(F(",readback_setpoint_centidbm,"));
  if (readbackOk) {
    Serial.print(readPower);
  } else {
    Serial.print(F("NA"));
  }
  Serial.print(F(",match,"));
  Serial.println(readbackOk && readPower == requestedPowerCentiDbm ? 1 : 0);
}

void handleVerifyRfCommand(char *samplesText) {
  unsigned long samples = 10;
  if (samplesText != NULL &&
      (!parseUnsigned(samplesText, samples) || samples < 1 || samples > 20)) {
    Serial.println(F("ERROR,VERIFY_RF,AMOSTRAS_DEVE_SER_1_A_20"));
    return;
  }
  verifyRfConfiguration(static_cast<uint8_t>(samples));
}

void processCommand(char *line) {
  char *command = strtok(line, ",");
  if (command == NULL) {
    return;
  }

  if (strcmp(command, "HELP") == 0) {
    printHelp();
  } else if (strcmp(command, "INFO") == 0) {
    printInfo();
  } else if (strcmp(command, "TAGS") == 0) {
    listTags();
  } else if (strcmp(command, "TAG") == 0) {
    char *tagIndexText = strtok(NULL, ",");
    char *epcText = strtok(NULL, ",");
    handleTagCommand(tagIndexText, epcText);
  } else if (strcmp(command, "RUN") == 0) {
    char *experiment = strtok(NULL, ",");
    char *condition = strtok(NULL, ",");
    char *tagText = strtok(NULL, ",");
    char *repetitionsText = strtok(NULL, ",");
    handleRunCommand(experiment, condition, tagText, repetitionsText);
  } else if (strcmp(command, "INV") == 0) {
    char *roundText = strtok(NULL, ",");
    char *durationText = strtok(NULL, ",");
    char *expectedText = strtok(NULL, ",");
    handleInventoryCommand(roundText, durationText, expectedText);
  } else if (strcmp(command, "POWER") == 0) {
    handlePowerCommand(strtok(NULL, ","));
  } else if (strcmp(command, "VERIFY_RF") == 0) {
    handleVerifyRfCommand(strtok(NULL, ","));
  } else if (strcmp(command, "POLL") == 0) {
    diagnosticPoll();
  } else {
    Serial.println(F("ERROR,COMANDO_DESCONHECIDO_USE_HELP"));
  }
}

void readSerialCommands() {
  while (Serial.available() > 0) {
    const char value = static_cast<char>(Serial.read());
    if (value == '\r') {
      continue;
    }
    if (value == '\n') {
      commandBuffer[commandLength] = '\0';
      if (commandLength > 0) {
        processCommand(commandBuffer);
      }
      commandLength = 0;
      continue;
    }
    if (commandLength < COMMAND_BUFFER_LENGTH - 1) {
      commandBuffer[commandLength++] = value;
    } else {
      commandLength = 0;
      Serial.println(F("ERROR,COMANDO_MUITO_LONGO"));
    }
  }
}

void setup() {
  memset(registeredTags, 0, sizeof(registeredTags));
  memset(inventoryTags, 0, sizeof(inventoryTags));

  Serial.begin(PC_BAUD);
  rfidSerial.begin(RFID_BAUD);
  r200.begin(&rfidSerial);

  delay(300);
  r200.setMultiplePollingMode(false);
  delay(250);
  r200.discardInput();
  r200.setTransmitPower(requestedPowerCentiDbm);
  delay(250);
  r200.discardInput();

  Serial.println(F("# RFID_MEASUREMENT_READY"));
  Serial.println(F("# Saida CSV; RSSI bruto sem conversao para dBm."));
  Serial.println(F("# ATENCAO: SoftwareSerial 115200 deve ser validada quanto a perdas."));
  Serial.println(F("# RESULT,millis,ensaio,condicao,tag,tentativa,janela_ms,polls,leituras_tag,detectou,primeira_ms,epc,rssi_raw,externas,timeouts,invalidos,erros_comando"));
  Serial.println(F("# INV_RESULT,rodada,janela_ms,polls,leituras_validas,unicas_esperadas,esperadas,completa,duplicatas,externas,tempo_todas_ms,timeouts,invalidos,overflow"));
  printHelp();
}

void loop() { readSerialCommands(); }
