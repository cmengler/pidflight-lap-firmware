#ifndef serial_msp_h
#define serial_msp_h

typedef enum {
  IDLE,
  HEADER_START,
  HEADER_M,
  HEADER_ARROW,
  HEADER_DEVICE_ID,
  HEADER_SIZE,
  HEADER_CMD,
  COMMAND_RECEIVED
} mspState_e;

typedef enum {
  MESSAGE_IN,
  MESSAGE_OUT
} mspMessageType_e;

#define MSP_PORT_INBUF_SIZE 64

typedef struct mspPort_s {
  uint8_t offset;
  uint8_t dataSize;
  uint8_t checksum;
  uint8_t indRX;
  uint8_t inBuf[MSP_PORT_INBUF_SIZE];
  mspState_e c_state;
  mspMessageType_e c_message_type;
  uint8_t cmdMSP;
  uint8_t deviceID;
} mspPort_t;

#endif