#include "rx5808.h"

RX5808::RX5808(uint8_t _rssiPin, uint8_t _spiDataPin, uint8_t _slaveSelectPin, uint8_t _spiClockPin)
{
  rssiPin = _rssiPin;
  spiDataPin = _spiDataPin;
  slaveSelectPin = _slaveSelectPin;
  spiClockPin = _spiClockPin;
}

void RX5808::init()
{
  pinMode(rssiPin, INPUT);
  pinMode(slaveSelectPin, OUTPUT);
  pinMode(spiDataPin, OUTPUT);
  pinMode(spiClockPin, OUTPUT);
}

void RX5808::setFrequency(uint16_t frequency)
{
 uint8_t frequencyIndex = getFrequencyIndex(frequency);
 setFrequencyByIndex(frequencyIndex);
}

uint8_t RX5808::getFrequencyIndex(uint16_t frequency)
{
  for (uint8_t channelIndex = 0; channelIndex < sizeof(channelFreqTable); channelIndex++) {
    if (frequency == pgm_read_word_near(channelFreqTable + channelIndex)) return channelIndex;
  }
}

uint16_t RX5808::readRssi()
{
  volatile uint16_t rssi = 0;

  for (uint8_t i = 0; i < RSSI_READS; i++) {
    rssi += analogRead(rssiPin);
  }

  rssi = rssi / RSSI_READS; // average of RSSI_READS readings

  return rssi;
}

void RX5808::waitRssi()
{
#ifdef MIN_TUNE_TIME
  delay(MIN_TUNE_TIME);
#endif
}

void RX5808::setFrequencyByIndex(uint8_t index)
{
  uint8_t i;
  uint16_t channelData;

  channelData = pgm_read_word_near(channelTable + index);

  // bit bash out 25 bits of data
  // Order: A0-3, !R/W, D0-D19
  // A0=0, A1=0, A2=0, A3=1, RW=0, D0-19=0
  serialEnable(HIGH);
  delayMicroseconds(1);
  serialEnable(LOW);

  serialSendBit(LOW);
  serialSendBit(LOW);
  serialSendBit(LOW);
  serialSendBit(HIGH);

  serialSendBit(LOW);

  // Remaining zeros
  for (i = 20; i > 0; i--) {
    serialSendBit(LOW);
  }

  // Clock the data in
  serialEnable(HIGH);
  delayMicroseconds(1);
  serialEnable(LOW);

  // Second is the channel data from the lookup table
  // 20 bytes of register data are sent, but the MSB 4 bits are zeros
  // register address = 0x1, write, data0-15=channelData data15-19=0x0
  serialEnable(HIGH);
  serialEnable(LOW);

  // Register 0x1
  serialSendBit(HIGH);
  serialSendBit(LOW);
  serialSendBit(LOW);
  serialSendBit(LOW);

  // Write to register
  serialSendBit(HIGH);

  // D0-D15
  // Note: Loop runs backwards as more efficent on AVR
  for (i = 16; i > 0; i--) {
    // Is bit high or low?
    if (channelData & 0x1) {
      serialSendBit(HIGH);
    } else {
      serialSendBit(LOW);
    }

    // Shift bits along to check the next one
    channelData >>= 1;
  }

  // Remaining D16-D19
  for (i = 4; i > 0; i--) {
    serialSendBit(LOW);
  }

  // Finished clocking data in
  serialEnable(HIGH);
  delayMicroseconds(1);

  digitalWrite(slaveSelectPin, LOW);
  digitalWrite(spiClockPin, LOW);
  digitalWrite(spiDataPin, LOW);

  // Wait to allow frequency to be tuned
  waitRssi();
}

void RX5808::serialSendBit(const uint8_t bit)
{
  digitalWrite(spiClockPin, LOW);
  delayMicroseconds(1);

  digitalWrite(spiDataPin, bit);
  delayMicroseconds(1);
  digitalWrite(spiClockPin, HIGH);
  delayMicroseconds(1);

  digitalWrite(spiClockPin, LOW);
  delayMicroseconds(1);
}

void RX5808::serialEnable(const uint8_t level)
{
  delayMicroseconds(1);
  digitalWrite(slaveSelectPin, level);
  delayMicroseconds(1);
}
