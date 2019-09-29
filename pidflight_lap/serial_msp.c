#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "pidflight_lap.h"
#include "buf_writer.h"
#include "msp_protocol.h"
#include "serial_msp.h"

static mspPort_t *currentPort;
static bufWriter_t *writer;

static mspPort_t hwPort;

#if defined(ERLT)
static mspPort_t swPort;
#endif

uint8_t lap_number;

static void serialize8(uint8_t a)
{
  bufWriterAppend(writer, a);
  currentPort->checksum ^= a;
}

static void serialize16(uint16_t a)
{
  serialize8((uint8_t)(a >> 0));
  serialize8((uint8_t)(a >> 8));
}

static void serialize32(uint32_t a)
{
  serialize16((uint16_t)(a >> 0));
  serialize16((uint16_t)(a >> 16));
}

static uint8_t read8(void)
{
  return currentPort->inBuf[currentPort->indRX++] & 0xff;
}

static uint16_t read16(void)
{
  uint16_t t = read8();
  t += (uint16_t)read8() << 8;
  return t;
}

static uint32_t read32(void)
{
  uint32_t t = read16();
  t += (uint32_t)read16() << 16;
  return t;
}

static void headSerialResponse(uint8_t err, uint8_t responseBodySize)
{
  serialize8('$');
  serialize8('M');
  serialize8(err ? '!' : '>');
  serialize8(deviceID);

  currentPort->checksum = 0; // start calculating a new checksum

  serialize8(responseBodySize);
  serialize8(currentPort->cmdMSP);
}

static void headSerialReply(uint8_t responseBodySize)
{
  headSerialResponse(0, responseBodySize);
}

static void headSerialError(uint8_t responseBodySize)
{
  headSerialResponse(1, responseBodySize);
}

static void tailSerialReply(void)
{
  serialize8(currentPort->checksum);
}

static bool processOutCommand(uint8_t cmdMSP)
{
  switch (cmdMSP) {
    case MSP_API_VERSION:
      headSerialReply(
          1 + // protocol version length
          API_VERSION_LENGTH
      );
      serialize8(MSP_PROTOCOL_VERSION);
      serialize8(API_VERSION_MAJOR);
      serialize8(API_VERSION_MINOR);
      break;
    case MSP_DEVICE_ID:
      headSerialReply(1);
      serialize8(deviceID);
      break;
    case MSP_STATUS:
      headSerialReply(2);
      serialize8(state);
      serialize8(lapTimer.state);
      break;
    case MSP_CHANNEL:
      headSerialReply(2);
      serialize16(channel);
      break;
    case MSP_RSSI:
      headSerialReply(8);
      serialize16(rssi);
      serialize16(rssi_min);
      serialize16(rssi_max);
      serialize16(rssi_filter);
      break;
    case MSP_RSSI_THRESHOLD:
      headSerialReply(2);
      serialize16(rssi_threshold);
      break;
    case MSP_RSSI_FILTER:
      headSerialReply(4);
      serialize16(rssi_filter_q);
      serialize16(rssi_filter_r);
      break;
    case MSP_LAP_MAX:
      headSerialReply(1);
      serialize8(lap_maximum);
      break;
    case MSP_LAP_MIN_TIME:
      headSerialReply(2);
      serialize16(lapTimer.timerMin / 1000);
      break;
    case MSP_CURRENT_LAP:
      headSerialReply(9);
      serialize8(lapTimer.count);
      serialize32(lapTimer.times[lapTimer.count - 1]);
      serialize16(lapTimer.rssi[lapTimer.count - 1]);
      serialize16(lapTimer.rssi_filter[lapTimer.count - 1]);
      break;
    case MSP_LAP:
      headSerialReply(9);
      serialize8(lap_number);
      serialize32(lapTimer.times[lap_number - 1]);
      serialize16(lapTimer.rssi[lap_number - 1]);
      serialize16(lapTimer.rssi_filter[lap_number - 1]);
      break;
    case MSP_DEBUG:
      headSerialReply(7);
      serialize8(state);
      serialize8(lapTimer.state);
      serialize8(lapTimer.count);
      serialize16(rssi);
      serialize16(rssi_filter);
      break;
    default:
      return false;
  }
  return true;
}

static bool processInCommand(void)
{
  switch (currentPort->cmdMSP) {
    case MSP_RESET:
      reset();
      break;
    case MSP_RSSI_CALIBRATE:
      calibrateRSSI();
      break;
    case MSP_START:
      start();
      break;
    case MSP_ACTIVATE:
      activate();
      break;
    case MSP_DEACTIVATE:
      deactivate();
      break;
    case MSP_SET_DEVICE_ID:
      deviceID = read8();
      break;
    case MSP_SET_LAP:
      lap_number = read8();
      if (lap_number > lapTimer.count) {
        lap_number = lapTimer.count;
      }
      break;
    case MSP_SET_CHANNEL:
      channel = read16();
      channelUpdate();
      break;
    case MSP_SET_RSSI_THRESHOLD:
      rssi_threshold = read16();
      break;
    case MSP_SET_RSSI_FILTER:
      rssi_filter_q = read16();
      rssi_filter_r = read16();
      rssiFilterUpdate();
      break;
    case MSP_SET_LAP_MIN_TIME:
      lapTimer.timerMin = (read16() * 1000);
      break;
    case MSP_SET_LAP_MAX:
      lap_maximum = read8();
      // Prevent user requested maximum lap from exceeding the devices maximum lap hard limit
      if (lap_maximum > LAP_TIMER_MAXIMUM_LAPS) {
        lap_maximum = LAP_TIMER_MAXIMUM_LAPS;
      }
      break;
    case MSP_SET_DEBUG:
      debug = read8();
      break;
    case MSP_EEPROM_WRITE:
      writeEEPROM();
      break;
    default:
      // We do not know how to handle the (valid) message, indicate error MSP $M!
      return false;
  }
  headSerialReply(0);
  return true;
}

static void mspSetCurrentPort(mspPort_t *port)
{
  currentPort = port;
}

static void mspProcessReceivedCommand()
{
  if (!(processOutCommand(currentPort->cmdMSP) || processInCommand())) {
    headSerialError(0);
  }
  tailSerialReply();
  currentPort->c_state = IDLE;
}

static void mspPassThroughCommand()
{
  bufWriterAppend(writer, '$');
  bufWriterAppend(writer, 'M');

  if (currentPort->c_message_type == MESSAGE_IN) {
    bufWriterAppend(writer, '<');
  } else if (currentPort->c_message_type == MESSAGE_OUT) {
    bufWriterAppend(writer, '>');
  }

  bufWriterAppend(writer, currentPort->deviceID);
  bufWriterAppend(writer, currentPort->dataSize);
  bufWriterAppend(writer, currentPort->cmdMSP);

  // Calculate new checksum
  uint8_t checksum = 0;
  checksum ^= currentPort->dataSize;
  checksum ^= currentPort->cmdMSP;

  // Special case logic to handle MSP_SET_DEVICE_ID requests
  if (currentPort->c_message_type == MESSAGE_IN && currentPort->cmdMSP == MSP_SET_DEVICE_ID) {
    uint8_t nextDeviceID = deviceID + 1;
    bufWriterAppend(writer, nextDeviceID);
    checksum ^= nextDeviceID;
  } else { // Otherwise forward the original request
    for (uint8_t i = 0; i < currentPort->dataSize; i++) {
      uint8_t c = currentPort->inBuf[i];
      bufWriterAppend(writer, c);
      checksum ^= c;
    }
  }

  bufWriterAppend(writer, checksum);

  currentPort->c_state = IDLE;
}

static bool mspProcessReceivedData(uint8_t c)
{
  if (currentPort->c_state == IDLE) {
    if (c == '$') {
      currentPort->c_state = HEADER_START;
    } else {
      return false;
    }
  } else if (currentPort->c_state == HEADER_START) {
    currentPort->c_state = (c == 'M') ? HEADER_M : IDLE;
  } else if (currentPort->c_state == HEADER_M) {
    currentPort->c_state = (c == '<' || c == '>') ? HEADER_ARROW : IDLE;
    if (c == '<') {
      currentPort->c_message_type = MESSAGE_IN;
    } else if (c == '>') {
      currentPort->c_message_type = MESSAGE_OUT;
    }
  } else if (currentPort->c_state == HEADER_ARROW) {
     currentPort->c_state = HEADER_DEVICE_ID;
     currentPort->deviceID = c;
  } else if (currentPort->c_state == HEADER_DEVICE_ID) {
    if (c > MSP_PORT_INBUF_SIZE) {
      currentPort->c_state = IDLE;
    } else {
      currentPort->dataSize = c;
      currentPort->offset = 0;
      currentPort->checksum = 0;
      currentPort->indRX = 0;
      currentPort->checksum ^= c;
      currentPort->c_state = HEADER_SIZE;
    }
  } else if (currentPort->c_state == HEADER_SIZE) {
    currentPort->cmdMSP = c;
    currentPort->checksum ^= c;
    currentPort->c_state = HEADER_CMD;
  } else if (currentPort->c_state == HEADER_CMD && currentPort->offset < currentPort->dataSize) {
    currentPort->checksum ^= c;
    currentPort->inBuf[currentPort->offset++] = c;
  } else if (currentPort->c_state == HEADER_CMD && currentPort->offset >= currentPort->dataSize) {
    if (currentPort->checksum == c) {
      currentPort->c_state = COMMAND_RECEIVED;
    } else {
      currentPort->c_state = IDLE;
    }
  }
  return true;
}

static void mspProcessSendCommand(uint8_t cmdMSP)
{
  currentPort->cmdMSP = cmdMSP;
  if (processOutCommand(cmdMSP)) {
    tailSerialReply();
    currentPort->c_state = IDLE;
  }
}
